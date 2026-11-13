#include "UIKit.h"
#include "BinaryData.h"

// Shared "hardware rack" look, the same as FLIPSIDE, so the two plugins feel like one product line.
namespace ui
{
// ---------------------------------------------------------------------------
// Drawing helpers
// ---------------------------------------------------------------------------
void brushed (juce::Graphics& g, juce::Rectangle<float> r, juce::Colour top, juce::Colour bot, int seed)
{
    g.setGradientFill (juce::ColourGradient (top, r.getTopLeft(), bot, r.getBottomLeft(), false));
    g.fillRect (r);

    juce::Random rnd (seed);
    for (float yy = r.getY(); yy < r.getBottom(); yy += 0.5f)
    {
        const float a = rnd.nextFloat() * 0.045f;
        g.setColour ((rnd.nextBool() ? juce::Colours::white : juce::Colours::black).withAlpha (a));
        const float x0 = r.getX() + rnd.nextFloat() * r.getWidth() * 0.4f;
        const float x1 = r.getRight() - rnd.nextFloat() * r.getWidth() * 0.4f;
        g.fillRect (x0, yy, x1 - x0, 0.5f);
    }
}

void screw (juce::Graphics& g, juce::Point<float> c, float r, float angle)
{
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (juce::Rectangle<float> (r * 2.0f + 2.0f, r * 2.0f + 2.0f).withCentre (c.translated (0.0f, 1.0f)));
    juce::ColourGradient grad (juce::Colour (0xffb9bdc4), c.translated (-r * 0.6f, -r * 0.6f),
                               juce::Colour (0xff4a4e55), c.translated (r * 0.7f, r * 0.7f), true);
    g.setGradientFill (grad);
    g.fillEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c));
    g.setColour (juce::Colour (0xff2a2c30));
    g.drawEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c), 0.8f);

    juce::Path slot;
    slot.addRoundedRectangle (-r * 0.75f, -r * 0.14f, r * 1.5f, r * 0.28f, r * 0.1f);
    g.setColour (juce::Colour (0xff25272b));
    g.fillPath (slot, juce::AffineTransform::rotation (angle).translated (c));
}

void recess (juce::Graphics& g, juce::Rectangle<float> r, float radius)
{
    // outer bevel highlight (light catches the lower lip)
    g.setColour (juce::Colours::white.withAlpha (0.06f));
    g.drawRoundedRectangle (r.translated (0.0f, 1.0f), radius, 1.0f);

    g.setColour (Col::recess);
    g.fillRoundedRectangle (r, radius);

    // inner shadow along the top edge
    juce::Path p;
    p.addRoundedRectangle (r, radius);
    g.saveState();
    g.reduceClipRegion (p);
    g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.7f), r.getX(), r.getY(),
                                             juce::Colours::transparentBlack, r.getX(), r.getY() + 10.0f, false));
    g.fillRect (r.withHeight (10.0f));
    g.restoreState();

    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawRoundedRectangle (r, radius, 1.0f);
}

void plate (juce::Graphics& g, juce::Rectangle<float> r, float radius)
{
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillRoundedRectangle (r.translated (0.0f, 2.0f), radius);
    g.setGradientFill (juce::ColourGradient (Col::plateTop, r.getTopLeft(), Col::plateBot, r.getBottomLeft(), false));
    g.fillRoundedRectangle (r, radius);
    g.setColour (juce::Colours::white.withAlpha (0.09f));
    g.drawHorizontalLine ((int) r.getY() + 1, r.getX() + radius, r.getRight() - radius);
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawRoundedRectangle (r, radius, 1.0f);
}

void engraved (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> r, const juce::Font& f,
               juce::Justification j, juce::Colour c)
{
    g.setFont (f);
    g.setColour (juce::Colours::black.withAlpha (0.75f));
    g.drawText (text, r.translated (0.0f, 1.0f), j, false);
    g.setColour (c);
    g.drawText (text, r, j, false);
}

void glowText (juce::Graphics& g, const juce::String& text, juce::Rectangle<float> r, const juce::Font& f,
               juce::Colour c, juce::Justification j)
{
    g.setFont (f);
    g.setColour (c.withAlpha (0.12f));
    for (auto off : { juce::Point<float> (-1.5f, 0), { 1.5f, 0 }, { 0, -1.5f }, { 0, 1.5f } })
        g.drawText (text, r.translated (off.x, off.y), j, false);
    g.setColour (c);
    g.drawText (text, r, j, false);
}

