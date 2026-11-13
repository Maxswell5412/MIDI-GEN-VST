#include "PluginEditor.h"

using namespace ui;
using namespace mg;

namespace
{
constexpr int kW = MidiGenPanel::designW, kH = MidiGenPanel::designH;
}

// =============================================================================
// Card
// =============================================================================
void Card::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (2.0f);
    const auto& vars = proc.getVariations();
    const bool has = index < (int) vars.size();
    const bool selected = proc.getSelected() == index;

    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (r.translated (0, 2), 7.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c2f35), r.getTopLeft(), juce::Colour (0xff191a1e), r.getBottomLeft(), false));
    g.fillRoundedRectangle (r, 7.0f);
    if (selected)
    {
        g.setColour (Col::amber.withAlpha (0.12f));
        g.fillRoundedRectangle (r, 7.0f);
        g.setColour (Col::amber);
        g.drawRoundedRectangle (r.reduced (0.75f), 7.0f, 2.0f);
    }
    else
    {
        g.setColour (juce::Colours::white.withAlpha (0.08f));
        g.drawRoundedRectangle (r.reduced (0.5f), 7.0f, 1.0f);
    }

    auto inner = r.reduced (12.0f, 8.0f);
    auto top = inner.removeFromTop (22.0f);
    const juce::String label = index == 0 ? "ORIGINAL" : juce::String::charToString ((juce::juce_wchar) ('A' + index - 1));
    glowText (g, label, top.removeFromLeft (index == 0 ? 120.0f : 30.0f), Fonts::logo (index == 0 ? 13.0f : 17.0f),
              selected ? Col::amber : Col::engrave.withAlpha (has ? 1.0f : 0.4f), juce::Justification::centredLeft);
    led (g, { top.getRight() - 6.0f, top.getCentreY() }, 4.0f, index == 0 ? Col::green : Col::amber, selected && playPos >= 0.0);
    top.removeFromRight (16.0f);

    if (! has)
    {
        g.setFont (Fonts::body (14.0f));
        g.setColour (Col::dim);
        g.drawFittedText (index == 0 ? "Press LISTEN and play your chord clip,\nor drop a MIDI file on MIDI GEN"
                                     : "New progressions appear here",
                          inner.toNearestInt(), juce::Justification::centred, 2);
        return;
    }

    const auto& v = vars[(size_t) index];
    juce::String sub;
    if (index == 0 && proc.getSource() != nullptr)
        sub = juce::String (hs::noteName (proc.getSource()->tonicPc, hs::keyPrefersFlats (proc.getSource()->tonicPc, proc.getSource()->minor)))
            + (proc.getSource()->minor ? " MINOR" : " MAJOR");
    else sub = juce::String (v.tags).toUpperCase();
    g.setFont (Fonts::label (11.5f));
    g.setColour (Col::dim.brighter (0.25f));
    g.drawFittedText (sub, top.toNearestInt(), juce::Justification::centredRight, 1, 0.8f);

    // timeline of chord boxes
    inner.removeFromTop (6.0f);
    const double L = proc.getSource() != nullptr ? proc.getSource()->lengthBeats : 16.0;
    lcdBox (g, inner);
    auto strip = inner.reduced (4.0f);
    for (auto& c : v.chords)
    {
        const float x0 = strip.getX() + (float) (c.start / L) * strip.getWidth();
        const float x1 = strip.getX() + (float) ((c.start + c.len) / L) * strip.getWidth();
        auto cell = juce::Rectangle<float> (x0, strip.getY(), x1 - x0, strip.getHeight()).reduced (1.0f, 0.0f);
        const bool now = selected && playPos >= c.start && playPos < c.start + c.len;
        const auto col = c.changed ? Col::cyan : Col::amber;
        g.setColour (col.withAlpha (now ? 0.3f : 0.08f));
        g.fillRoundedRectangle (cell, 3.0f);
        const float fh = juce::jlimit (8.5f, 15.0f, cell.getWidth() / juce::jmax (3.0f, (float) c.name.size() * 0.62f));
        g.setFont (Fonts::mono (fh));
        g.setColour (now ? juce::Colours::white : col);
        g.drawFittedText (c.name, cell.reduced (2.0f, 0.0f).toNearestInt(), juce::Justification::centred, 2, 0.7f);
    }
    if (selected && playPos >= 0.0)
    {
        const float x = strip.getX() + (float) (playPos / L) * strip.getWidth();
        g.setColour (juce::Colour (0xffffe0b0));
        g.fillRect (x - 0.75f, strip.getY(), 1.5f, strip.getHeight());
    }
}

