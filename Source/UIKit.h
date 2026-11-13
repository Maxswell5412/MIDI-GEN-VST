#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

// Hardware rack-unit look shared with FLIPSIDE: brushed gunmetal, screws, LED-ring knobs,
// amber phosphor screens and LCD selectors.
namespace ui
{
namespace Col
{
    const juce::Colour chassisTop { 0xff2c2f34 };
    const juce::Colour chassisBot { 0xff17181b };
    const juce::Colour ear        { 0xff1e2024 };
    const juce::Colour recess     { 0xff121316 };
    const juce::Colour plateTop   { 0xff30333a };
    const juce::Colour plateBot   { 0xff222429 };
    const juce::Colour engrave    { 0xffc3c6cc };
    const juce::Colour dim        { 0xff7d828c };
    const juce::Colour amber      { 0xffffa21f };
    const juce::Colour amberDim   { 0xff3b2709 };
    const juce::Colour cyan       { 0xff3ee3ff };
    const juce::Colour red        { 0xffff3b3b };
    const juce::Colour green      { 0xff4cff6a };
    const juce::Colour violet     { 0xffc77dff };
    const juce::Colour lcd        { 0xff0a0b08 };
}

struct Fonts
{
    static juce::Font logo (float h);
    static juce::Font label (float h);
    static juce::Font body (float h);
    static juce::Font mono (float h);
};

void brushed (juce::Graphics&, juce::Rectangle<float>, juce::Colour top, juce::Colour bot, int seed);
void screw (juce::Graphics&, juce::Point<float> c, float r, float angle);
void recess (juce::Graphics&, juce::Rectangle<float>, float radius);
void plate (juce::Graphics&, juce::Rectangle<float>, float radius);
void engraved (juce::Graphics&, const juce::String&, juce::Rectangle<float>, const juce::Font&,
               juce::Justification, juce::Colour c = Col::engrave);
void glowText (juce::Graphics&, const juce::String&, juce::Rectangle<float>, const juce::Font&,
               juce::Colour, juce::Justification);
void lcdBox (juce::Graphics&, juce::Rectangle<float>);
void led (juce::Graphics&, juce::Point<float> c, float r, juce::Colour, bool on);

class Look : public juce::LookAndFeel_V4
{
public:
    Look();
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override;
    void drawComboBox (juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
    juce::Font getPopupMenuFont() override;
    void drawPopupMenuBackground (juce::Graphics&, int, int) override;
    juce::Font getSliderPopupFont (juce::Slider&) override;
};

class Knob : public juce::Slider
{
public:
    Knob();
    void setMod (float v);
    void setBipolar (bool b) { getProperties().set ("bipolar", b); }
};

class LedButton : public juce::TextButton
{
public:
    LedButton (const juce::String& buttonText, juce::Colour led = Col::amber, bool hasLed = true);
};

class HostedWindow : public juce::DocumentWindow
{
public:
    HostedWindow (juce::AudioPluginInstance&, std::function<void()> onClose);
    void closeButtonPressed() override { if (onClose) onClose(); }
private:
    std::function<void()> onClose;
};
} // namespace ui
