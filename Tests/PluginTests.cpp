#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <cstdio>
#include <set>
static int failures = 0;
#define CHECK(c, ...) do { if (!(c)) { ++failures; std::printf ("FAIL %d: ", __LINE__); std::printf (__VA_ARGS__); std::printf ("\n"); } } while (0)

struct Head : juce::AudioPlayHead
{
    double ppq = 0, bpm = 120; bool playing = true;
    juce::Optional<PositionInfo> getPosition() const override
    { PositionInfo p; p.setBpm (bpm); p.setPpqPosition (ppq); p.setIsPlaying (playing); return p; }
};

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    const double sr = 48000; const int block = 480; const double spb = sr * 60 / 120;
    MidiGenProcessor p; Head head;
    p.setPlayHead (&head); p.setPlayConfigDetails (0, 2, sr, block); p.prepareToPlay (sr, block);

    // the "clip" on the track: Am F C G, 1 bar each, hits on 1 and the and-of-2
    const int ch[4][4] = { { 57, 60, 64, 69 }, { 53, 57, 60, 65 }, { 55, 60, 64, 67 }, { 55, 59, 62, 67 } };
    auto clipEvents = [&] (double b0, double b1, juce::MidiBuffer& m)
    {
        for (double loopStart = std::floor (b0 / 16) * 16; loopStart < b1; loopStart += 16)
            for (int bar = 0; bar < 4; ++bar)
                for (double t : { 0.0, 1.5 })
                {
                    const double on = loopStart + bar * 4 + t, off = on + 1.2;
                    for (int n : ch[bar])
                    {
                        if (on >= b0 && on < b1) m.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 100), (int) ((on - b0) * spb));
                        if (off >= b0 && off < b1) m.addEvent (juce::MidiMessage::noteOff (1, n), (int) ((off - b0) * spb));
                    }
                }
    };
    struct Ev { double s; int note; bool on; };
    auto run = [&] (int blocks)
    {
        std::vector<Ev> out;
        juce::AudioBuffer<float> buf (2, block);
        for (int b = 0; b < blocks; ++b)
        {
            juce::MidiBuffer m;
            const double b0 = head.ppq, b1 = head.ppq + block / spb;
            if (head.playing) clipEvents (b0, b1, m);
            p.processBlock (buf, m);
            for (const auto meta : m) if (meta.getMessage().isNoteOnOrOff())
                out.push_back ({ b * (double) block + meta.samplePosition, meta.getMessage().getNoteNumber(), meta.getMessage().isNoteOn() });
            if (head.playing) head.ppq += block / spb;
            if (b % 10 == 0) p.poll();
        }
        return out;
    };
    auto pump = [&] { for (int i = 0; i < 3; ++i) p.poll(); };

    // start in the middle of bar 2 so it has to wait for bar 1 of the loop
    head.ppq = 5.0;
    p.startListening();
    auto thru = run ((int) (spb * 30 / block));     // ~30 beats: waits, then records 16
    pump();
    CHECK (p.getListenState() == 0, "listening finished (%d)", p.getListenState());
    CHECK (p.hasSource(), "learned the clip");
    if (p.hasSource())
    {
        juce::String names; for (auto& c : p.getVariations()[0].chords) names << c.name << " ";
        std::printf ("learned: %s(key %d %s)\n", names.toRawUTF8(), p.getSource()->tonicPc, p.getSource()->minor ? "minor" : "major");
        CHECK (p.getSource()->spans.size() == 4 && p.getSource()->tonicPc == 9 && p.getSource()->minor, "Am F C G in A minor");
        CHECK (p.getVariations().size() == 9, "original + 8");
        for (size_t i = 1; i < p.getVariations().size(); ++i)
        {
            std::printf ("  %s:", p.getVariations()[i].label.c_str());
            for (auto& c : p.getVariations()[i].chords) std::printf (" %s", c.name.c_str());
            std::printf ("\n");
        }
    }
    CHECK (! thru.empty(), "the original passes through while listening");

    // pick variation C: the clip's notes are replaced by the new chords, in time with the clip's rhythm
    p.select (3); pump();
    head.ppq = 0.0;
    auto out = run ((int) (spb * 16 / block));
    const auto& v = p.getVariations()[3];
    std::set<int> want; for (auto& c : v.chords) for (int n : c.notes) want.insert (n);
    int ons = 0, foreign = 0; double worst = 0;
    std::set<double> hitBeats;
    for (auto& e : out) if (e.on)
    {
        ++ons; if (! want.count (e.note)) ++foreign;
        const double beat = e.s / spb; hitBeats.insert (std::round (beat * 8) / 8);
        const double nearest = std::round (beat * 2) / 2; worst = std::max (worst, std::abs (beat - nearest) * spb);
    }
    CHECK (ons > 20 && foreign == 0, "only the new chords play (%d notes, %d from the old clip)", ons, foreign);
    CHECK (worst <= 1.0, "on the clip's grid (worst %.1f samples)", worst);
    CHECK (hitBeats.count (1.5) && hitBeats.count (5.5), "keeps the clip's rhythm (and-of-2 hits)");
    std::printf ("variation C played %d notes, worst timing %.2f samples\n", ons, worst);

    // back to ORIGINAL: the clip passes through again
    p.select (0); pump();
    head.ppq = 0.0;
    out = run ((int) (spb * 3.5 / block));
    std::set<int> origNotes; for (auto& e : out) if (e.on) origNotes.insert (e.note);
    { juce::String s; for (int x : origNotes) s << x << " "; CHECK (origNotes == std::set<int> ({ 57, 60, 64, 69 }), "original bar 1 = Am again (got %s)", s.toRawUTF8()); }

    if (auto* snap = std::getenv ("MG_SNAPSHOT"))
    {
        p.select (3); p.poll();
        std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
        ed->setSize (MidiGenPanel::designW, MidiGenPanel::designH);
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.0f);
        juce::PNGImageFormat png; juce::File (snap).deleteFile();
        juce::FileOutputStream os { juce::File (snap) }; png.writeImageToStream (img, os);
        std::printf ("snapshot written\n");
    }

    // export + state
    const auto f = p.writeMidiFile (2);
    CHECK (f.existsAsFile(), "midi export"); std::printf ("export: %s\n", f.getFileName().toRawUTF8());
    const auto all = p.writeMidiFile (-1);
    CHECK (all.getSize() > f.getSize() * 4, "all 8 export");
    juce::MemoryBlock st; p.getStateInformation (st);
    MidiGenProcessor q; q.setStateInformation (st.getData(), (int) st.getSize());
    bool same = q.getVariations().size() == p.getVariations().size();
    for (size_t i = 0; same && i < q.getVariations().size(); ++i)
        same = q.getVariations()[i].chords.size() == p.getVariations()[i].chords.size()
            && q.getVariations()[i].chords[0].name == p.getVariations()[i].chords[0].name;
    CHECK (same, "the same 8 progressions come back with the project");

    // learning from a HOOKSMITH export file
    if (auto* hs = std::getenv ("HS_FILE"))
    {
        juce::String err;
        CHECK (p.learnFromFile (juce::File (hs), err), "learn from file: %s", err.toRawUTF8());
        juce::String names; for (auto& c : p.getVariations()[0].chords) names << c.name << " ";
        std::printf ("from HOOKSMITH file: %s\n", names.toRawUTF8());
    }
    std::printf ("%d failures\n", failures);
    return failures ? 1 : 0;
}
