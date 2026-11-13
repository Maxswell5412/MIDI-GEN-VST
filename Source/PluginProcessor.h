#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "Reharm.h"
#include "SketchSynth.h"

// -----------------------------------------------------------------------------
// MIDI GEN by MB Audio
// Put it on a track that has a chord clip (e.g. dragged from HOOKSMITH), press
// LISTEN and play: it learns the progression. GENERATE writes 8 new progressions
// in the same key and rhythm. Click one and it plays instead of the original.
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
    void generate();                                    // new seed
    const std::vector<hs::Variation>& getVariations() const { return variations; }   // [0] = ORIGINAL
    juce::uint32 getVersion() const { return version; }
    void select (int index);
    int getSelected() const;
    juce::File writeMidiFile (int index);               // -1 = all variations back to back
    void audition (int index);                          // select + play without the DAW

    // ---- Hosted instrument ----------------------------------------------------------
    bool loadInstrumentFromFile (const juce::File&, juce::String& error);
    void unloadInstrument();
    juce::AudioPluginInstance* getInstrument() const { return instrument.get(); }

    void poll() { timerCallback(); }   // for the tests (normally the timer does this)

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
        std::vector<std::vector<Ev>> takes;   // [0] original, [1..8] variations
    };

    std::shared_ptr<hs::ImportedProgression> source;
    std::vector<hs::Variation> variations;
    juce::uint32 version = 0, seed = 1;
    hs::GenSettings lastSettings;
    bool haveLast = false, dirty = true, lastRhythm = true;

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

    std::atomic<float> *pStyle, *pSim, *pColor, *pSpice, *pBass, *pAddBass, *pRhythm, *pSel, *pBars,
                       *pSound, *pVol, *pMidiOut, *pPlay, *pBpm;

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
