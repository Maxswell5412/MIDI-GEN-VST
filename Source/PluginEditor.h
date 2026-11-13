#pragma once

#include "PluginProcessor.h"
#include "UIKit.h"

namespace mg
{
// One progression card: ORIGINAL or A..H. Click = play it, drag = drop it into the DAW.
class Card : public juce::Component, public juce::SettableTooltipClient
{
public:
    Card (MidiGenProcessor& p, int i) : proc (p), index (i) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    double playPos = -1.0;
private:
    MidiGenProcessor& proc;
    const int index;
    bool dragging = false;
};

class DragAllTile : public juce::Component, public juce::SettableTooltipClient
{
public:
    explicit DragAllTile (MidiGenProcessor& p) : proc (p) { setMouseCursor (juce::MouseCursor::DraggingHandCursor); }
    void paint (juce::Graphics&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override { dragging = false; }
private:
    MidiGenProcessor& proc;
    bool dragging = false;
};
} // namespace mg

class MidiGenPanel : public juce::Component,
                     public juce::DragAndDropContainer,
                     public juce::FileDragAndDropTarget,
                     private juce::ChangeListener,
                     private juce::Timer
{
public:
    void tick() { timerCallback(); }   // one UI update (video renderer)
    explicit MidiGenPanel (MidiGenProcessor&);
    ~MidiGenPanel() override;
    static constexpr int designW = 1100, designH = 730;

    void mouseDown (const juce::MouseEvent&) override;
    void paint (juce::Graphics&) override;
    void resized() override;
    bool isInterestedInFileDrag (const juce::StringArray&) override;
    void filesDropped (const juce::StringArray&, int, int) override;

private:
    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    void renderBackground();
    void setStatus (const juce::String& s) { statusText = s; statusTicks = 90; repaint(); }
    void setChoice (const juce::String& id, int index);
    void showWindow (bool show);

    MidiGenProcessor& proc;
    ui::Look look;
    juce::TooltipWindow tooltips { this, 500 };
    juce::Image background;
    juce::Rectangle<int> headerArea, controlArea, cardArea, bottomArea, lcdArea, instLcd;
    int shownPatch = -1;

    ui::LedButton listenButton { "Listen", ui::Col::green };
    juce::OwnedArray<ui::LedButton> barButtons;
    ui::LedButton generateButton { "Generate", ui::Col::amber, false };
    ui::LedButton loadMidiButton { "Load MIDI", ui::Col::amber, false };

    juce::ComboBox style;
    ui::Knob similarity, color, spice, bassMotion;
    ui::LedButton rhythmButton { "Keep Rhythm", ui::Col::amber };
    ui::LedButton bassButton { "Add Bass", ui::Col::amber };

    juce::OwnedArray<mg::Card> cards;

    ui::LedButton loadButton { "Load", ui::Col::amber, false };
    ui::LedButton showButton { "Show", ui::Col::green };
    ui::LedButton ejectButton { "X", ui::Col::amber, false };
    juce::ComboBox sound;
    ui::Knob vol, bpm;
    ui::LedButton midiOutButton { "MIDI Out", ui::Col::green };
    ui::LedButton playButton { "Play", ui::Col::green };
    mg::DragAllTile dragAll;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using CA = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<CA> styleA, soundA;
    std::unique_ptr<SA> simA, colorA, spiceA, bassA, volA, bpmA;
    std::unique_ptr<BA> rhythmA, addBassA, midiOutA, playA;

    std::unique_ptr<ui::HostedWindow> window;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::String statusText;
    int statusTicks = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiGenPanel)
};

class MidiGenEditor : public juce::AudioProcessorEditor
{
public:
    explicit MidiGenEditor (MidiGenProcessor&);
    void resized() override;
private:
    MidiGenPanel panel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MidiGenEditor)
};
