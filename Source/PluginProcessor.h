#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Reharm.h"
#include "SketchSynth.h"
#include "ToneFX.h"
#include <vector>

// -----------------------------------------------------------------------------
// MIDI GEN by MB Audio
// Put it on a track that has a chord clip (e.g. dragged from HOOKSMITH), press
// LISTEN and play: it learns the progression. Turn the IDEA dial: every click is a
// new progression in the same key and rhythm, playing straight away. SAVE keeps the
// ones you like in slots A..H (drag them into the DAW). GENERATE = a fresh dial.
// -----------------------------------------------------------------------------
class MidiGenProcessor : public juce::AudioProcessor,
                         public juce::ChangeBroadcaster,
                         private juce::Timer
{
public:
    MidiGenProcessor();
    ~MidiGenProcessor() override;

    void prepareToPlay (double, int) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout&) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override                      { return true; }
    const juce::String getName() const override          { return "MIDI GEN"; }
    bool acceptsMidi() const override                    { return true; }
    bool producesMidi() const override                   { return true; }
    bool isMidiEffect() const override                   { return false; }
    double getTailLengthSeconds() const override         { return 1.0; }
    int getNumPrograms() override                        { return 1; }
    int getCurrentProgram() override                     { return 0; }
    void setCurrentProgram (int) override                {}
    const juce::String getProgramName (int) override     { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    // ---- Learning --------------------------------------------------------------
    void startListening();
    void stopListening();
    int getListenState() const { return listenState.load(); }   // 0 idle, 1 waiting for bar 1, 2 recording, 3 done
    double getListenProgress() const { return listenProgress.load(); }
    bool learnFromFile (const juce::File&, juce::String& error);
    bool hasSource() const { return source != nullptr && ! source->spans.empty(); }
    const hs::ImportedProgression* getSource() const { return source.get(); }

    // ---- Variations (message thread) ---------------------------------------------
    static constexpr int kSlots = 8, kDial = kSlots + 1;   // takes: 0 original, 1..8 saved slots A..H, 9 the IDEA dial
    static constexpr int kIdeas = 100;
    void generate();                                    // new seed = a whole new set of ideas on the dial
    // [0] = ORIGINAL, [1..8] = saved slots (empty chords = free), [9] = the idea on the dial
    const std::vector<hs::Variation>& getVariations() const { return variations; }
    int save();                                         // keeps the dial's idea in the next free slot, returns it (-1 = full)
    void clearSlot (int index);                         // 1..8
    bool slotUsed (int index) const { return index >= 1 && index <= kSlots && ! saved[(size_t) index - 1].v.chords.empty(); }
    int getIdea() const;
    void setIdea (int idea);
    bool isSyncedToHost() const { return hostSynced.load(); }
    // KEYS mode: your keyboard plays the chords (white keys from C3 = chord 1, 2, 3...)
    bool keysMode() const { return pKeys->load() > 0.5f; }
    int heldChord() const { return heldChordIdx.load(); }      // chord being held, -1 = none
    static int chordForKey (int note, int numChords);           // -1 = not a chord key
    static juce::String keyName (int chordIndex);               // "C3", "D3"...
    juce::uint32 getVersion() const { return version; }
    void select (int index);
    int getSelected() const;
    juce::File writeMidiFile (int index);               // -1 = all variations back to back
    void audition (int index);                          // select + play without the DAW

    // ---- Hosted instrument ----------------------------------------------------------
    bool loadInstrumentFromFile (const juce::File&, juce::String& error);
    void unloadInstrument();
    juce::AudioPluginInstance* getInstrument() const { return instrument.get(); }

    void poll() { timerCallback(); }
    void tick() { timerCallback(); }
    // Built-in sound: menu index 0 = Auto (the style's sound)
    static const std::vector<int>& patchList();
    int builtInPatch() const;   // for the tests (normally the timer does this)

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<double> displayPos { -1.0 };
    std::atomic<double> displayBpm { 120.0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void timerCallback() override;
    void rebuild();
    hs::GenSettings currentSettings() const;
    void setSource (std::shared_ptr<hs::ImportedProgression>);
    void killNotes (juce::MidiBuffer& toInstrument, int pos);

    struct PlayData
    {
        struct Ev { double start, end; int pitch, vel; };
        double length = 16;
        std::vector<std::vector<Ev>> takes;   // [0] original, [1..8] saved, [9] dial
        std::vector<juce::uint64> hashes;     // per take: did its notes change? (re-strike the chord)
        std::vector<std::vector<std::vector<int>>> chords;   // per take: the voiced chords, for KEYS mode
    };
    struct Saved { hs::Variation v; std::vector<hs::NoteEv> notes; double length = 16; };
    std::array<Saved, kSlots> saved;
    std::vector<float> settingsKey() const;
    std::vector<float> lastKey;
    static juce::String encode (const Saved&);
    static bool decode (const juce::String&, Saved&);

    std::shared_ptr<hs::ImportedProgression> source;
    std::vector<hs::Variation> variations;
    juce::uint32 version = 0, seed = 1;
    hs::GenSettings lastSettings;
    bool haveLast = false, dirty = true;

    juce::SpinLock swapLock;
    std::unique_ptr<PlayData> pending, current, retired;

    // listening (audio thread writes, message thread reads after it's done)
    std::atomic<int> listenState { 0 };
    std::atomic<double> listenProgress { 0.0 };
    std::array<hs::NoteEv, 4096> recNotes {};
    int recCount = 0;
    std::array<double, 128> recOpen {};
    double recStart = 0.0, lastLoopPos = -1.0;

    // playback
    double sampleRateHz = 44100.0;
    bool prepared = false, wasRunning = false;
    double internalClock = 0.0, lastClock = -1.0;
    bool internalWasOn = false;
    std::array<double, 128> activeOff {};
    int lastTake = -1;
    const PlayData* lastData = nullptr;
    juce::uint64 lastHash = 0;
    bool chasePending = false, lastWasHost = false;
    double sinceChase = 1.0;            // seconds since the last re-strike
    std::atomic<bool> hostSynced { false };
    std::atomic<int> heldChordIdx { -1 };
    std::array<std::vector<int>, 128> keyHeld;     // KEYS mode: which notes each key is holding
    std::array<int, 128> keyCount {};              // how many held keys hold each note
    bool wasKeys = false;
    void releaseKeys (juce::MidiBuffer& dest, int pos);
    int chunkStartNow = 0;
    bool sendOutNow = false;
    void chase (const PlayData&, int take, double c0, juce::MidiBuffer& dest, int chunkStart, bool midiOutOn, int sound);

    std::atomic<float> *pStyle, *pSim, *pColor, *pSpice, *pBass, *pAddBass, *pRhythm, *pSel, *pBars,
                       *pSound, *pVol, *pMidiOut, *pPlay, *pBpm,
                       *pFlavor, *pIdea, *pGate, *pLength, *pA, *pD, *pS, *pR, *pHp, *pLp, *pRes, *pLow, *pMid, *pHigh, *pSquash, *pKeys;
    mg::ToneFX tone;

    hs::SketchSynth keys;
    juce::AudioBuffer<float> chunkBuf;
    juce::MidiBuffer toInst, outMidi;

    juce::AudioPluginFormatManager formatManager;
    std::unique_ptr<juce::AudioPluginInstance> instrument;
    juce::PluginDescription instrumentDesc;
    juce::CriticalSection instLock;
    juce::AudioBuffer<float> instBuf;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiGenProcessor)
};