void lcdBox (juce::Graphics& g, juce::Rectangle<float> r)
{
    g.setColour (juce::Colours::white.withAlpha (0.05f));
    g.drawRoundedRectangle (r.translated (0.0f, 1.0f), 4.0f, 1.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff141109), r.getTopLeft(),
                                             Col::lcd, r.getBottomLeft(), false));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (juce::Colours::black);
    g.drawRoundedRectangle (r, 4.0f, 1.2f);
    g.setColour (Col::amber.withAlpha (0.05f));
    g.fillRoundedRectangle (r.reduced (2.0f), 3.0f);
}

void led (juce::Graphics& g, juce::Point<float> c, float r, juce::Colour col, bool on)
{
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.fillEllipse (juce::Rectangle<float> (r * 2.0f + 2.0f, r * 2.0f + 2.0f).withCentre (c));
    if (on)
    {
        g.setColour (col.withAlpha (0.25f));
        g.fillEllipse (juce::Rectangle<float> (r * 4.0f, r * 4.0f).withCentre (c));
        g.setGradientFill (juce::ColourGradient (col.brighter (0.8f), c.translated (-r * 0.3f, -r * 0.3f),
                                                 col, c.translated (r, r), true));
    }
    else
    {
        g.setColour (col.withMultipliedBrightness (0.22f).withMultipliedSaturation (0.7f));
    }
    g.fillEllipse (juce::Rectangle<float> (r * 2.0f, r * 2.0f).withCentre (c));
}

// =============================================================================
// Fonts
// =============================================================================
namespace
{
juce::Typeface::Ptr tf (const char* data, int size)
{
    return juce::Typeface::createSystemTypefaceFor (data, (size_t) size);
}
juce::Typeface::Ptr logoTf()  { static auto t = tf (BinaryData::MichromaRegular_ttf,      BinaryData::MichromaRegular_ttfSize);      return t; }
juce::Typeface::Ptr boldTf()  { static auto t = tf (BinaryData::RajdhaniBold_ttf,         BinaryData::RajdhaniBold_ttfSize);         return t; }
juce::Typeface::Ptr semiTf()  { static auto t = tf (BinaryData::RajdhaniSemiBold_ttf,     BinaryData::RajdhaniSemiBold_ttfSize);     return t; }
juce::Typeface::Ptr monoTf()  { static auto t = tf (BinaryData::ShareTechMonoRegular_ttf, BinaryData::ShareTechMonoRegular_ttfSize); return t; }
}

juce::Font Fonts::logo (float h)  { return juce::Font (juce::FontOptions (logoTf()).withHeight (h)); }
juce::Font Fonts::label (float h) { return juce::Font (juce::FontOptions (boldTf()).withHeight (h)); }
juce::Font Fonts::body (float h)  { return juce::Font (juce::FontOptions (semiTf()).withHeight (h)); }
juce::Font Fonts::mono (float h)  { return juce::Font (juce::FontOptions (monoTf()).withHeight (h)); }

// =============================================================================
// Look
// =============================================================================
Look::Look()
{
    setDefaultSansSerifTypeface (semiTf());

    setColour (juce::ResizableWindow::backgroundColourId, Col::chassisBot);
    setColour (juce::ComboBox::textColourId, Col::amber);
    setColour (juce::ComboBox::backgroundColourId, Col::lcd);
    setColour (juce::ComboBox::arrowColourId, Col::amber);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff141518));
    setColour (juce::PopupMenu::textColourId, Col::engrave);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, Col::amber.withAlpha (0.25f));
    setColour (juce::PopupMenu::highlightedTextColourId, Col::amber);
    setColour (juce::TooltipWindow::backgroundColourId, juce::Colour (0xff0d0e10));
    setColour (juce::TooltipWindow::textColourId, Col::amber);
    setColour (juce::TooltipWindow::outlineColourId, Col::amber.withAlpha (0.4f));
    setColour (juce::BubbleComponent::backgroundColourId, juce::Colour (0xff0d0e10));
    setColour (juce::BubbleComponent::outlineColourId, Col::amber.withAlpha (0.5f));
    setColour (juce::TextButton::textColourOffId, Col::engrave);
    setColour (juce::TextButton::textColourOnId, Col::engrave);
}