void Card::mouseDown (const juce::MouseEvent&)
{
    dragging = false;
    if (index < (int) proc.getVariations().size()) proc.select (index);
    if (auto* p = getParentComponent()) p->repaint();
}

void Card::mouseDoubleClick (const juce::MouseEvent&)
{
    if (index < (int) proc.getVariations().size()) proc.audition (index);
}

void Card::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 6 || index >= (int) proc.getVariations().size()) return;
    dragging = true;
    const auto f = proc.writeMidiFile (index);
    if (f.existsAsFile()) juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
}

// =============================================================================
void DragAllTile::paint (juce::Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (1.5f);
    g.setColour (juce::Colours::black.withAlpha (0.55f));
    g.fillRoundedRectangle (r.translated (0, 2), 5.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff3a3d44), r.getTopLeft(), juce::Colour (0xff202227), r.getBottomLeft(), false));
    g.fillRoundedRectangle (r, 5.0f);
    g.setColour (Col::amber.withAlpha (0.6f));
    g.drawRoundedRectangle (r.reduced (0.5f), 5.0f, 1.0f);
    g.setFont (Fonts::label (13.0f));
    g.setColour (Col::engrave);
    g.drawFittedText ("DRAG ALL 8", r.toNearestInt(), juce::Justification::centred, 1);
}

void DragAllTile::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 6) return;
    dragging = true;
    const auto f = proc.writeMidiFile (-1);
    if (f.existsAsFile()) juce::DragAndDropContainer::performExternalDragDropOfFiles ({ f.getFullPathName() }, false, this);
}

void DragAllTile::mouseUp (const juce::MouseEvent&)
{
    if (dragging) return;
    const auto f = proc.writeMidiFile (-1);
    if (f.existsAsFile()) f.revealToUser();
}

