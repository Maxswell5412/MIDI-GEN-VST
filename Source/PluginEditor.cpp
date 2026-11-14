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
    const bool has = index < (int) vars.size() && ! vars[(size_t) index].chords.empty();
    const bool selected = proc.getSelected() == index;
    const bool isDial = index == MidiGenProcessor::kDial;

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
    const juce::String label = index == 0 ? juce::String ("ORIGINAL") : isDial ? "IDEA " + juce::String (proc.getIdea())
                                          : juce::String::charToString ((juce::juce_wchar) ('A' + index - 1));
    glowText (g, label, top.removeFromLeft (index == 0 || isDial ? 120.0f : 30.0f), Fonts::logo (index == 0 ? 13.0f : isDial ? 16.0f : 17.0f),
              selected ? Col::amber : isDial ? Col::cyan : Col::engrave.withAlpha (has ? 1.0f : 0.4f), juce::Justification::centredLeft);
    led (g, { top.getRight() - 6.0f, top.getCentreY() }, 4.0f, index == 0 ? Col::green : Col::amber, selected && playPos >= 0.0);
    top.removeFromRight (16.0f);

    if (! has)
    {
        g.setFont (Fonts::body (14.0f));
        g.setColour (Col::dim);
        g.drawFittedText (index == 0 || (isDial && proc.getSource() == nullptr) ? "Press LISTEN and play your chord clip,\nor drop a MIDI file on MIDI GEN"
                                     : isDial ? "Turn the IDEA dial" : "EMPTY\nturn the dial, press SAVE",
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
    const bool keys = proc.keysMode() && selected;
    int ci = -1;
    for (auto& c : v.chords)
    {
        ++ci;
        const float x0 = strip.getX() + (float) (c.start / L) * strip.getWidth();
        const float x1 = strip.getX() + (float) ((c.start + c.len) / L) * strip.getWidth();
        auto cell = juce::Rectangle<float> (x0, strip.getY(), x1 - x0, strip.getHeight()).reduced (1.0f, 0.0f);
        const bool now = keys ? proc.heldChord() == ci : (selected && playPos >= c.start && playPos < c.start + c.len);
        const auto col = c.changed ? Col::cyan : Col::amber;
        g.setColour (col.withAlpha (now ? 0.3f : 0.08f));
        g.fillRoundedRectangle (cell, 3.0f);
        const float fh = juce::jlimit (8.5f, 15.0f, cell.getWidth() / juce::jmax (3.0f, (float) c.name.size() * 0.62f));
        g.setFont (Fonts::mono (fh));
        g.setColour (now ? juce::Colours::white : col);
        g.drawFittedText (c.name, cell.reduced (2.0f, 0.0f).toNearestInt(), juce::Justification::centred, 2, 0.7f);
        if (keys)
        {
            // which key plays this chord
            g.setFont (Fonts::mono (9.5f));
            g.setColour (Col::cyan.withAlpha (0.85f));
            g.drawText (MidiGenProcessor::keyName (ci), cell.reduced (3.0f, 2.0f), juce::Justification::topLeft);
        }
    }
    if (selected && playPos >= 0.0 && ! keys)
    {
        const float x = strip.getX() + (float) (playPos / L) * strip.getWidth();
        g.setColour (juce::Colour (0xffffe0b0));
        g.fillRect (x - 0.75f, strip.getY(), 1.5f, strip.getHeight());
    }
}

void Card::mouseDown (const juce::MouseEvent& e)
{
    dragging = false;
    if (e.mods.isPopupMenu() && proc.slotUsed (index))
    {
        juce::PopupMenu m;
        m.addItem (1, "Clear slot " + juce::String::charToString ((juce::juce_wchar) ('A' + index - 1)));
        juce::Component::SafePointer<Card> safe (this);
        m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this), [safe] (int r)
        {
            if (safe != nullptr && r == 1) { safe->proc.clearSlot (safe->index); if (auto* p = safe->getParentComponent()) p->repaint(); }
        });
        return;
    }
    if (index < (int) proc.getVariations().size()) proc.select (index);
    if (auto* p = getParentComponent()) p->repaint();
}

void Card::mouseDoubleClick (const juce::MouseEvent&)
{
    if (index < (int) proc.getVariations().size()) proc.audition (index);
}