void Look::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                             float start, float end, juce::Slider& s)
{
    const float size = (float) juce::jmin (w, h);
    const auto c = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).getCentre();
    const bool big = size >= 70.0f;

    const float ringR = size * 0.5f - 3.0f;
    const float bodyR = ringR - (big ? size * 0.14f : size * 0.16f);
    const float capR  = bodyR * 0.74f;

    // --- LED ring -------------------------------------------------------------
    const int segs = big ? 27 : 17;
    const float dotR = juce::jlimit (1.6f, 3.0f, size * 0.026f);
    const bool bipolar = (bool) s.getProperties().getWithDefault ("bipolar", false);
    const float modPos = (float) (double) s.getProperties().getWithDefault ("mod", -1.0);
    int modSeg = -1;
    if (modPos >= 0.0f && std::abs (modPos - pos) > 0.01f)
        modSeg = juce::roundToInt (modPos * (float) (segs - 1));

    for (int i = 0; i < segs; ++i)
    {
        const float t = (float) i / (float) (segs - 1);
        const float a = start + t * (end - start);
        const auto pt = c.getPointOnCircumference (ringR - dotR, a);

        bool lit;
        if (bipolar) lit = (t >= juce::jmin (0.5f, pos) - 0.001f) && (t <= juce::jmax (0.5f, pos) + 0.001f);
        else         lit = t <= pos + 0.001f && pos > 0.001f;

        juce::Colour col = lit ? Col::amber : Col::amberDim;
        if (i == modSeg) { col = Col::cyan; lit = true; }

        if (lit)
        {
            g.setColour (col.withAlpha (0.22f));
            g.fillEllipse (juce::Rectangle<float> (dotR * 4.0f, dotR * 4.0f).withCentre (pt));
        }
        g.setColour (col);
        g.fillEllipse (juce::Rectangle<float> (dotR * 2.0f, dotR * 2.0f).withCentre (pt));
    }

    // --- Body: drop shadow, knurled skirt, metal cap ---------------------------
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillEllipse (juce::Rectangle<float> (bodyR * 2.0f + 4.0f, bodyR * 2.0f + 4.0f).withCentre (c.translated (0.0f, 3.0f)));

    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff474b52), c.translated (0.0f, -bodyR),
                                             juce::Colour (0xff17181b), c.translated (0.0f, bodyR), false));
    g.fillEllipse (juce::Rectangle<float> (bodyR * 2.0f, bodyR * 2.0f).withCentre (c));

    const int ridges = big ? 60 : 36;
    for (int i = 0; i < ridges; ++i)
    {
        const float a = juce::MathConstants<float>::twoPi * (float) i / (float) ridges;
        g.setColour ((i % 2 == 0 ? juce::Colours::black : juce::Colours::white).withAlpha (i % 2 == 0 ? 0.35f : 0.06f));
        g.drawLine (juce::Line<float> (c.getPointOnCircumference (capR + 1.0f, a), c.getPointOnCircumference (bodyR - 0.5f, a)), 1.0f);
    }

    juce::ColourGradient capGrad (juce::Colour (0xff7a7f88), c.translated (-capR * 0.5f, -capR * 0.6f),
                                  juce::Colour (0xff25282d), c.translated (capR * 0.6f, capR * 0.8f), true);
    g.setGradientFill (capGrad);
    g.fillEllipse (juce::Rectangle<float> (capR * 2.0f, capR * 2.0f).withCentre (c));
    g.setColour (juce::Colours::black.withAlpha (0.7f));
    g.drawEllipse (juce::Rectangle<float> (capR * 2.0f, capR * 2.0f).withCentre (c), 1.0f);

    // brushed rings on the cap
    for (float rr = capR * 0.25f; rr < capR; rr += capR * 0.18f)
    {
        g.setColour (juce::Colours::white.withAlpha (0.035f));
        g.drawEllipse (juce::Rectangle<float> (rr * 2.0f, rr * 2.0f).withCentre (c), 0.7f);
    }

    // pointer
    const float ang = start + pos * (end - start);
    const auto p0 = c.getPointOnCircumference (capR * 0.25f, ang);
    const auto p1 = c.getPointOnCircumference (bodyR - 2.0f, ang);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.drawLine (juce::Line<float> (p0, p1).withShortenedStart (0.0f), big ? 4.5f : 3.5f);
    g.setColour (juce::Colour (0xfff4f1ea));
    g.drawLine (juce::Line<float> (p0, p1), big ? 2.6f : 2.0f);
}

