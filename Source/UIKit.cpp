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

// Console knob: a Neve-style fluted charcoal skirt with a printed scale around it,
// topped with an SSL-style colour cap and a white pointer line.
void Look::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                             float start, float end, juce::Slider& s)
{
    const float size = (float) juce::jmin (w, h);
    const auto c = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).getCentre();
    const bool big = size >= 90.0f;

    const float scaleR = size * 0.5f - 1.0f;
    const float skirtR = scaleR - juce::jmax (5.0f, size * 0.12f);
    const float capR   = skirtR * (big ? 0.62f : 0.68f);
    const bool bipolar = (bool) s.getProperties().getWithDefault ("bipolar", false);
    const int steps = (int) s.getProperties().getWithDefault ("steps", 0);
    const auto capCol = juce::Colour ((juce::uint32) (juce::int64) s.getProperties().getWithDefault ("cap", (juce::int64) Col::capGrey.getARGB()));
    const float ang = start + pos * (end - start);

    // --- printed scale (Neve): ticks, a longer one at the ends / centre, the value lit in amber
    const int ticks = steps > 1 ? steps : (big ? 31 : 11);
    for (int i = 0; i < ticks; ++i)
    {
        const float t = (float) i / (float) (ticks - 1);
        const float a = start + t * (end - start);
        const bool major = steps > 1 || i == 0 || i == ticks - 1 || (bipolar && i == ticks / 2) || (big ? i % 5 == 0 : i % 5 == 0);
        bool lit = bipolar ? (t >= juce::jmin (0.5f, pos) - 0.001f && t <= juce::jmax (0.5f, pos) + 0.001f)
                           : (t <= pos + 0.001f && pos > 0.001f);
        if (steps > 1) lit = std::abs (t - pos) < 0.5f / (float) (ticks - 1);
        const float r0 = scaleR - (major ? juce::jmax (4.0f, size * 0.09f) : juce::jmax (2.5f, size * 0.055f));
        g.setColour (lit ? Col::amber : juce::Colour (0xffd8d4c8).withAlpha (major ? 0.55f : 0.3f));
        g.drawLine (juce::Line<float> (c.getPointOnCircumference (r0, a), c.getPointOnCircumference (scaleR, a)), major ? 1.4f : 1.0f);
    }

    // --- skirt: shadow, fluted charcoal body (Neve / Marconi)
    g.setColour (juce::Colours::black.withAlpha (0.65f));
    g.fillEllipse (juce::Rectangle<float> (skirtR * 2.0f + 3.0f, skirtR * 2.0f + 3.0f).withCentre (c.translated (0.0f, 2.5f)));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff4a505a), c.translated (-skirtR * 0.4f, -skirtR),
                                             juce::Colour (0xff15171b), c.translated (skirtR * 0.3f, skirtR), false));
    g.fillEllipse (juce::Rectangle<float> (skirtR * 2.0f, skirtR * 2.0f).withCentre (c));
    const int flutes = big ? 28 : 16;
    for (int i = 0; i < flutes; ++i)
    {
        const float a = ang + juce::MathConstants<float>::twoPi * (float) i / (float) flutes;   // the flutes turn with the knob
        const float lightness = 0.5f + 0.5f * std::cos (a + 0.9f);
        g.setColour (juce::Colours::white.withAlpha (0.03f + 0.07f * lightness));
        g.drawLine (juce::Line<float> (c.getPointOnCircumference (capR + 1.5f, a), c.getPointOnCircumference (skirtR - 1.0f, a)), big ? 2.2f : 1.6f);
        g.setColour (juce::Colours::black.withAlpha (0.35f));
        g.drawLine (juce::Line<float> (c.getPointOnCircumference (capR + 1.5f, a + 0.06f), c.getPointOnCircumference (skirtR - 1.0f, a + 0.06f)), 0.8f);
    }
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawEllipse (juce::Rectangle<float> (skirtR * 2.0f, skirtR * 2.0f).withCentre (c), 1.0f);
    // skirt pointer (Neve white line on the skirt)
    g.setColour (juce::Colour (0xfff2efe6).withAlpha (0.85f));
    g.drawLine (juce::Line<float> (c.getPointOnCircumference (capR + 1.0f, ang), c.getPointOnCircumference (skirtR - 1.5f, ang)), big ? 2.4f : 1.8f);

    // --- SSL colour cap: domed plastic with a white line
    const auto capBox = juce::Rectangle<float> (capR * 2.0f, capR * 2.0f).withCentre (c);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillEllipse (capBox.translated (0.0f, 1.2f));
    juce::ColourGradient cg (capCol.brighter (0.45f), c.translated (-capR * 0.45f, -capR * 0.55f),
                             capCol.darker (0.55f), c.translated (capR * 0.55f, capR * 0.75f), true);
    g.setGradientFill (cg);
    g.fillEllipse (capBox);
    g.setColour (juce::Colours::white.withAlpha (0.22f));
    g.fillEllipse (juce::Rectangle<float> (capR * 0.9f, capR * 0.5f).withCentre (c.translated (-capR * 0.18f, -capR * 0.45f)));
    g.setColour (juce::Colours::black.withAlpha (0.6f));
    g.drawEllipse (capBox, 0.9f);
    const bool lightCap = capCol.getPerceivedBrightness() > 0.7f;
    g.setColour (lightCap ? juce::Colour (0xff1b1c1f) : juce::Colour (0xfff7f5ef));
    g.drawLine (juce::Line<float> (c.getPointOnCircumference (capR * 0.15f, ang), c.getPointOnCircumference (capR - 1.5f, ang)), big ? 3.0f : 2.0f);
}