void Card::mouseDrag (const juce::MouseEvent& e)
{
    if (dragging || e.getDistanceFromDragStart() < 6 || index >= (int) proc.getVariations().size() || proc.getVariations()[(size_t) index].chords.empty()) return;
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
    g.drawFittedText ("DRAG SAVED", r.toNearestInt(), juce::Justification::centred, 1);
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
    generateButton.setTooltip ("A whole new set of 100 ideas on the IDEA dial (same key, same rhythm). Your saved slots stay");
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
    flavor.setTooltip ("FLAVOR: the kind of chords the ideas reach for, in the same key. Auto = the style's own. Soul Keys, West Coast, Dance Kiss, Neo Gold, Dark Trap, Gospel Lift, Cinema, Lo-Fi Jazz");
    flavor.setSteps ((int) hs::flavorNames().size());
    flavor.setCap (Col::capOrange);
    similarity.setCap (Col::capGrey); color.setCap (Col::capBlue); spice.setCap (Col::capRed); bassMotion.setCap (Col::capBrown);
    for (juce::Component* c : { (juce::Component*) &style, (juce::Component*) &flavor, (juce::Component*) &similarity, (juce::Component*) &color, (juce::Component*) &spice,
                                (juce::Component*) &bassMotion, (juce::Component*) &rhythmButton, (juce::Component*) &bassButton })
        addAndMakeVisible (c);
    flavorA = std::make_unique<SA> (proc.apvts, "flavor", flavor);

    // ---- the IDEA dial
    idea.setCap (Col::capRed);
    idea.setSteps (0);
    idea.setMouseDragSensitivity (900);
    idea.setTooltip ("IDEA: turn it. Every click is a new progression in the same key and rhythm, playing straight away. Hear one you love? Press SAVE");
    ideaA = std::make_unique<SA> (proc.apvts, "idea", idea);
    saveButton.getProperties().set ("accent", true);
    saveButton.setTooltip ("SAVE the idea on the dial into the next free slot (A to H). Then keep turning");
    saveButton.onClick = [this]
    {
        const int slot = proc.save();
        setStatus (slot > 0 ? "SAVED IDEA " + juce::String (proc.getIdea()) + " TO SLOT " + juce::String::charToString ((juce::juce_wchar) ('A' + slot - 1))
                            : proc.hasSource() ? juce::String ("SLOTS FULL: RIGHT-CLICK ONE TO CLEAR IT") : juce::String ("LISTEN FIRST"));
    };
    prevButton.setTooltip ("Previous idea");
    nextButton.setTooltip ("Next idea");
    prevButton.onClick = [this] { proc.setIdea (proc.getIdea() - 1); };
    nextButton.onClick = [this] { proc.setIdea (proc.getIdea() + 1); };
    for (juce::Component* c : { (juce::Component*) &idea, (juce::Component*) &saveButton, (juce::Component*) &prevButton, (juce::Component*) &nextButton })
        addAndMakeVisible (c);

    // ---- SHAPE + TONE
    struct T { ui::Knob* k; const char* id; juce::Colour cap; const char* tip; bool bip; };
    const T tones[] = {
        { &gate,   "gate",   Col::capOrange, "GATE: chops the chords into a rhythm locked to your DAW (1/4 ... 1/16, triplets, offbeat, bounce, house stab, trance, pump)", false },
        { &length, "length", Col::capWhite,  "LENGTH: how long each chord rings. Middle = up to the next hit (legato), left = short stabs, right = overlap", false },
        { &envA,   "envA",   Col::capGreen,  "ATTACK of the built-in sound (middle = as designed)", false },
        { &envD,   "envD",   Col::capGreen,  "DECAY of the built-in sound (middle = as designed)", false },
        { &envS,   "envS",   Col::capGreen,  "SUSTAIN of the built-in sound (middle = as designed)", false },
        { &envR,   "envR",   Col::capGreen,  "RELEASE of the built-in sound (middle = as designed)", false },
        { &hp,     "hp",     Col::capBlue,   "HIGH PASS filter: cuts the lows (off at the left, up to 2 kHz)", false },
        { &lp,     "lp",     Col::capBlue,   "LOW PASS filter: cuts the highs (off at the right, down to 200 Hz)", false },
        { &res,    "res",    Col::capBlue,   "RESONANCE of both filters", false },
        { &mbLow,  "mbLow",  Col::capBrown,  "MULTIBAND LOW (below 200 Hz): -12 to +12 dB", true },
        { &mbMid,  "mbMid",  Col::capBrown,  "MULTIBAND MID (200 Hz to 2.5 kHz): -12 to +12 dB", true },
        { &mbHigh, "mbHigh", Col::capBrown,  "MULTIBAND HIGH (above 2.5 kHz): -12 to +12 dB", true },
        { &squash, "squash", Col::capRed,    "SQUASH: 3-band compression that glues and thickens the chords", false },
    };
    for (auto& t : tones)
    {
        t.k->setCap (t.cap);
        t.k->setTooltip (t.tip);
        if (t.bip) t.k->setBipolar (true);
        addAndMakeVisible (t.k);
        toneA.push_back (std::make_unique<SA> (proc.apvts, t.id, *t.k));
    }
    gate.setSteps ((int) hs::gateNames().size());
    vol.setCap (Col::capWhite); bpm.setCap (Col::capBlack);
    styleA  = std::make_unique<CA> (proc.apvts, "style", style);
    simA    = std::make_unique<SA> (proc.apvts, "similarity", similarity);
    colorA  = std::make_unique<SA> (proc.apvts, "color", color);
    spiceA  = std::make_unique<SA> (proc.apvts, "spice", spice);
    bassA   = std::make_unique<SA> (proc.apvts, "bassMotion", bassMotion);
    rhythmA = std::make_unique<BA> (proc.apvts, "keepRhythm", rhythmButton);
    addBassA = std::make_unique<BA> (proc.apvts, "addBass", bassButton);

    for (int i = 0; i <= MidiGenProcessor::kDial; ++i)
    {
        auto* c = cards.add (new Card (proc, i));
        c->setTooltip (i == 0 ? "Your original progression. Click to hear it again"
                     : i == MidiGenProcessor::kDial ? "The idea on the dial. Click to hear it, drag it into your DAW"
                     : "A saved idea. Click to play it, double-click to play it without the DAW, drag it into your DAW. Right-click to clear");
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
    keysButton.setClickingTogglesState (true);
    keysButton.setTooltip ("KEYS: play the chords from your MIDI keyboard instead of auto-play. White keys from C3 = chord 1, 2, 3... of the selected progression. Hold a key, the chord holds. Great for recording");
    playButton.setTooltip ("Play the selected progression without pressing play in the DAW");
    dragAll.setTooltip ("Drag all your saved ideas into the DAW as one clip, back to back. Click to save it as a file");
    for (juce::Component* c : { (juce::Component*) &loadButton, (juce::Component*) &showButton, (juce::Component*) &ejectButton,
                                (juce::Component*) &sound, (juce::Component*) &vol, (juce::Component*) &bpm, (juce::Component*) &midiOutButton,
                                (juce::Component*) &playButton, (juce::Component*) &keysButton, (juce::Component*) &dragAll })
        addAndMakeVisible (c);
    soundA   = std::make_unique<CA> (proc.apvts, "sound", sound);
    volA     = std::make_unique<SA> (proc.apvts, "vol", vol);
    bpmA     = std::make_unique<SA> (proc.apvts, "bpm", bpm);
    midiOutA = std::make_unique<BA> (proc.apvts, "midiOut", midiOutButton);
    playA    = std::make_unique<BA> (proc.apvts, "play", playButton);
    keysA    = std::make_unique<BA> (proc.apvts, "keys", keysButton);

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
    toneArea    = { x0, 614, w, 122 };
    bottomArea  = { x0, 746, w, 120 };

    auto h = headerArea.reduced (14, 0);
    h.removeFromLeft (250);
    listenButton.setBounds (h.removeFromLeft (100).withSizeKeepingCentre (100, 36));
    h.removeFromLeft (8);
    for (auto* b : barButtons) { b->setBounds (h.removeFromLeft (46).withSizeKeepingCentre (46, 30)); h.removeFromLeft (3); }
    h.removeFromLeft (10);
    generateButton.setBounds (h.removeFromLeft (140).withSizeKeepingCentre (140, 40));
    lcdArea = h.removeFromRight (330).withSizeKeepingCentre (330, 46);

    auto c = controlArea.reduced (14, 0);
    style.setBounds (c.getX(), controlArea.getY() + 32, 250, 30);
    auto kn = juce::Rectangle<int> (c.getX() + 262, controlArea.getY() + 16, 5 * 88, 66);
    for (auto* k : { &flavor, &similarity, &color, &spice, &bassMotion })
        k->setBounds (kn.removeFromLeft (88).withSizeKeepingCentre (60, 60));
    auto t = juce::Rectangle<int> (c.getRight() - 286, controlArea.getY() + 28, 136, 64);
    rhythmButton.setBounds (t.removeFromTop (30));
    t.removeFromTop (6);
    bassButton.setBounds (t.removeFromTop (30));
    loadMidiButton.setBounds (c.getRight() - 136, controlArea.getY() + 28, 136, 30);

    // IDEA dial on the left, the dial's idea + ORIGINAL on top, 8 saved slots below
    auto cr = cardArea.reduced (14, 0).withTrimmedTop (32).withTrimmedBottom (12);
    auto left = cr.removeFromLeft (230);
    cr.removeFromLeft (12);
    idea.setBounds (left.getX() + 25, left.getY() + 2, 180, 180);
    ideaReadout = { left.getX(), left.getY() + 186, 230, 40 };
    auto row = juce::Rectangle<int> (left.getX(), left.getY() + 232, 230, 34);
    prevButton.setBounds (row.removeFromLeft (44));
    nextButton.setBounds (row.removeFromRight (44));
    row.reduce (8, 0);
    saveButton.setBounds (row);
    cards[0]->setBounds (left.getX(), left.getY() + 276, 230, left.getBottom() - (left.getY() + 276));

    auto topRow = cr.removeFromTop (118);
    cards[MidiGenProcessor::kDial]->setBounds (topRow);
    cr.removeFromTop (10);
    const int cw = (cr.getWidth() - 30) / 4, ch = (cr.getHeight() - 10) / 2;
    for (int i = 0; i < MidiGenProcessor::kSlots; ++i)
        cards[i + 1]->setBounds (cr.getX() + (i % 4) * (cw + 10), cr.getY() + (i / 4) * (ch + 10), cw, ch);

    // SHAPE + TONE
    auto tr = toneArea.reduced (14, 0).withTrimmedTop (30).withTrimmedBottom (20);
    const ui::Knob* order[] = { &gate, &length, &envA, &envD, &envS, &envR, &hp, &lp, &res, &mbLow, &mbMid, &mbHigh, &squash };
    const int gaps = 3 * 14;   // a little air between the groups
    const int kw = (tr.getWidth() - gaps) / 13;
    int xx = tr.getX();
    for (int i = 0; i < 13; ++i)
    {
        if (i == 2 || i == 6 || i == 9) xx += 14;
        const_cast<ui::Knob*> (order[i])->setBounds (juce::Rectangle<int> (xx, tr.getY(), kw, tr.getHeight()).withSizeKeepingCentre (54, 54));
        xx += kw;
    }

    auto b = bottomArea.reduced (14, 0).withTrimmedTop (34).withTrimmedBottom (10);
    auto bl = b.removeFromLeft (260);
    instLcd = bl.removeFromTop (26);
    bl.removeFromTop (6);
    auto brow = bl.removeFromTop (28);
    loadButton.setBounds (brow.removeFromLeft (brow.getWidth() - 106));
    brow.removeFromLeft (6);
    showButton.setBounds (brow.removeFromLeft (60));
    brow.removeFromLeft (6);
    ejectButton.setBounds (brow);
    b.removeFromLeft (16);
    sound.setBounds (b.removeFromLeft (170).withSizeKeepingCentre (170, 30));
    b.removeFromLeft (10);
    vol.setBounds (b.removeFromLeft (64).withSizeKeepingCentre (54, 54));
    b.removeFromLeft (10);
    midiOutButton.setBounds (b.removeFromLeft (110).withSizeKeepingCentre (110, 34));
    b.removeFromLeft (10);
    playButton.setBounds (b.removeFromLeft (90).withSizeKeepingCentre (90, 34));
    bpm.setBounds (b.removeFromLeft (64).withSizeKeepingCentre (54, 54));
    b.removeFromLeft (10);
    keysButton.setBounds (b.removeFromLeft (90).withSizeKeepingCentre (90, 34));
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
    for (auto r : { controlArea, cardArea, toneArea, bottomArea }) recess (g, r.toFloat(), 8.0f);

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
    title (cardArea, "IDEAS", "turn the dial, SAVE the ones you love, drag them into your DAW.  cyan = new chord");
    title (toneArea, "SHAPE + TONE", "gate and length shape the MIDI too.  ADSR is for the built-in sound");
    knobLabel (length, "LENGTH");
    knobLabel (envA, "ATTACK"); knobLabel (envD, "DECAY"); knobLabel (envS, "SUSTAIN"); knobLabel (envR, "RELEASE");
    knobLabel (hp, "HIGH PASS"); knobLabel (lp, "LOW PASS"); knobLabel (res, "RES");
    knobLabel (mbLow, "LOW"); knobLabel (mbMid, "MID"); knobLabel (mbHigh, "HIGH"); knobLabel (squash, "SQUASH");
    {
        // group captions above the tone knobs
        auto cap = [&] (const ui::Knob& a, const ui::Knob& z, const char* text)
        {
            auto r = juce::Rectangle<float> ((float) a.getX() - 6.0f, (float) a.getY() - 16.0f, (float) (z.getRight() - a.getX()) + 12.0f, 12.0f);
            g.setColour (Col::dim.withAlpha (0.5f));
            g.drawHorizontalLine ((int) r.getCentreY(), r.getX(), r.getRight());
            engraved (g, text, r.withSizeKeepingCentre (juce::GlyphArrangement::getStringWidth (Fonts::label (10.5f), text) + 14.0f, 12.0f),
                      Fonts::label (10.5f), juce::Justification::centred, Col::dim.brighter (0.35f));
        };
        cap (gate, length, "RHYTHM"); cap (envA, envR, "ENVELOPE"); cap (hp, res, "FILTER"); cap (mbLow, squash, "MULTIBAND");
    }
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
    g.drawFittedText (juce::String (hs::styleBlurbs()[(size_t) si]), juce::Rectangle<int> (style.getX(), style.getBottom() + 4, style.getWidth(), 34),
                      juce::Justification::topLeft, 2);

    // stepped knob names (FLAVOR, GATE) in amber under the knob
    auto stepName = [&] (const ui::Knob& k, const juce::String& text)
    {
        glowText (g, text.toUpperCase(), k.getBounds().toFloat().withY ((float) k.getBottom() - 1.0f).withHeight (14.0f).expanded (34.0f, 0.0f),
                  Fonts::label (11.5f), Col::amber, juce::Justification::centred);
    };
    {
        const auto& fn = hs::flavorNames();
        const auto& gn = hs::gateNames();
        stepName (flavor, "FLAVOR: " + juce::String (fn[(size_t) juce::jlimit (0, (int) fn.size() - 1, (int) proc.apvts.getRawParameterValue ("flavor")->load())]));
        stepName (gate, "GATE " + juce::String (gn[(size_t) juce::jlimit (0, (int) gn.size() - 1, (int) proc.apvts.getRawParameterValue ("gate")->load())]));
    }
    // IDEA readout under the big dial
    {
        lcdBox (g, ideaReadout.toFloat());
        const bool live = proc.getSelected() == MidiGenProcessor::kDial;
        g.setFont (Fonts::mono (10.5f));
        g.setColour (Col::cyan.withAlpha (0.6f));
        g.drawText (live ? "PLAYING" : "DIAL", ideaReadout.toFloat().reduced (10.0f, 3.0f), juce::Justification::topLeft);
        glowText (g, "IDEA " + juce::String (proc.getIdea()) + " / " + juce::String (MidiGenProcessor::kIdeas), ideaReadout.toFloat().reduced (10.0f, 2.0f).withTrimmedTop (12.0f),
                  Fonts::mono (17.0f), live ? Col::cyan : Col::cyan.withAlpha (0.55f), juce::Justification::centredLeft);
    }

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
            top << " / " << (sel == 0 ? juce::String ("ORIGINAL") : juce::String (vars[(size_t) sel].label));
        }
    }
    else main = "PRESS LISTEN";
    if (proc.keysMode() && statusText.isEmpty() && proc.getListenState() == 0) top = "KEYS MODE: C3 D3 E3... PLAY THE CHORDS";
    g.setFont (Fonts::mono (11.0f));
    g.setColour (Col::cyan.withAlpha (0.6f));
    g.drawText (top, lcd.withHeight (14.0f), juce::Justification::centredLeft);
    {
        const bool synced = proc.isSyncedToHost();
        const juce::String sync = (synced ? "DAW SYNC " : "FREE ") + juce::String ((int) std::round (proc.displayBpm.load())) + " BPM";
        g.setColour (synced ? Col::green.withAlpha (0.85f) : Col::dim.brighter (0.2f));
        g.drawText (sync, lcd.withHeight (14.0f), juce::Justification::centredRight);
    }
    glowText (g, main.trim(), lcd.withTrimmedTop (14.0f), Fonts::mono (main.length() > 28 ? 13.0f : 17.0f), Col::cyan, juce::Justification::centredLeft);

    // instrument LCD
    auto* inst = proc.getInstrument();
    g.setFont (Fonts::mono (14.0f));
    g.setColour (inst != nullptr ? Col::amber : Col::dim.brighter (0.2f));
    g.drawFittedText (inst != nullptr ? inst->getName().toUpperCase()
                                     : (proc.apvts.getRawParameterValue ("patch")->load() < 0.5f ? "AUTO: " : "") + juce::String (hs::SketchSynth::patchName (proc.builtInPatch())), instLcd.reduced (8, 0),
                      juce::Justification::centredLeft, 1, 0.8f);
}

