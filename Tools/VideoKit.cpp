// Renders MIDI GEN's real screen + real sound for promo videos.
// VideoKit <out.wav> <seconds> <bpm> [actions...] > frames.bgra
// actions:  T:param=value   T:gen   T:select=N
// Starts having learned Am F C G (one bar each).
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <cstdio>
#include <cstdlib>
#include <vector>

struct Head : juce::AudioPlayHead
{
    double ppq = 0, bpm = 120;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo p; p.setBpm (bpm); p.setPpqPosition (ppq); p.setIsPlaying (true);
        p.setTimeSignature (TimeSignature { 4, 4 });
        return p;
    }
};
struct Action { double t; juce::String key; float v = 0; bool done = false; };

static void setParam (MidiGenProcessor& p, const juce::String& id, float v)
{
    if (auto* prm = p.apvts.getParameter (id)) prm->setValueNotifyingHost (prm->convertTo0to1 (v));
    else std::fprintf (stderr, "no param %s\n", id.toRawUTF8());
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    if (argc < 4) return 1;
    const juce::File wav { juce::String (argv[1]) };
    const double seconds = juce::String (argv[2]).getDoubleValue();
    const double sr = 48000.0; const int fps = 30; const int block = (int) sr / fps;
    std::vector<Action> acts;
    for (int i = 4; i < argc; ++i)
    {
        juce::String s (argv[i]); Action a;
        a.t = s.upToFirstOccurrenceOf (":", false, false).getDoubleValue();
        auto rest = s.fromFirstOccurrenceOf (":", false, false);
        a.key = rest.upToFirstOccurrenceOf ("=", false, false);
        a.v = rest.fromFirstOccurrenceOf ("=", false, false).getFloatValue();
        acts.push_back (a);
    }

    // the source progression: Am F C G
    juce::MidiMessageSequence seq;
    const int chords[4][3] = { { 57, 60, 64 }, { 53, 57, 60 }, { 60, 64, 67 }, { 55, 59, 62 } };
    for (int c = 0; c < 4; ++c)
        for (int n : chords[c])
        {
            seq.addEvent (juce::MidiMessage::noteOn (1, n, (juce::uint8) 96), c * 3840.0);
            seq.addEvent (juce::MidiMessage::noteOff (1, n), c * 3840.0 + 3700.0);
        }
    seq.updateMatchedPairs();
    juce::MidiFile mf; mf.setTicksPerQuarterNote (960);
    juce::MidiMessageSequence meta; meta.addEvent (juce::MidiMessage::textMetaEvent (3, "Chords"), 0);
    seq.addSequence (meta, 0);
    mf.addTrack (seq);
    auto tmp = juce::File::createTempFile (".mid");
    { juce::FileOutputStream os (tmp); mf.writeTo (os); }

    MidiGenProcessor p;
    Head head; head.bpm = juce::String (argv[3]).getDoubleValue();
    p.setPlayHead (&head);
    p.setPlayConfigDetails (0, 2, sr, block);
    p.prepareToPlay (sr, block);
    for (auto& a : acts) if (a.t <= 0 && a.key != "gen" && a.key != "select") { setParam (p, a.key, a.v); a.done = true; }
    juce::String err;
    if (! p.learnFromFile (tmp, err)) { std::fprintf (stderr, "learn failed: %s\n", err.toRawUTF8()); return 2; }
    p.tick();

    std::unique_ptr<juce::AudioProcessorEditor> ed (p.createEditor());
    juce::Component* panel = ed->getChildComponent (0);
    panel->setBounds (0, 0, panel->getWidth(), panel->getHeight());
    auto* mp = dynamic_cast<MidiGenPanel*> (panel);
    std::fprintf (stderr, "panel %dx%d\n", panel->getWidth(), panel->getHeight());

    const int frames = (int) std::lround (seconds * fps);
    juce::AudioBuffer<float> all (2, frames * block), buf (2, block);
    const float vscale = std::getenv ("VK_SCALE") ? (float) std::atof (std::getenv ("VK_SCALE")) : 1.0f;
    const float scale = 1080.0f / (float) panel->getHeight() * vscale;

    for (int f = 0; f < frames; ++f)
    {
        const double t = (double) f / fps;
        for (auto& a : acts)
        {
            if (a.done || t < a.t) continue;
            if (a.key == "gen") p.generate();
            else if (a.key == "select") p.select ((int) a.v);
            else setParam (p, a.key, a.v);
            a.done = true;
        }
        p.tick();
        buf.clear();
        juce::MidiBuffer midi;
        p.processBlock (buf, midi);
        for (int ch = 0; ch < 2; ++ch) all.copyFrom (ch, f * block, buf, ch, 0, block);
        head.ppq += block * head.bpm / (60.0 * sr);
        if (mp) mp->tick();
        auto img = panel->createComponentSnapshot (panel->getLocalBounds(), true, scale).convertedToFormat (juce::Image::ARGB);
        const juce::Image::BitmapData bd (img, juce::Image::BitmapData::readOnly);
        if (f == 0) std::fprintf (stderr, "frame %dx%d\n", bd.width, bd.height);
        for (int y = 0; y < bd.height; ++y) std::fwrite (bd.getLinePointer (y), 1, (size_t) bd.width * 4, stdout);
    }
    std::fflush (stdout);
    wav.deleteFile();
    juce::WavAudioFormat fmt;
    std::unique_ptr<juce::AudioFormatWriter> w (fmt.createWriterFor (new juce::FileOutputStream (wav), sr, 2, 24, {}, 0));
    w->writeFromAudioSampleBuffer (all, 0, all.getNumSamples());
    tmp.deleteFile();
    return 0;
}