// Console buttons: square SSL key caps, the LED as a lit lens in the cap
void Look::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool over, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (1.0f, 1.0f);
    const bool accent = (bool) b.getProperties().getWithDefault ("accent", false);
    const float radius = 2.5f;

    // bezel
    g.setColour (juce::Colours::black.withAlpha (0.75f));
    g.fillRoundedRectangle (r, radius + 1.0f);
    auto body = r.reduced (2.0f).translated (0.0f, down ? 1.0f : 0.0f);

    juce::Colour top = accent ? juce::Colour (0xffe8842a) : juce::Colour (0xffd9d5cb);   // SSL off-white keys, orange for the big ones
    juce::Colour bot = accent ? juce::Colour (0xff9b4710) : juce::Colour (0xff8f8b82);
    if (! accent && ! (bool) b.getProperties().getWithDefault ("lightKey", false)) { top = juce::Colour (0xff4a4e56); bot = juce::Colour (0xff24262b); }
    if (! b.isEnabled()) { top = top.withMultipliedBrightness (0.55f); bot = bot.withMultipliedBrightness (0.55f); }
    if (over && b.isEnabled()) { top = top.brighter (0.08f); bot = bot.brighter (0.05f); }
    if (down) std::swap (top, bot);
    g.setGradientFill (juce::ColourGradient (top, body.getTopLeft(), bot, body.getBottomLeft(), false));
    g.fillRoundedRectangle (body, radius);
    g.setColour (juce::Colours::white.withAlpha (accent ? 0.35f : 0.14f));
    g.drawHorizontalLine ((int) body.getY() + 1, body.getX() + radius, body.getRight() - radius);
    g.setColour (juce::Colours::black.withAlpha (0.85f));
    g.drawRoundedRectangle (body, radius, 1.0f);

    if ((bool) b.getProperties().getWithDefault ("led", false))
    {
        const auto col = juce::Colour ((juce::uint32) (juce::int64) b.getProperties().getWithDefault ("ledColour", (juce::int64) Col::amber.getARGB()));
        const bool on = (b.getToggleState() || (bool) b.getProperties().getWithDefault ("ledForce", false)) && b.isEnabled();
        // a lens strip on the left of the key, lit when on
        auto lens = juce::Rectangle<float> (6.0f, body.getHeight() - 8.0f).withCentre ({ body.getX() + 9.0f, body.getCentreY() });
        g.setColour (juce::Colours::black.withAlpha (0.7f));
        g.fillRoundedRectangle (lens.expanded (1.0f), 2.0f);
        g.setColour (on ? col : col.withMultipliedBrightness (0.22f).withMultipliedSaturation (0.6f));
        g.fillRoundedRectangle (lens, 1.5f);
        if (on)
        {
            g.setColour (col.withAlpha (0.25f));
            g.fillRoundedRectangle (lens.expanded (3.0f), 3.0f);
            g.setColour (juce::Colours::white.withAlpha (0.5f));
            g.fillRoundedRectangle (lens.withHeight (lens.getHeight() * 0.35f).reduced (1.0f, 0.5f), 1.0f);
        }
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