// Click the instrument display to pick a built-in sound
void MidiGenPanel::mouseDown (const juce::MouseEvent& e)
{
    if (! instLcd.contains (e.getPosition()) || proc.getInstrument() != nullptr) return;
    auto* prm = proc.apvts.getParameter ("patch");
    const int cur = (int) proc.apvts.getRawParameterValue ("patch")->load();
    juce::PopupMenu m;
    m.addSectionHeader ("BUILT-IN SOUND");
    m.addItem (1, "Auto (sound of the style)", true, cur == 0);
    const auto& list = MidiGenProcessor::patchList();
    for (int i = 0; i < (int) list.size(); ++i)
        m.addItem (i + 2, juce::String (hs::SketchSynth::patchName (list[(size_t) i])), true, cur == i + 1);
    juce::Component::SafePointer<MidiGenPanel> safe (this);
    m.showMenuAsync (juce::PopupMenu::Options().withTargetComponent (this).withTargetScreenArea (localAreaToGlobal (instLcd)),
                     [safe, prm] (int r)
                     {
                         if (safe == nullptr || r <= 0) return;
                         prm->beginChangeGesture();
                         prm->setValueNotifyingHost (prm->convertTo0to1 ((float) (r - 1)));
                         prm->endChangeGesture();
                         safe->repaint();
                     });
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
    if (const int pt = proc.builtInPatch(); pt != shownPatch) { shownPatch = pt; repaint (instLcd); }
    const double pos = proc.displayPos.load();
    const int sel = proc.getSelected();
    for (int i = 0; i < cards.size(); ++i)
    {
        const double want = i == sel ? pos : -1.0;
        if (std::abs (cards[i]->playPos - want) > 1e-4 || i == sel) { cards[i]->playPos = want; cards[i]->repaint(); }
    }
    if (statusTicks > 0 && --statusTicks == 0) statusText = {};
    repaint (lcdArea);
    if (const int held = proc.keysMode() ? proc.heldChord() : -2; held != lastHeld)
    {
        lastHeld = held;
        for (auto* c : cards) c->repaint();
    }
    // the step names and the idea number follow the knobs
    static int lastF = -1, lastG = -1, lastI = -1, lastSel = -1;
    const int f = (int) proc.apvts.getRawParameterValue ("flavor")->load(), gt = (int) proc.apvts.getRawParameterValue ("gate")->load();
    if (f != lastF || gt != lastG || proc.getIdea() != lastI || sel != lastSel)
    {
        lastF = f; lastG = gt; lastI = proc.getIdea(); lastSel = sel;
        repaint (flavor.getBounds().expanded (40, 20)); repaint (gate.getBounds().expanded (40, 20)); repaint (ideaReadout);
        cards[MidiGenProcessor::kDial]->repaint();
    }
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
