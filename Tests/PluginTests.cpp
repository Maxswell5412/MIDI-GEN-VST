#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <cstdio>
#include <set>
#include <array>
#include <cmath>
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
        CHECK ((int) p.getVariations().size() == MidiGenProcessor::kDial + 1, "original + 8 slots + dial");
        CHECK (p.getSelected() == MidiGenProcessor::kDial, "after learning you hear the dial");
        for (int i = 1; i <= 6; ++i)
        {
            p.setIdea (i); pump();
            std::printf ("  idea %d:", i);
            for (auto& c : p.getVariations()[(size_t) MidiGenProcessor::kDial].chords) std::printf (" %s", c.name.c_str());
            std::printf ("   [%s]\n", p.getVariations()[(size_t) MidiGenProcessor::kDial].tags.c_str());
        }
    }
    CHECK (! thru.empty(), "the original passes through while listening");

    // SAVE ideas 2, 4, 5 into slots A, B, C
    for (int i : { 2, 4, 5 }) { p.setIdea (i); pump(); CHECK (p.save() >= 1, "saved idea %d", i); }
    CHECK (p.slotUsed (1) && p.slotUsed (2) && p.slotUsed (3) && ! p.slotUsed (4), "three slots used");
    p.setIdea (4); pump();
    const auto slotB = p.getVariations()[2];
    CHECK (slotB.chords.size() == p.getVariations()[(size_t) MidiGenProcessor::kDial].chords.size()
           && slotB.chords[0].name == p.getVariations()[(size_t) MidiGenProcessor::kDial].chords[0].name, "slot B = idea 4");
    p.setIdea (9); pump();
    CHECK (p.getVariations()[2].chords[0].name == slotB.chords[0].name, "a saved slot doesn't change when the dial turns");

    // play slot C: only its chords, on the grid, keeping the clip's rhythm, and NO silent gaps
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
    auto longestGap = [&] (const std::vector<Ev>& evs)
    {
        // longest stretch with nothing sounding, after the first note
        std::array<int, 128> on {}; int sounding = 0; double silentFrom = -1, longest = 0; bool started = false;
        for (auto& e : evs)
        {
            if (e.on) { if (sounding == 0 && silentFrom >= 0) longest = std::max (longest, e.s - silentFrom); if (on[(size_t) e.note]++ == 0) ++sounding; started = true; }
            else if (on[(size_t) e.note] > 0 && --on[(size_t) e.note] == 0 && --sounding == 0) silentFrom = e.s;
        }
        (void) started;
        return longest;
    };
    const double gap = longestGap (out);
    CHECK (gap <= 2.0, "no silent gaps between chords (longest %.0f samples)", gap);
    std::printf ("slot C played %d notes, worst timing %.2f samples, longest gap %.0f samples\n", ons, worst, gap);

    // turning the dial while it plays: the new chord is heard at once (not at the next chord)
    p.select (MidiGenProcessor::kDial); pump();
    head.ppq = 1.0;
    run ((int) (spb * 0.25 / block));
    p.setIdea (17); pump();
    auto after = run (2);
    int quickOns = 0; for (auto& e : after) if (e.on && e.s < block * 2) ++quickOns;
    CHECK (quickOns >= 3, "turning the dial re-strikes the chord straight away (%d notes)", quickOns);

    // GATE 1/8 chops the chords: many more hits
    if (auto* g8 = p.apvts.getParameter ("gate")) g8->setValueNotifyingHost (g8->convertTo0to1 (2.0f));
    pump(); head.ppq = 0.0;
    auto gated = run ((int) (spb * 16 / block));
    int gatedOns = 0; for (auto& e : gated) if (e.on) ++gatedOns;
    CHECK (gatedOns > ons * 2, "GATE 1/8 makes more hits (%d vs %d)", gatedOns, ons);
    if (auto* g8 = p.apvts.getParameter ("gate")) g8->setValueNotifyingHost (0.0f);
    pump();

    // back to ORIGINAL: the clip passes through again
    p.select (0); pump();
    head.ppq = 0.0;
    out = run ((int) (spb * 3.5 / block));
    std::set<int> origNotes; for (auto& e : out) if (e.on) origNotes.insert (e.note);
    { juce::String s; for (int x : origNotes) s << x << " "; CHECK (origNotes == std::set<int> ({ 57, 60, 64, 69 }), "original bar 1 = Am again (got %s)", s.toRawUTF8()); }

    // KEYS mode: D3 plays chord 2 of the selected progression, for as long as it's held; no auto-play
    {
        p.select (2); pump();
        if (auto* k = p.apvts.getParameter ("keys")) k->setValueNotifyingHost (1.0f);
        pump();
        head.playing = true; head.ppq = 0.0;
        juce::AudioBuffer<float> buf (2, block);
        auto block1 = [&] (juce::MidiBuffer m) { p.processBlock (buf, m); head.ppq += block / spb; std::vector<Ev> e;
                                                 for (const auto meta : m) if (meta.getMessage().isNoteOnOrOff()) e.push_back ({ (double) meta.samplePosition, meta.getMessage().getNoteNumber(), meta.getMessage().isNoteOn() });
                                                 return e; };
        auto quiet = block1 ({});
        int autoOns = 0; for (int i = 0; i < 50; ++i) for (auto& e : block1 ({})) autoOns += e.on;
        CHECK (autoOns == 0, "KEYS: nothing plays by itself (%d)", autoOns);
        juce::MidiBuffer press; press.addEvent (juce::MidiMessage::noteOn (1, 62, (juce::uint8) 90), 10);
        auto on = block1 (press);
        std::set<int> got; for (auto& e : on) if (e.on) got.insert (e.note);
        const auto& chord2 = p.getVariations()[2].chords[1].notes;
        CHECK (got == std::set<int> (chord2.begin(), chord2.end()), "KEYS: D3 plays chord 2 (%zu notes)", got.size());
        int heldOffs = 0; for (int i = 0; i < 20; ++i) for (auto& e : block1 ({})) heldOffs += ! e.on;
        CHECK (heldOffs == 0 && p.heldChord() == 1, "KEYS: the chord holds while the key is down");
        juce::MidiBuffer rel; rel.addEvent (juce::MidiMessage::noteOff (1, 62), 5);
        int offs = 0; for (auto& e : block1 (rel)) offs += ! e.on;
        CHECK (offs == (int) got.size() && p.heldChord() == -1, "KEYS: releasing the key stops the chord (%d offs)", offs);
        juce::MidiBuffer black; black.addEvent (juce::MidiMessage::noteOn (1, 61, (juce::uint8) 90), 0);
        int blackOns = 0; for (auto& e : block1 (black)) blackOns += e.on;
        CHECK (blackOns == 0, "KEYS: black keys are silent");
        CHECK (MidiGenProcessor::chordForKey (60, 4) == 0 && MidiGenProcessor::chordForKey (65, 4) == 3 && MidiGenProcessor::chordForKey (67, 4) == 0
               && MidiGenProcessor::chordForKey (59, 4) == 3, "KEYS: C3 D3 E3 F3 = 1 2 3 4, then it repeats");
        if (auto* k = p.apvts.getParameter ("keys")) k->setValueNotifyingHost (0.0f);
        pump();
        std::printf ("KEYS: D3 played %zu notes\n", got.size());
    }

    // tone section doesn't blow up: run the built-in sound with everything turned
    for (auto id : { "hp", "lp", "res", "mbLow", "mbHigh", "squash" })
        if (auto* prm = p.apvts.getParameter (id)) prm->setValueNotifyingHost (juce::String (id) == "lp" ? 0.4f : 0.8f);
    p.select (MidiGenProcessor::kDial); pump();
    {
        head.ppq = 0.0;
        juce::AudioBuffer<float> buf (2, block); float peak = 0; bool finite = true;
        for (int b = 0; b < 200; ++b)
        {
            juce::MidiBuffer m; p.processBlock (buf, m); head.ppq += block / spb;
            for (int c = 0; c < 2; ++c) for (int i = 0; i < block; ++i) { const float x = buf.getSample (c, i); finite = finite && std::isfinite (x); peak = std::max (peak, std::abs (x)); }
        }
        CHECK (finite && peak > 0.01f && peak < 4.0f, "tone section output is sane (peak %.3f)", peak);
    }

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
    CHECK (all.getSize() > f.getSize() * 2, "export all saved");
    CHECK (! p.writeMidiFile (6).existsAsFile(), "an empty slot exports nothing");
    juce::MemoryBlock st; p.getStateInformation (st);
    MidiGenProcessor q; q.setStateInformation (st.getData(), (int) st.getSize());
    bool same = q.getVariations().size() == p.getVariations().size();
    for (int i = 1; same && i <= 3; ++i)
        same = q.getVariations()[(size_t) i].chords.size() == p.getVariations()[(size_t) i].chords.size()
            && q.getVariations()[(size_t) i].chords[0].name == p.getVariations()[(size_t) i].chords[0].name;
    CHECK (same && q.slotUsed (3) && ! q.slotUsed (4), "saved slots come back with the project");
    q.clearSlot (2);
    CHECK (! q.slotUsed (2), "clear a slot");

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