void Look::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f, 1.0f);
    const bool accent = (bool) b.getProperties().getWithDefault ("accent", false);
    const float radius = 4.0f;

    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.fillRoundedRectangle (r.translated (0.0f, down ? 0.5f : 2.0f), radius);

    juce::Colour top = accent ? juce::Colour (0xffd9751a) : juce::Colour (0xff43474e);
    juce::Colour bot = accent ? juce::Colour (0xff8a3e0c) : juce::Colour (0xff25272c);
    if (! b.isEnabled()) { top = top.withMultipliedBrightness (0.6f); bot = bot.withMultipliedBrightness (0.6f); }
    if (over && b.isEnabled()) { top = top.brighter (0.08f); bot = bot.brighter (0.05f); }
    if (down) std::swap (top, bot);

    const auto body = r.translated (0.0f, down ? 1.0f : 0.0f);
    g.setGradientFill (juce::ColourGradient (top, body.getTopLeft(), bot, body.getBottomLeft(), false));
    g.fillRoundedRectangle (body, radius);

    g.setColour (juce::Colours::white.withAlpha (accent ? 0.3f : 0.12f));
    g.drawHorizontalLine ((int) body.getY() + 1, body.getX() + radius, body.getRight() - radius);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawRoundedRectangle (body, radius, 1.0f);

    if ((bool) b.getProperties().getWithDefault ("led", false))
    {
        const auto col = juce::Colour ((juce::uint32) (juce::int64) b.getProperties().getWithDefault ("ledColour", (juce::int64) Col::amber.getARGB()));
        const bool on = b.getToggleState() || (bool) b.getProperties().getWithDefault ("ledForce", false);
        led (g, { body.getX() + 12.0f, body.getCentreY() }, 3.6f, col, on && b.isEnabled());
    }
}

void Look::drawButtonText (juce::Graphics& g, juce::TextButton& b, bool, bool down)
{
    auto r = b.getLocalBounds().toFloat().translated (0.0f, down ? 1.0f : 0.0f);
    if ((bool) b.getProperties().getWithDefault ("led", false))
        r.removeFromLeft (16.0f);

    const bool accent = (bool) b.getProperties().getWithDefault ("accent", false);
    const auto font = Fonts::label (juce::jmin (16.0f, b.getHeight() * 0.5f));
    auto col = accent ? juce::Colour (0xff1a0d02) : Col::engrave;
    if (! b.isEnabled()) col = col.withAlpha (0.35f);

    g.setFont (font);
    if (! accent)
    {
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.drawText (b.getButtonText().toUpperCase(), r.translated (0.0f, 1.0f), juce::Justification::centred, false);
    }
    g.setColour (col);
    g.drawText (b.getButtonText().toUpperCase(), r, juce::Justification::centred, false);
}

void Look::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    lcdBox (g, juce::Rectangle<float> (0.0f, 0.0f, (float) w, (float) h).reduced (0.5f));
    juce::Path arrow;
    const float ax = (float) w - 13.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4.0f, ay - 2.5f, ax + 4.0f, ay - 2.5f, ax, ay + 3.0f);
    g.setColour (box.isEnabled() ? Col::amber.withAlpha (0.85f) : Col::dim);
    g.fillPath (arrow);
}

juce::Font Look::getComboBoxFont (juce::ComboBox& box)
{
    return Fonts::mono (juce::jmin (16.0f, box.getHeight() * 0.62f));
}

void Look::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (6, 1, box.getWidth() - 24, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

juce::Font Look::getPopupMenuFont()                { return Fonts::body (17.0f); }
juce::Font Look::getSliderPopupFont (juce::Slider&) { return Fonts::mono (15.0f); }

void Look::drawPopupMenuBackground (juce::Graphics& g, int w, int h)
{
    g.fillAll (findColour (juce::PopupMenu::backgroundColourId));
    g.setColour (Col::amber.withAlpha (0.35f));
    g.drawRect (0, 0, w, h, 1);
}

// =============================================================================
// Knob / buttons
// =============================================================================
Knob::Knob()
{
    setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    setPopupDisplayEnabled (true, true, nullptr);
    setMouseDragSensitivity (220);
    setVelocityBasedMode (false);
}

void Knob::setMod (float v)
{
    const float old = (float) (double) getProperties().getWithDefault ("mod", -1.0);
    if (std::abs (old - v) > 0.004f)
    {
        getProperties().set ("mod", v);
        repaint();
    }
}

LedButton::LedButton (const juce::String& buttonText, juce::Colour ledColour, bool hasLed)
    : juce::TextButton (buttonText)
{
    getProperties().set ("led", hasLed);
    getProperties().set ("ledColour", (juce::int64) ledColour.getARGB());
}

// =============================================================================
// HostedWindow
// =============================================================================
HostedWindow::HostedWindow (juce::AudioPluginInstance& plugin, std::function<void()> closeFn)
    : juce::DocumentWindow ("HOOKSMITH  |  " + plugin.getName(), Col::chassisBot, juce::DocumentWindow::closeButton),
      onClose (std::move (closeFn))
{
    setUsingNativeTitleBar (true);
    juce::AudioProcessorEditor* ed = plugin.createEditorIfNeeded();
    if (ed == nullptr)
        ed = new juce::GenericAudioProcessorEditor (plugin);
    setContentOwned (ed, true);
    setResizable (ed->isResizable(), false);
    setAlwaysOnTop (true);
    centreWithSize (getWidth(), getHeight());
    setVisible (true);
}
} // namespace ui