// =============================================================================
// Panel
// =============================================================================
MidiGenPanel::MidiGenPanel (MidiGenProcessor& p) : proc (p), dragAll (p)
{
    setLookAndFeel (&look);
    tooltips.setLookAndFeel (&look);

    listenButton.setTooltip ("LISTEN: press it, then play your chord clip from the start. MIDI GEN records one loop (4, 8 or 16 bars) and learns it");
    listenButton.onClick = [this]
    {
        if (proc.getListenState() == 0) proc.startListening();
        else proc.stopListening();
    };
    for (int i = 0; i < 3; ++i)
    {
        auto* b = barButtons.add (new LedButton (juce::String (4 << i)));
        b->setTooltip ("Listen for " + juce::String (4 << i) + " bars");
        b->onClick = [this, i] { setChoice ("bars", i); };
        addAndMakeVisible (b);
    }
    generateButton.getProperties().set ("accent", true);
    generateButton.setTooltip ("Write 8 new progressions (same key, same rhythm)");
    generateButton.onClick = [this] { proc.generate(); };
    loadMidiButton.setTooltip ("Learn from a MIDI file instead (you can also drag .mid files onto MIDI GEN)");
    loadMidiButton.onClick = [this]
    {
        chooser = std::make_unique<juce::FileChooser> ("Learn chords from a MIDI file",
                                                       juce::File::getSpecialLocation (juce::File::userMusicDirectory), "*.mid;*.midi");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [this] (const juce::FileChooser& fc)
                              {
                                  juce::String err;
                                  if (fc.getResult() != juce::File())
                                      setStatus (proc.learnFromFile (fc.getResult(), err) ? "LEARNED " + fc.getResult().getFileNameWithoutExtension().toUpperCase()
                                                                                         : err.toUpperCase());
                              });
    };
    for (juce::Component* c : { (juce::Component*) &listenButton, (juce::Component*) &generateButton, (juce::Component*) &loadMidiButton })
        addAndMakeVisible (c);

    for (int i = 0; i < (int) hs::styleNames().size(); ++i) style.addItem (juce::String (hs::styleNames()[(size_t) i]), i + 1);
    style.setTooltip ("The sound the new chords are written in");
    similarity.setTooltip ("SIMILARITY: right = stay close to your chords (just new colours), left = all-new chords");
    color.setTooltip ("COLOR: plain triads (left) to lush 7ths, 9ths, 11ths and 13ths (right)");
    spice.setTooltip ("SPICE: passing chords, secondary dominants, suspensions, tritone subs and borrowed chords");
    bassMotion.setTooltip ("BASS MOTION: inversions for a walking bass line, and pedal bass under the chords");
    rhythmButton.setTooltip ("KEEP RHYTHM: play the new chords with your clip's rhythm (off = held chords)");
    bassButton.setTooltip ("ADD BASS: put the bass note underneath each chord");
    rhythmButton.setClickingTogglesState (true);
    bassButton.setClickingTogglesState (true);
    for (juce::Component* c : { (juce::Component*) &style, (juce::Component*) &similarity, (juce::Component*) &color, (juce::Component*) &spice,
                                (juce::Component*) &bassMotion, (juce::Component*) &rhythmButton, (juce::Component*) &bassButton })
        addAndMakeVisible (c);
    styleA  = std::make_unique<CA> (proc.apvts, "style", style);
    simA    = std::make_unique<SA> (proc.apvts, "similarity", similarity);
    colorA  = std::make_unique<SA> (proc.apvts, "color", color);
    spiceA  = std::make_unique<SA> (proc.apvts, "spice", spice);
    bassA   = std::make_unique<SA> (proc.apvts, "bassMotion", bassMotion);
    rhythmA = std::make_unique<BA> (proc.apvts, "keepRhythm", rhythmButton);
    addBassA = std::make_unique<BA> (proc.apvts, "addBass", bassButton);

    for (int i = 0; i < 9; ++i)
    {
        auto* c = cards.add (new Card (proc, i));
        c->setTooltip (i == 0 ? "Your original progression. Click to hear it again"
                              : "Click to play this progression instead of yours. Double-click to play it without the DAW. Drag it into Ableton to drop the MIDI");
        addAndMakeVisible (c);
    }

    loadButton.setTooltip ("Load the instrument that plays the chords (VST3 / AU)");
    loadButton.onClick = [this]
    {
       #if JUCE_MAC
        const juce::File start ("/Library/Audio/Plug-Ins");
       #else
        const juce::File start ("C:\\Program Files\\Common Files\\VST3");
       #endif
        chooser = std::make_unique<juce::FileChooser> ("Pick an instrument (.vst3 or .component)", start, "*.vst3;*.component");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles
                                  | juce::FileBrowserComponent::canSelectDirectories,
                              [this] (const juce::FileChooser& fc)
                              {
                                  if (fc.getResult() == juce::File()) return;
                                  showWindow (false);
                                  juce::String err;
                                  if (proc.loadInstrumentFromFile (fc.getResult(), err)) showWindow (true);
                                  else setStatus (err.toUpperCase());
                              });
    };
    showButton.onClick = [this] { showWindow (window == nullptr); };
    ejectButton.onClick = [this] { showWindow (false); proc.unloadInstrument(); };
    sound.addItemList ({ "Plugin / Built-in", "Built-in", "MIDI Out Only" }, 1);
    sound.setTooltip ("Where the chords go: your plugin (or the built-in keys if none), always the built-in keys, or MIDI out only");
    midiOutButton.setClickingTogglesState (true);
    midiOutButton.setTooltip ("Also send the chords out as MIDI");
    playButton.setClickingTogglesState (true);
    playButton.setTooltip ("Play the selected progression without pressing play in the DAW");
    dragAll.setTooltip ("Drag all 8 new progressions into Ableton as one clip, back to back. Click to save it");
    for (juce::Component* c : { (juce::Component*) &loadButton, (juce::Component*) &showButton, (juce::Component*) &ejectButton,
                                (juce::Component*) &sound, (juce::Component*) &vol, (juce::Component*) &bpm, (juce::Component*) &midiOutButton,
                                (juce::Component*) &playButton, (juce::Component*) &dragAll })
        addAndMakeVisible (c);
    soundA   = std::make_unique<CA> (proc.apvts, "sound", sound);
    volA     = std::make_unique<SA> (proc.apvts, "vol", vol);
    bpmA     = std::make_unique<SA> (proc.apvts, "bpm", bpm);
    midiOutA = std::make_unique<BA> (proc.apvts, "midiOut", midiOutButton);
    playA    = std::make_unique<BA> (proc.apvts, "play", playButton);

    proc.addChangeListener (this);
    setSize (kW, kH);
    startTimerHz (30);
}

MidiGenPanel::~MidiGenPanel()
{
    proc.removeChangeListener (this);
    window.reset();
    tooltips.setLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

void MidiGenPanel::setChoice (const juce::String& id, int index)
{
    if (auto* prm = proc.apvts.getParameter (id))
    {
        prm->beginChangeGesture();
        prm->setValueNotifyingHost (prm->convertTo0to1 ((float) index));
        prm->endChangeGesture();
    }
}

void MidiGenPanel::showWindow (bool show)
{
    if (! show || proc.getInstrument() == nullptr) { window.reset(); showButton.setToggleState (false, juce::dontSendNotification); return; }
    if (window == nullptr)
        window = std::make_unique<HostedWindow> (*proc.getInstrument(), [this]
        {
            juce::Component::SafePointer<MidiGenPanel> safe (this);
            juce::MessageManager::callAsync ([safe] { if (safe != nullptr) safe->showWindow (false); });
        });
    window->toFront (true);
    showButton.setToggleState (true, juce::dontSendNotification);
}

bool MidiGenPanel::isInterestedInFileDrag (const juce::StringArray& files)
{
    for (auto& f : files) if (juce::File (f).hasFileExtension ("mid;midi")) return true;
    return false;
}

void MidiGenPanel::filesDropped (const juce::StringArray& files, int, int)
{
    for (auto& f : files)
        if (juce::File (f).hasFileExtension ("mid;midi"))
        {
            juce::String err;
            setStatus (proc.learnFromFile (juce::File (f), err) ? "LEARNED " + juce::File (f).getFileNameWithoutExtension().toUpperCase() : err.toUpperCase());
            return;
        }
}

void MidiGenPanel::resized()
{
    const int x0 = 36, w = kW - 72;
    headerArea  = { x0, 14, w, 64 };
    controlArea = { x0, 90, w, 104 };
    cardArea    = { x0, 204, w, 400 };
    bottomArea  = { x0, 614, w, 102 };

    auto h = headerArea.reduced (14, 0);
    h.removeFromLeft (250);
    listenButton.setBounds (h.removeFromLeft (100).withSizeKeepingCentre (100, 36));
    h.removeFromLeft (8);
    for (auto* b : barButtons) { b->setBounds (h.removeFromLeft (46).withSizeKeepingCentre (46, 30)); h.removeFromLeft (3); }
    h.removeFromLeft (10);
    generateButton.setBounds (h.removeFromLeft (140).withSizeKeepingCentre (140, 40));
    lcdArea = h.removeFromRight (330).withSizeKeepingCentre (330, 46);

    auto c = controlArea.reduced (14, 0);
    style.setBounds (c.getX(), controlArea.getY() + 32, 260, 30);
    auto kn = juce::Rectangle<int> (c.getX() + 300, controlArea.getY() + 18, 4 * 100, 64);
    for (auto* k : { &similarity, &color, &spice, &bassMotion })
        k->setBounds (kn.removeFromLeft (100).withSizeKeepingCentre (56, 56));
    auto t = juce::Rectangle<int> (c.getRight() - 300, controlArea.getY() + 28, 140, 64);
    rhythmButton.setBounds (t.removeFromTop (30));
    t.removeFromTop (6);
    bassButton.setBounds (t.removeFromTop (30));
    loadMidiButton.setBounds (c.getRight() - 140, controlArea.getY() + 28, 140, 30);

    auto cr = cardArea.reduced (14, 0).withTrimmedTop (32).withTrimmedBottom (12);
    const int cw = (cr.getWidth() - 20) / 3, ch = (cr.getHeight() - 20) / 3;
    for (int i = 0; i < 9; ++i)
        cards[i]->setBounds (cr.getX() + (i % 3) * (cw + 10), cr.getY() + (i / 3) * (ch + 10), cw, ch);

    auto b = bottomArea.reduced (14, 0).withTrimmedTop (32);
    auto left = b.removeFromLeft (260);
    instLcd = left.removeFromTop (26);
    left.removeFromTop (6);
    auto row = left.removeFromTop (28);
    loadButton.setBounds (row.removeFromLeft (row.getWidth() - 106));
    row.removeFromLeft (6);
    showButton.setBounds (row.removeFromLeft (60));
    row.removeFromLeft (6);
    ejectButton.setBounds (row);
    b.removeFromLeft (16);
    sound.setBounds (b.removeFromLeft (170).withSizeKeepingCentre (170, 30));
    b.removeFromLeft (10);
    vol.setBounds (b.removeFromLeft (60).withSizeKeepingCentre (48, 48));
    b.removeFromLeft (10);
    midiOutButton.setBounds (b.removeFromLeft (110).withSizeKeepingCentre (110, 34));
    b.removeFromLeft (10);
    playButton.setBounds (b.removeFromLeft (90).withSizeKeepingCentre (90, 34));
    bpm.setBounds (b.removeFromLeft (60).withSizeKeepingCentre (48, 48));
    dragAll.setBounds (b.removeFromRight (130).withSizeKeepingCentre (130, 52));

    renderBackground();
}

void MidiGenPanel::renderBackground()
{
    constexpr float sc = 2.0f;
    background = juce::Image (juce::Image::ARGB, (int) (kW * sc), (int) (kH * sc), true);
    juce::Graphics g (background);
    g.addTransform (juce::AffineTransform::scale (sc));
    const auto full = juce::Rectangle<float> (0, 0, (float) kW, (float) kH);
    brushed (g, full, Col::chassisTop, Col::chassisBot, 41);
    for (int side = 0; side < 2; ++side)
    {
        auto ear = side == 0 ? full.withWidth (26.0f) : full.withLeft ((float) kW - 26.0f);
        brushed (g, ear, Col::ear.brighter (0.15f), Col::ear.darker (0.3f), 43 + side);
        for (float yy : { 60.0f, (float) kH * 0.5f, (float) kH - 60.0f })
        {
            auto slot = juce::Rectangle<float> (12.0f, 30.0f).withCentre ({ ear.getCentreX(), yy });
            g.setColour (juce::Colour (0xff070708));
            g.fillRoundedRectangle (slot, 6.0f);
        }
    }
    {
        const auto hr = headerArea.toFloat();
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.fillRoundedRectangle (hr.translated (0.0f, 2.0f), 6.0f);
        juce::Path p; p.addRoundedRectangle (hr, 6.0f);
        g.saveState(); g.reduceClipRegion (p);
        brushed (g, hr, juce::Colour (0xff3a3d44), juce::Colour (0xff1f2125), 7);
        g.restoreState();
        for (auto pt : { hr.getTopLeft().translated (10, 10), hr.getBottomLeft().translated (10, -10),
                         hr.getTopRight().translated (-10, 10), hr.getBottomRight().translated (-10, -10) })
            screw (g, pt, 4.5f, pt.x * 0.37f + pt.y * 0.11f);
        glowText (g, "MIDI GEN", juce::Rectangle<float> (hr.getX() + 26.0f, hr.getY() + 9.0f, 230.0f, 34.0f), Fonts::logo (25.0f), Col::cyan,
                  juce::Justification::centredLeft);
        engraved (g, "PROGRESSION REWRITER  //  MB AUDIO", juce::Rectangle<float> (hr.getX() + 28.0f, hr.getY() + 42.0f, 230.0f, 14.0f),
                  Fonts::label (11.5f), juce::Justification::centredLeft, Col::dim);
        engraved (g, "LISTEN LENGTH", barButtons[0]->getBounds().toFloat().withY (hr.getY() + 2.0f).withHeight (13.0f).withWidth (150.0f),
                  Fonts::label (10.5f), juce::Justification::centredLeft, Col::dim.brighter (0.3f));
        lcdBox (g, lcdArea.toFloat());
    }
    for (auto r : { controlArea, cardArea, bottomArea }) recess (g, r.toFloat(), 8.0f);

    auto title = [&] (juce::Rectangle<int> a, const char* t, const char* sub)
    {
        engraved (g, t, a.toFloat().translated (16.0f, 7.0f).withHeight (20.0f), Fonts::label (16.0f), juce::Justification::centredLeft);
        if (sub != nullptr)
            engraved (g, sub, a.toFloat().translated (16.0f + juce::GlyphArrangement::getStringWidth (Fonts::label (16.0f), t) + 14.0f, 8.0f).withHeight (20.0f),
                      Fonts::body (13.0f), juce::Justification::centredLeft, Col::dim);
    };
    auto knobLabel = [&] (const juce::Component& k, const char* text)
    {
        engraved (g, text, k.getBounds().toFloat().withY ((float) k.getBottom() - 1.0f).withHeight (14.0f).expanded (30.0f, 0.0f),
                  Fonts::label (11.5f), juce::Justification::centred, Col::dim.brighter (0.3f));
    };
    title (controlArea, "STYLE", nullptr);
    knobLabel (similarity, "SIMILARITY"); knobLabel (color, "COLOR"); knobLabel (spice, "SPICE"); knobLabel (bassMotion, "BASS MOTION");
    title (cardArea, "PROGRESSIONS", "click to swap in, drag into your DAW.  cyan = new chord");
    title (bottomArea, "SOUND", nullptr);
    knobLabel (vol, "VOL"); knobLabel (bpm, "TEMPO");
    lcdBox (g, instLcd.toFloat());
}

void MidiGenPanel::paint (juce::Graphics& g)
{
    g.drawImage (background, getLocalBounds().toFloat());

    // style description under the menu
    g.setFont (Fonts::body (13.0f));
    g.setColour (Col::dim.brighter (0.35f));
    const int si = juce::jlimit (0, (int) hs::styleBlurbs().size() - 1, style.getSelectedItemIndex());
    g.drawFittedText (juce::String (hs::styleBlurbs()[(size_t) si]), juce::Rectangle<int> (style.getX(), style.getBottom() + 4, 290, 34),
                      juce::Justification::topLeft, 2);

    // header LCD
    const auto lcd = lcdArea.toFloat().reduced (12.0f, 4.0f);
    juce::String top = "LEARNED", main;
    const int ls = proc.getListenState();
    if (statusText.isNotEmpty()) { top = "STATUS"; main = statusText; }
    else if (ls == 1) { top = "LISTENING"; main = "PLAY YOUR CLIP FROM BAR 1..."; }
    else if (ls == 2) { top = "LISTENING"; main = "RECORDING " + juce::String ((int) std::round (proc.getListenProgress() * 100)) + "%"; }
    else if (auto* src = proc.getSource(); src != nullptr && ! src->spans.empty())
    {
        top = "LEARNED  " + juce::String (hs::noteName (src->tonicPc, hs::keyPrefersFlats (src->tonicPc, src->minor))) + (src->minor ? " MINOR" : " MAJOR");
        const auto& vars = proc.getVariations();
        const int sel = proc.getSelected();
        if (sel < (int) vars.size())
        {
            for (auto& c : vars[(size_t) sel].chords) main << c.name << " ";
            top << "  /  PLAYING " << (sel == 0 ? juce::String ("ORIGINAL") : juce::String (vars[(size_t) sel].label));
        }
    }
    else main = "PRESS LISTEN";
    g.setFont (Fonts::mono (11.0f));
    g.setColour (Col::cyan.withAlpha (0.6f));
    g.drawText (top, lcd.withHeight (14.0f), juce::Justification::centredLeft);
    glowText (g, main.trim(), lcd.withTrimmedTop (14.0f), Fonts::mono (main.length() > 28 ? 13.0f : 17.0f), Col::cyan, juce::Justification::centredLeft);

    // instrument LCD
    auto* inst = proc.getInstrument();
    g.setFont (Fonts::mono (14.0f));
    g.setColour (inst != nullptr ? Col::amber : Col::dim.brighter (0.2f));
    g.drawFittedText (inst != nullptr ? inst->getName().toUpperCase() : juce::String ("BUILT-IN KEYS"), instLcd.reduced (8, 0),
                      juce::Justification::centredLeft, 1, 0.8f);
}

void MidiGenPanel::changeListenerCallback (juce::ChangeBroadcaster*)
{
    for (auto* c : cards) c->repaint();
    showButton.setEnabled (proc.getInstrument() != nullptr);
    ejectButton.setEnabled (proc.getInstrument() != nullptr);
    repaint();
}

void MidiGenPanel::timerCallback()
{
    const int bars = (int) proc.apvts.getRawParameterValue ("bars")->load();
    for (int i = 0; i < barButtons.size(); ++i) barButtons[i]->setToggleState (i == bars, juce::dontSendNotification);
    listenButton.setToggleState (proc.getListenState() == 1 || proc.getListenState() == 2, juce::dontSendNotification);
    const double pos = proc.displayPos.load();
    const int sel = proc.getSelected();
    for (int i = 0; i < cards.size(); ++i)
    {
        const double want = i == sel ? pos : -1.0;
        if (std::abs (cards[i]->playPos - want) > 1e-4 || i == sel) { cards[i]->playPos = want; cards[i]->repaint(); }
    }
    if (statusTicks > 0 && --statusTicks == 0) statusText = {};
    repaint (lcdArea);
}

// =============================================================================
MidiGenEditor::MidiGenEditor (MidiGenProcessor& p) : AudioProcessorEditor (p), panel (p)
{
    addAndMakeVisible (panel);
    float scale = 1.0f;
    if (auto* d = juce::Desktop::getInstance().getDisplays().getPrimaryDisplay())
        scale = (float) juce::jlimit (0.6, 1.0, (d->userArea.getHeight() - 110.0) / (double) kH);
    setResizable (true, true);
    setResizeLimits ((int) (kW * 0.55f), (int) (kH * 0.55f), (int) (kW * 1.6f), (int) (kH * 1.6f));
    if (auto* c = getConstrainer()) c->setFixedAspectRatio ((double) kW / (double) kH);
    setSize ((int) std::lround (kW * scale), (int) std::lround (kH * scale));
}

void MidiGenEditor::resized()
{
    panel.setTransform (juce::AffineTransform::scale ((float) getWidth() / (float) kW));
    panel.setBounds (0, 0, kW, kH);
}
