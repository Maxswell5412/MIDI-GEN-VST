#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace hs;

namespace
{
constexpr int kChunk = 64;

juce::StringArray toStringArray (const std::vector<std::string>& v)
{
    juce::StringArray a;
    for (auto& s : v) a.add (juce::String (s));
    return a;
}
} // namespace

juce::AudioProcessorValueTreeState::ParameterLayout MidiGenProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;
    auto flt = [&] (const char* id, const char* name, float lo, float hi, float def)
    { layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (lo, hi), def)); };
    auto bln = [&] (const char* id, const char* name, bool def)
    { layout.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def)); };

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "style", 1 }, "Style", toStringArray (styleNames()), 0));
    flt ("similarity", "Similarity", 0.0f, 1.0f, 0.35f);
    flt ("color", "Color", 0.0f, 1.0f, 0.5f);
    flt ("spice", "Spice", 0.0f, 1.0f, 0.4f);
    flt ("bassMotion", "Bass Motion", 0.0f, 1.0f, 0.3f);
    bln ("addBass", "Add Bass Note", true);
    bln ("keepRhythm", "Keep Rhythm", true);
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { "selected", 1 }, "Selected Progression", 0, MidiGenProcessor::kDial, MidiGenProcessor::kDial));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "flavor", 1 }, "Flavor", toStringArray (flavorNames()), 0));
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { "idea", 1 }, "Idea", 1, MidiGenProcessor::kIdeas, 1));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "gate", 1 }, "Gate", toStringArray (gateNames()), 0));
    flt ("length", "Length", 0.1f, 2.0f, 1.0f);
    flt ("envA", "Attack", 0.0f, 1.0f, 0.5f);
    flt ("envD", "Decay", 0.0f, 1.0f, 0.5f);
    flt ("envS", "Sustain", 0.0f, 1.0f, 0.5f);
    flt ("envR", "Release", 0.0f, 1.0f, 0.5f);
    flt ("hp", "High Pass", 0.0f, 1.0f, 0.0f);
    flt ("lp", "Low Pass", 0.0f, 1.0f, 1.0f);
    flt ("res", "Resonance", 0.0f, 1.0f, 0.2f);
    flt ("mbLow", "Low Band", -12.0f, 12.0f, 0.0f);
    flt ("mbMid", "Mid Band", -12.0f, 12.0f, 0.0f);
    flt ("mbHigh", "High Band", -12.0f, 12.0f, 0.0f);
    flt ("squash", "Squash", 0.0f, 1.0f, 0.0f);
    bln ("keys", "Play Chords From Keys", false);
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "bars", 1 }, "Listen Length", StringArray { "4 Bars", "8 Bars", "16 Bars" }, 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "sound", 1 }, "Sound",
                                                        StringArray { "Plugin / Built-in", "Built-in", "MIDI Out Only" }, 0));
    {
        StringArray names { "Auto" };
        for (int pt : MidiGenProcessor::patchList()) names.add (String (hs::SketchSynth::patchName (pt)));
        layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "patch", 1 }, "Built-in Sound", names, 0));
    }
    flt ("vol", "Volume", 0.0f, 1.0f, 0.8f);
    bln ("midiOut", "MIDI Out", true);
    bln ("play", "Play (without DAW)", false);
    flt ("bpm", "Tempo (without DAW)", 60.0f, 180.0f, 120.0f);
    return layout;
}

const std::vector<int>& MidiGenProcessor::patchList()
{
    using P = hs::SketchSynth;
    static const std::vector<int> list { P::EPiano, P::Piano, P::Pad, P::Strings, P::Pluck, P::Organ, P::SuperSaw, P::LoFiKeys, P::FMBell };
    return list;
}

int MidiGenProcessor::builtInPatch() const
{
    using P = hs::SketchSynth;
    const int idx = (int) apvts.getRawParameterValue ("patch")->load();
    if (idx > 0 && idx <= (int) patchList().size()) return patchList()[(size_t) idx - 1];
    // Auto: the sound of the style
    static const int byStyle[] = { P::Piano, P::Pad, P::Strings, P::Piano, P::Organ, P::Pluck, P::EPiano, P::SuperSaw,
                                   P::EPiano, P::LoFiKeys, P::Organ, P::SuperSaw, P::SuperSaw, P::Piano, P::Piano, P::LoFiKeys,
                                   P::Organ, P::Pluck, P::EPiano, P::SuperSaw, P::FMBell, P::Piano };
    constexpr int nStyles = (int) (sizeof (byStyle) / sizeof (byStyle[0]));
    const int st = juce::jlimit (0, nStyles - 1, (int) apvts.getRawParameterValue ("style")->load());
    return byStyle[st];
}

MidiGenProcessor::MidiGenProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    formatManager.addDefaultFormats();
    auto raw = [this] (const char* id) { return apvts.getRawParameterValue (id); };
    pStyle = raw ("style"); pSim = raw ("similarity"); pColor = raw ("color"); pSpice = raw ("spice");
    pBass = raw ("bassMotion"); pAddBass = raw ("addBass"); pRhythm = raw ("keepRhythm"); pSel = raw ("selected");
    pBars = raw ("bars"); pSound = raw ("sound"); pVol = raw ("vol"); pMidiOut = raw ("midiOut");
    pPlay = raw ("play"); pBpm = raw ("bpm");
    pFlavor = raw ("flavor"); pIdea = raw ("idea"); pGate = raw ("gate"); pLength = raw ("length");
    pA = raw ("envA"); pD = raw ("envD"); pS = raw ("envS"); pR = raw ("envR");
    pHp = raw ("hp"); pLp = raw ("lp"); pRes = raw ("res");
    pLow = raw ("mbLow"); pMid = raw ("mbMid"); pHigh = raw ("mbHigh"); pSquash = raw ("squash"); pKeys = raw ("keys");
    seed = (juce::uint32) juce::Random::getSystemRandom().nextInt() | 1u;
    activeOff.fill (-1.0);
    toInst.ensureSize (2048); outMidi.ensureSize (4096);
    rebuild();
    startTimerHz (20);
}

MidiGenProcessor::~MidiGenProcessor()
{
    stopTimer();
    instrument.reset();
}

bool MidiGenProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto o = l.getMainOutputChannelSet();
    return o == juce::AudioChannelSet::stereo() || o == juce::AudioChannelSet::mono();
}

// =============================================================================
GenSettings MidiGenProcessor::currentSettings() const
{
    GenSettings s;
    s.style = (int) pStyle->load();
    s.similarity = pSim->load();
    s.color = pColor->load();
    s.spice = pSpice->load();
    s.bassMotion = pBass->load();
    s.addBass = pAddBass->load() > 0.5f;
    s.flavor = (int) pFlavor->load();
    s.seed = seed;
    return s;
}

std::vector<float> MidiGenProcessor::settingsKey() const
{
    return { pStyle->load(), pSim->load(), pColor->load(), pSpice->load(), pBass->load(), pAddBass->load(), pRhythm->load(),
             pFlavor->load(), pIdea->load(), pGate->load(), pLength->load(), (float) (seed & 0xffff), (float) (seed >> 16) };
}

int MidiGenProcessor::getIdea() const { return (int) pIdea->load(); }

void MidiGenProcessor::setIdea (int idea)
{
    if (auto* p = apvts.getParameter ("idea"))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 ((float) juce::jlimit (1, kIdeas, idea)));
        p->endChangeGesture();
    }
}

void MidiGenProcessor::timerCallback()
{
    if (listenState.load (std::memory_order_acquire) == 3)
    {
        std::vector<NoteEv> notes (recNotes.begin(), recNotes.begin() + recCount);
        static const double bars[] = { 4, 8, 16 };
        const double L = bars[juce::jlimit (0, 2, (int) pBars->load())] * 4.0;
        listenState.store (0);
        if (auto* p = apvts.getParameter ("selected")) p->setValueNotifyingHost (1.0f);   // hear the dial
        if (! notes.empty())
            setSource (analyseImport (notes, L, "Learned from the track"));
        sendChangeMessage();
    }

    // turning the dial plays the dial
    const auto key = settingsKey();
    if (! haveLast || dirty || key != lastKey)
    {
        const bool ideaMoved = haveLast && ! lastKey.empty() && key[8] != lastKey[8];
        rebuild();
        if (ideaMoved && getSelected() != kDial) select (kDial);
    }

    std::unique_ptr<PlayData> trash;
    {
        const juce::SpinLock::ScopedTryLockType tl (swapLock);
        if (tl.isLocked()) trash = std::move (retired);   // the audio thread is done with it once it's here
    }
}

void MidiGenProcessor::rebuild()
{
    const auto s = currentSettings();
    const bool rhythm = pRhythm->load() > 0.5f;
    const int gate = (int) pGate->load();
    const float length = pLength->load();
    const int idea = juce::jlimit (1, kIdeas, (int) pIdea->load());

    variations.clear();
    auto d = std::make_unique<PlayData>();
    const double L = hasSource() ? source->lengthBeats : 16.0;
    d->length = L;

    auto addTake = [&] (const std::vector<NoteEv>& raw, bool shape)
    {
        const auto notes = shape ? applyGate (raw, gate, length, L) : raw;
        std::vector<PlayData::Ev> evs;
        juce::uint64 h = 1469598103934665603ull;
        for (auto& n : notes)
        {
            evs.push_back ({ n.start, n.start + n.len, n.pitch, n.vel });
            h = (h ^ (juce::uint64) (n.pitch * 131 + (int) std::lround (n.start * 960) * 7 + (int) std::lround (n.len * 96))) * 1099511628211ull;
        }
        std::sort (evs.begin(), evs.end(), [] (const PlayData::Ev& a, const PlayData::Ev& b) { return a.start < b.start; });
        d->takes.push_back (std::move (evs));
        d->hashes.push_back (h);
    };
    auto addChords = [&] (const hs::Variation& v)
    {
        std::vector<std::vector<int>> cs;
        for (auto& c : v.chords) cs.push_back (c.notes);
        d->chords.push_back (std::move (cs));
    };

    // 0: the original
    if (hasSource()) { variations.push_back (originalVariation (*source, s)); addTake (source->notes, false); }
    else { hs::Variation o; o.label = "ORIGINAL"; variations.push_back (o); addTake ({}, false); }
    addChords (variations.back());
    // 1..8: saved slots (frozen: they keep their chords whatever you turn)
    for (int i = 0; i < kSlots; ++i)
    {
        auto v = saved[(size_t) i].v;
        v.label = std::string (1, (char) ('A' + i));
        variations.push_back (v);
        addTake (saved[(size_t) i].notes, true);
        addChords (v);
    }
    // 9: the idea on the dial
    if (hasSource())
    {
        auto gen = generateVariations (*source, s, 1, idea - 1);
        auto v = gen.empty() ? hs::Variation() : gen.front();
        v.label = "IDEA " + std::to_string (idea);
        addTake (v.chords.empty() ? std::vector<NoteEv>() : renderVariation (*source, v, rhythm), true);
        variations.push_back (v);
    }
    else { hs::Variation v; v.label = "IDEA " + std::to_string (idea); variations.push_back (v); addTake ({}, false); }
    addChords (variations.back());

    {
        std::unique_ptr<PlayData> trash;
        const juce::SpinLock::ScopedLockType sl (swapLock);
        trash = std::move (pending);
        pending = std::move (d);
        if (current == nullptr) current = std::move (pending);
    }
    lastSettings = s;
    lastKey = settingsKey();
    haveLast = true;
    dirty = false;
    ++version;
    sendChangeMessage();
}

int MidiGenProcessor::save()
{
    if (! hasSource() || (int) variations.size() <= kDial || variations[(size_t) kDial].chords.empty()) return -1;
    for (int i = 0; i < kSlots; ++i)
        if (saved[(size_t) i].v.chords.empty())
        {
            auto& sl = saved[(size_t) i];
            sl.v = variations[(size_t) kDial];
            sl.v.tags = sl.v.label + (sl.v.tags.empty() ? "" : "  " + sl.v.tags);
            sl.notes = renderVariation (*source, sl.v, pRhythm->load() > 0.5f);
            sl.length = source->lengthBeats;
            dirty = true;
            rebuild();
            return i + 1;
        }
    return -1;
}

void MidiGenProcessor::clearSlot (int index)
{
    if (index < 1 || index > kSlots) return;
    saved[(size_t) index - 1] = Saved {};
    if (getSelected() == index) select (kDial);
    dirty = true;
    rebuild();
}

void MidiGenProcessor::setSource (std::shared_ptr<ImportedProgression> src)
{
    source = std::move (src);
    seed = (juce::uint32) juce::Random::getSystemRandom().nextInt() | 1u;
    dirty = true;
    rebuild();
}

void MidiGenProcessor::generate()
{
    seed = (juce::uint32) juce::Random::getSystemRandom().nextInt() | 1u;
    dirty = true;
    rebuild();
}

void MidiGenProcessor::select (int index)
{
    if (index >= 1 && index <= kSlots && ! slotUsed (index)) return;
    if (auto* p = apvts.getParameter ("selected"))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 ((float) juce::jlimit (0, (int) kDial, index)));
        p->endChangeGesture();
    }
}

int MidiGenProcessor::getSelected() const { return (int) pSel->load(); }

void MidiGenProcessor::audition (int index)
{
    select (index);
    if (auto* p = apvts.getParameter ("play")) p->setValueNotifyingHost (1.0f);
}

void MidiGenProcessor::startListening()
{
    recCount = 0;
    recOpen.fill (-1.0);
    lastLoopPos = -1.0;
    listenProgress.store (0.0);
    listenState.store (1, std::memory_order_release);
}

void MidiGenProcessor::stopListening()
{
    if (listenState.load() == 2 && recCount > 0) listenState.store (3, std::memory_order_release);   // learn what we have
    else listenState.store (0);
}

bool MidiGenProcessor::learnFromFile (const juce::File& file, juce::String& error)
{
    juce::FileInputStream in (file);
    juce::MidiFile mf;
    if (! in.openedOk() || ! mf.readFrom (in) || mf.getTimeFormat() <= 0) { error = "Couldn't read " + file.getFileName(); return false; }
    const int tpq = mf.getTimeFormat();
    std::vector<NoteEv> notes;
    double lastTick = 0;
    // a HOOKSMITH "All" file has a track per part: learn from the chords only
    std::vector<int> useTracks;
    for (int t = 0; t < mf.getNumTracks(); ++t)
        for (auto* e : *mf.getTrack (t))
            if (e->message.isTextMetaEvent() && e->message.getTextFromTextMetaEvent().containsIgnoreCase ("chord")) { useTracks.push_back (t); break; }
    for (int t = 0; t < mf.getNumTracks(); ++t)
    {
        if (! useTracks.empty() && std::find (useTracks.begin(), useTracks.end(), t) == useTracks.end()) continue;
        juce::MidiMessageSequence seq (*mf.getTrack (t));
        seq.updateMatchedPairs();
        for (auto* e : seq)
        {
            lastTick = juce::jmax (lastTick, e->message.getTimeStamp());
            if (! e->message.isNoteOn() || e->message.getChannel() == 10) continue;
            const double on = e->message.getTimeStamp();
            const double off = e->noteOffObject != nullptr ? e->noteOffObject->message.getTimeStamp() : on + tpq;
            notes.push_back ({ on / tpq, juce::jmax (0.05, (off - on) / tpq), e->message.getNoteNumber(), e->message.getVelocity() });
        }
    }
    if (notes.empty()) { error = file.getFileName() + " has no notes"; return false; }
    const auto nm = file.getFileNameWithoutExtension().toStdString();
    int pc = -1; bool mn = false;
    const bool hint = keyFromName (nm, pc, mn);
    auto imp = analyseImport (notes, lastTick / tpq, nm, hint ? pc : -1, hint ? (mn ? 1 : 0) : -1);
    if (imp->spans.empty()) { error = "No chords found in " + file.getFileName(); return false; }
    if (auto* p = apvts.getParameter ("selected")) p->setValueNotifyingHost (1.0f);   // hear the dial
    setSource (imp);
    return true;
}

// =============================================================================
void MidiGenProcessor::prepareToPlay (double sr, int)
{
    sampleRateHz = sr;
    prepared = true;
    keys.prepare (sr, SketchSynth::Keys);
    tone.prepare (sr);
    chunkBuf.setSize (2, kChunk);
    const juce::ScopedLock sl (instLock);
    if (instrument != nullptr)
    {
        instBuf.setSize (juce::jmax (2, instrument->getTotalNumOutputChannels()), kChunk);
        instrument->setRateAndBufferSizeDetails (sr, kChunk);
        instrument->prepareToPlay (sr, kChunk);
    }
    activeOff.fill (-1.0);
}

void MidiGenProcessor::releaseResources()
{
    prepared = false;
    const juce::ScopedLock sl (instLock);
    if (instrument != nullptr) instrument->releaseResources();
}

void MidiGenProcessor::killNotes (juce::MidiBuffer& dest, int pos)
{
    for (int p = 0; p < 128; ++p)
        if (activeOff[(size_t) p] >= 0.0)
        {
            dest.addEvent (juce::MidiMessage::noteOff (1, p), pos);
            if (sendOutNow) outMidi.addEvent (juce::MidiMessage::noteOff (1, p), chunkStartNow + pos);
            activeOff[(size_t) p] = -1.0;
        }
}

void MidiGenProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals nd;
    const int n = buffer.getNumSamples();
    buffer.clear();
    outMidi.clear();
    {
        const juce::SpinLock::ScopedTryLockType tl (swapLock);
        if (tl.isLocked() && pending != nullptr && retired == nullptr) { retired = std::move (current); current = std::move (pending); }
    }
    const PlayData* d = current.get();
    keys.setEnvelope (pA->load(), pD->load(), pS->load(), pR->load());

    double bpm = pBpm->load(), hostPpq = 0.0;
    bool hostPlaying = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            hostPlaying = pos->getIsPlaying();
            if (auto p = pos->getPpqPosition()) hostPpq = *p;
        }
    bpm = juce::jlimit (20.0, 400.0, bpm);
    hostSynced.store (hostPlaying);
    displayBpm.store (bpm);
    const double bps = bpm / (60.0 * sampleRateHz);
    const bool internalPlay = pPlay->load() > 0.5f && ! hostPlaying;
    if (internalPlay && ! internalWasOn) internalClock = 0.0;
    internalWasOn = internalPlay;

    const int take = (int) pSel->load();
    const bool midiOutOn = pMidiOut->load() > 0.5f;
    const int sound = (int) pSound->load();
    const float vol = pVol->load();
    static const double barsTable[] = { 4, 8, 16 };
    const double listenLen = barsTable[juce::jlimit (0, 2, (int) pBars->load())] * 4.0;

    auto send = [&] (const juce::MidiMessage& m, int pos, int chunkStart)
    {
        toInst.addEvent (m, pos);
        if (midiOutOn || sound == 2) outMidi.addEvent (m, chunkStart + pos);
    };

    for (int start = 0; start < n; start += kChunk)
    {
        const int len = juce::jmin (kChunk, n - start);
        toInst.clear();
        chunkStartNow = start;
        sendOutNow = midiOutOn || sound == 2;

        const bool running = hostPlaying || internalPlay;
        const double c0 = (hostPlaying ? hostPpq : internalClock) + start * bps;
        const double c1 = c0 + len * bps;
        const int ls = listenState.load (std::memory_order_acquire);
        const bool keyMode = pKeys->load() > 0.5f && ls == 0;
        if (keyMode != wasKeys) { releaseKeys (toInst, 0); killNotes (toInst, 0); wasKeys = keyMode; }
        const bool replacing = d != nullptr && take > 0 && take < (int) d->takes.size() && ls == 0 && ! keyMode;
        const bool auditionOriginal = d != nullptr && take == 0 && ! hostPlaying && internalPlay && ! d->takes.empty();

        // a different take, or the notes of this take changed (dial turned, knob moved): re-strike the chord now
        {
            const juce::uint64 h = d != nullptr && take < (int) d->hashes.size() ? d->hashes[(size_t) take] : 0;
            if (take != lastTake || h != lastHash) { chasePending = true; lastTake = take; lastHash = h; }
            // DAW <-> built-in clock: start clean
            if (hostPlaying != lastWasHost) { killNotes (toInst, 0); lastWasHost = hostPlaying; chasePending = true; }
        }

        // ---- LISTEN: wait for bar 1 of the loop, then record exactly one loop
        if (ls == 1 && running)
        {
            const double boundary = std::ceil (c0 / listenLen - 1.0e-9) * listenLen;   // next loop start at or after c0
            if (boundary < c1)
            {
                recStart = boundary;
                recCount = 0;
                recOpen.fill (-1.0);
                listenState.store (2, std::memory_order_release);
            }
        }
        // ---- incoming MIDI (the original clip on this track)
        for (const auto meta : midi)
        {
            if (meta.samplePosition < start || meta.samplePosition >= start + len) continue;
            const auto m = meta.getMessage();
            const int pos = meta.samplePosition - start;
            if (listenState.load() == 2 && m.isNoteOnOrOff())
            {
                const double t = c0 + pos * bps - recStart;
                const int p = m.getNoteNumber();
                if (m.isNoteOn()) { if (t >= -1.0e-6 && t < listenLen - 1.0e-6) recOpen[(size_t) p] = std::max (0.0, t); }
                else if (recOpen[(size_t) p] >= 0.0 && recCount < (int) recNotes.size())
                {
                    recNotes[(size_t) recCount++] = { recOpen[(size_t) p], juce::jmax (0.05, juce::jmin (t, listenLen) - recOpen[(size_t) p]), p, 96 };
                    recOpen[(size_t) p] = -1.0;
                }
            }
            if (keyMode && m.isNoteOnOrOff() && d != nullptr && take < (int) d->chords.size())
            {
                // KEYS: a white key plays a whole chord of the selected progression, for as long as it's held
                const int key = m.getNoteNumber();
                if (m.isNoteOn())
                {
                    const auto& cs = d->chords[(size_t) take];
                    const int ci = chordForKey (key, (int) cs.size());
                    if (ci < 0) continue;
                    for (int p : keyHeld[(size_t) key]) if (--keyCount[(size_t) p] == 0) send (juce::MidiMessage::noteOff (1, p), pos, start);
                    keyHeld[(size_t) key] = cs[(size_t) ci];
                    for (int p : keyHeld[(size_t) key])
                    {
                        if (keyCount[(size_t) p]++ > 0) send (juce::MidiMessage::noteOff (1, p), pos, start);   // re-strike a shared note
                        send (juce::MidiMessage::noteOn (1, p, m.getVelocity()), pos, start);
                    }
                    heldChordIdx.store (ci);
                }
                else
                {
                    for (int p : keyHeld[(size_t) key]) if (keyCount[(size_t) p] > 0 && --keyCount[(size_t) p] == 0) send (juce::MidiMessage::noteOff (1, p), pos, start);
                    keyHeld[(size_t) key].clear();
                    bool any = false; for (auto& kh : keyHeld) any = any || ! kh.empty();
                    if (! any) heldChordIdx.store (-1);
                }
                continue;
            }
            if (m.isNoteOnOrOff() && replacing) continue;   // the new progression plays instead
            send (m, pos, start);
        }

        if (listenState.load() == 2)
        {
            listenProgress.store (juce::jlimit (0.0, 1.0, (c1 - recStart) / listenLen));
            if (! running || c1 - recStart >= listenLen)
            {
                for (int p = 0; p < 128; ++p)
                    if (recOpen[(size_t) p] >= 0.0 && recCount < (int) recNotes.size())
                        recNotes[(size_t) recCount++] = { recOpen[(size_t) p], listenLen - recOpen[(size_t) p], p, 90 };
                listenState.store (recCount > 0 ? 3 : 0, std::memory_order_release);
            }
        }

        // ---- play the chosen progression in time (not in KEYS mode: your keyboard plays it)
        if (running && ! keyMode && (replacing || auditionOriginal))
        {
            const auto& evs = d->takes[(size_t) take];
            auto posOf = [&] (double t) { return juce::jlimit (0, len - 1, (int) ((t - c0) / bps)); };
            if (wasRunning && std::abs (c0 - lastClock) > 0.05) { killNotes (toInst, 0); chasePending = true; }   // the DAW jumped / looped
            if (! wasRunning) chasePending = true;
            if (chasePending && sinceChase >= 0.09)
            {
                killNotes (toInst, 0);
                chase (*d, take, c0, toInst, start, midiOutOn, sound);
                chasePending = false;
                sinceChase = 0.0;
            }
            for (int p = 0; p < 128; ++p)
                if (activeOff[(size_t) p] >= 0.0 && activeOff[(size_t) p] < c1)
                {
                    send (juce::MidiMessage::noteOff (1, p), posOf (activeOff[(size_t) p]), start);
                    activeOff[(size_t) p] = -1.0;
                }
            const double L = d->length;
            for (auto k = (juce::int64) std::floor (c0 / L); k <= (juce::int64) std::floor (c1 / L); ++k)
            {
                const double base = (double) k * L;
                auto it = std::lower_bound (evs.begin(), evs.end(), c0 - base - 1.0e-9,
                                            [] (const PlayData::Ev& e, double t) { return e.start < t; });
                for (; it != evs.end() && it->start < c1 - base - 1.0e-9; ++it)
                {
                    const int pos = posOf (base + it->start);
                    if (activeOff[(size_t) it->pitch] >= 0.0) send (juce::MidiMessage::noteOff (1, it->pitch), pos, start);
                    send (juce::MidiMessage::noteOn (1, it->pitch, (juce::uint8) juce::jlimit (1, 127, it->vel)), pos, start);
                    activeOff[(size_t) it->pitch] = base + it->end;
                }
            }
            lastClock = c1;
            wasRunning = true;
            displayPos.store (std::fmod (std::fmod (c0, L) + L, L));
        }
        else
        {
            if (wasRunning)
                for (int p = 0; p < 128; ++p)
                    if (activeOff[(size_t) p] >= 0.0) { send (juce::MidiMessage::noteOff (1, p), 0, start); activeOff[(size_t) p] = -1.0; }
            wasRunning = false;
            if (start == 0) displayPos.store (-1.0);
        }

        // ---- sound
        bool usedPlugin = false;
        if (sound == 0)
        {
            const juce::ScopedTryLock tl (instLock);
            if (tl.isLocked() && instrument != nullptr)
            {
                juce::AudioBuffer<float> view (instBuf.getArrayOfWritePointers(), instBuf.getNumChannels(), len);
                view.clear();
                instrument->setPlayHead (getPlayHead());
                instrument->processBlock (view, toInst);
                const int outs = instrument->getTotalNumOutputChannels();
                if (outs > 0)
                {
                    buffer.addFrom (0, start, view, 0, 0, len, vol);
                    if (buffer.getNumChannels() > 1) buffer.addFrom (1, start, view, juce::jmin (1, outs - 1), 0, len, vol);
                }
                usedPlugin = true;
            }
        }
        if (! usedPlugin && sound != 2)
        {
            chunkBuf.clear();
            keys.setPatch (builtInPatch());
            keys.render (toInst, chunkBuf.getWritePointer (0), chunkBuf.getWritePointer (1), len, vol);
            buffer.addFrom (0, start, chunkBuf, 0, 0, len);
            if (buffer.getNumChannels() > 1) buffer.addFrom (1, start, chunkBuf, 1, 0, len);
        }
        else if (sound == 2) keys.allNotesOff();

        if (internalPlay) internalClock += len * bps;
        sinceChase += len / sampleRateHz;
    }
    {
        mg::ToneSettings ts;
        ts.hp = pHp->load(); ts.lp = pLp->load(); ts.res = pRes->load();
        ts.low = pLow->load(); ts.mid = pMid->load(); ts.high = pHigh->load(); ts.squash = pSquash->load();
        tone.process (buffer.getWritePointer (0), buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : nullptr, n, ts);
    }
    midi.swapWith (outMidi);
}

void MidiGenProcessor::releaseKeys (juce::MidiBuffer& dest, int pos)
{
    for (int p = 0; p < 128; ++p)
        if (keyCount[(size_t) p] > 0)
        {
            dest.addEvent (juce::MidiMessage::noteOff (1, p), pos);
            if (sendOutNow) outMidi.addEvent (juce::MidiMessage::noteOff (1, p), chunkStartNow + pos);
            keyCount[(size_t) p] = 0;
        }
    for (auto& k : keyHeld) k.clear();
    heldChordIdx.store (-1);
}

// White keys from C3 (MIDI 60) play chord 1, 2, 3...; it repeats every N white keys, up and down the keyboard
int MidiGenProcessor::chordForKey (int note, int numChords)
{
    if (numChords <= 0) return -1;
    static const int white[12] = { 0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6 };
    const int w = white[note % 12];
    if (w < 0) return -1;
    const int idx = (note / 12) * 7 + w - (60 / 12) * 7;   // white keys counted from C3
    return ((idx % numChords) + numChords) % numChords;
}

juce::String MidiGenProcessor::keyName (int chordIndex)
{
    static const char* names[] = { "C", "D", "E", "F", "G", "A", "B" };
    return juce::String (names[chordIndex % 7]) + juce::String (3 + chordIndex / 7);
}

// Plays the notes of the take that should already be sounding at c0 (so a change is heard at once, not at the next chord)
void MidiGenProcessor::chase (const PlayData& d, int take, double c0, juce::MidiBuffer& dest, int chunkStart, bool midiOutOn, int sound)
{
    if (take < 0 || take >= (int) d.takes.size() || d.length <= 0) return;
    const double t = std::fmod (std::fmod (c0, d.length) + d.length, d.length);
    const double base = c0 - t;
    for (auto& e : d.takes[(size_t) take])
    {
        if (e.start >= t - 1.0e-9) break;                 // starts in this chunk or later: played normally
        if (e.end <= t + 0.03) continue;                  // already over (or about to be)
        const auto on = juce::MidiMessage::noteOn (1, e.pitch, (juce::uint8) juce::jlimit (1, 127, (int) (e.vel * 0.9)));
        dest.addEvent (on, 0);
        if (midiOutOn || sound == 2) outMidi.addEvent (on, chunkStart);
        activeOff[(size_t) e.pitch] = base + e.end;
    }
}

// =============================================================================
juce::File MidiGenProcessor::writeMidiFile (int index)
{
    if (! hasSource() || variations.empty()) return {};
    const bool rhythm = pRhythm->load() > 0.5f;
    const double L = source->lengthBeats;
    constexpr int ppq = 960;
    juce::MidiMessageSequence seq;
    seq.addEvent (juce::MidiMessage::tempoMetaEvent ((int) std::lround (60000000.0 / displayBpm.load())), 0);
    seq.addEvent (juce::MidiMessage::timeSignatureMetaEvent (4, 4), 0);
    const int gate = (int) pGate->load();
    const float length = pLength->load();
    auto notesOf = [&] (int i) -> std::vector<NoteEv>
    {
        if (i == 0) return source->notes;
        if (i >= 1 && i <= kSlots) return applyGate (saved[(size_t) i - 1].notes, gate, length, saved[(size_t) i - 1].length);
        if (i == kDial && (int) variations.size() > kDial) return applyGate (renderVariation (*source, variations[(size_t) kDial], rhythm), gate, length, L);
        return {};
    };
    auto add = [&] (int i, double offset)
    {
        for (auto& n : notesOf (i))
        {
            seq.addEvent (juce::MidiMessage::noteOn (1, n.pitch, (juce::uint8) juce::jlimit (1, 127, n.vel)), (offset + n.start) * ppq);
            seq.addEvent (juce::MidiMessage::noteOff (1, n.pitch), (offset + n.start + n.len) * ppq);
        }
    };
    juce::String what;
    if (index < 0)
    {
        int k = 0;
        for (int i = 1; i <= kSlots; ++i) if (slotUsed (i)) add (i, (k++) * L);
        if (k == 0) return {};
        seq.addEvent (juce::MidiMessage::endOfTrack(), (double) k * L * ppq);
        what = "Saved " + juce::String (k);
    }
    else
    {
        index = juce::jlimit (0, (int) kDial, index);
        if (index >= 1 && index <= kSlots && ! slotUsed (index)) return {};
        add (index, 0.0);
        seq.addEvent (juce::MidiMessage::endOfTrack(), L * ppq);
        what = index == 0 ? juce::String ("Original") : juce::String (variations[(size_t) index].label);
        if (index > 0)
        {
            juce::String names;
            for (auto& c : variations[(size_t) index].chords) names << c.name << " ";
            what << " - " << names.trim().replace ("/", "over");
        }
    }
    seq.updateMatchedPairs();
    juce::MidiFile mf;
    mf.setTicksPerQuarterNote (ppq);
    mf.addTrack (seq);
    auto dir = juce::File::getSpecialLocation (juce::File::userMusicDirectory).getChildFile ("MIDI GEN");
    dir.createDirectory();
    const auto style = juce::String (styleNames()[(size_t) juce::jlimit (0, (int) styleNames().size() - 1, (int) pStyle->load())]);
    auto f = dir.getChildFile (juce::File::createLegalFileName ("MIDI GEN " + style.replace ("/", "-") + " " + what).substring (0, 120) + ".mid");
    f.deleteFile();
    if (auto os = f.createOutputStream()) { mf.writeTo (*os); os->flush(); }
    return f;
}

// =============================================================================
bool MidiGenProcessor::loadInstrumentFromFile (const juce::File& file, juce::String& error)
{
    juce::OwnedArray<juce::PluginDescription> found;
    for (auto* f : formatManager.getFormats())
        if (f->fileMightContainThisPluginType (file.getFullPathName()))
            f->findAllTypesForFile (found, file.getFullPathName());
    if (found.isEmpty()) { error = "No VST3/AU plugin found in " + file.getFileName(); return false; }
    auto* desc = found.getFirst();
    for (auto* x : found) if (x->isInstrument) { desc = x; break; }
    if (desc->fileOrIdentifier.containsIgnoreCase ("MIDI GEN")) { error = "MIDI GEN can't load itself"; return false; }
    auto inst = formatManager.createPluginInstance (*desc, sampleRateHz, kChunk, error);
    if (inst == nullptr) return false;
    std::unique_ptr<juce::AudioPluginInstance> old;
    suspendProcessing (true);
    {
        const juce::ScopedLock sl (instLock);
        old = std::move (instrument);
        instrument = std::move (inst);
        instrumentDesc = *desc;
        instBuf.setSize (juce::jmax (2, instrument->getTotalNumOutputChannels()), kChunk);
        if (prepared) { instrument->setRateAndBufferSizeDetails (sampleRateHz, kChunk); instrument->prepareToPlay (sampleRateHz, kChunk); }
    }
    suspendProcessing (false);
    old.reset();
    sendChangeMessage();
    return true;
}

void MidiGenProcessor::unloadInstrument()
{
    std::unique_ptr<juce::AudioPluginInstance> old;
    suspendProcessing (true);
    { const juce::ScopedLock sl (instLock); old = std::move (instrument); }
    suspendProcessing (false);
    old.reset();
    sendChangeMessage();
}

// =============================================================================
void MidiGenProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    juce::XmlElement root ("MidiGenState");
    if (auto x = apvts.copyState().createXml()) root.addChildElement (x.release());
    root.setAttribute ("seed", juce::String ((juce::int64) seed));
    if (hasSource())
    {
        auto* sx = root.createNewChildElement ("Source");
        sx->setAttribute ("name", juce::String (source->name));
        sx->setAttribute ("length", source->lengthBeats);
        sx->setAttribute ("tonic", source->tonicPc);
        sx->setAttribute ("minor", source->minor);
        juce::String txt;
        for (auto& nt : source->notes) txt << juce::String (nt.start, 4) << "," << juce::String (nt.len, 4) << "," << nt.pitch << "," << nt.vel << ";";
        sx->setAttribute ("notes", txt);
    }
    for (int i = 0; i < kSlots; ++i)
        if (! saved[(size_t) i].v.chords.empty())
        {
            auto* x = root.createNewChildElement ("Slot");
            x->setAttribute ("index", i);
            x->setAttribute ("data", encode (saved[(size_t) i]));
        }
    if (instrument != nullptr)
    {
        auto* ix = root.createNewChildElement ("Instrument");
        ix->addChildElement (instrumentDesc.createXml().release());
        juce::MemoryBlock st; instrument->getStateInformation (st);
        ix->setAttribute ("state", st.toBase64Encoding());
    }
    copyXmlToBinary (root, dest);
}

void MidiGenProcessor::setStateInformation (const void* data, int size)
{
    auto root = getXmlFromBinary (data, size);
    if (root == nullptr || ! root->hasTagName ("MidiGenState")) return;
    if (auto* x = root->getChildByName (apvts.state.getType())) apvts.replaceState (juce::ValueTree::fromXml (*x));
    seed = (juce::uint32) root->getStringAttribute ("seed", "1").getLargeIntValue();
    if (auto* sx = root->getChildByName ("Source"))
    {
        std::vector<NoteEv> notes;
        for (auto& tok : juce::StringArray::fromTokens (sx->getStringAttribute ("notes"), ";", ""))
        {
            auto f = juce::StringArray::fromTokens (tok, ",", "");
            if (f.size() == 4) notes.push_back ({ f[0].getDoubleValue(), f[1].getDoubleValue(), f[2].getIntValue(), f[3].getIntValue() });
        }
        if (! notes.empty())
        {
            const juce::uint32 keepSeed = seed;
            auto imp = analyseImport (notes, sx->getDoubleAttribute ("length", 16), sx->getStringAttribute ("name").toStdString(),
                                      sx->getIntAttribute ("tonic", -1), sx->getBoolAttribute ("minor") ? 1 : 0);
            source = imp;
            seed = keepSeed;
        }
    }
    for (auto& sl : saved) sl = Saved {};
    for (auto* x : root->getChildWithTagNameIterator ("Slot"))
    {
        const int i = x->getIntAttribute ("index", -1);
        if (i >= 0 && i < kSlots) decode (x->getStringAttribute ("data"), saved[(size_t) i]);
    }
    if (auto* ix = root->getChildByName ("Instrument"))
    {
        juce::PluginDescription desc;
        if (auto* dx = ix->getChildElement (0); dx != nullptr && desc.loadFromXml (*dx))
        {
            juce::String err;
            auto inst = formatManager.createPluginInstance (desc, sampleRateHz, kChunk, err);
            if (inst != nullptr)
            {
                juce::MemoryBlock st;
                if (st.fromBase64Encoding (ix->getStringAttribute ("state"))) inst->setStateInformation (st.getData(), (int) st.getSize());
                suspendProcessing (true);
                {
                    const juce::ScopedLock sl (instLock);
                    instrument = std::move (inst);
                    instrumentDesc = desc;
                    instBuf.setSize (juce::jmax (2, instrument->getTotalNumOutputChannels()), kChunk);
                    if (prepared) { instrument->setRateAndBufferSizeDetails (sampleRateHz, kChunk); instrument->prepareToPlay (sampleRateHz, kChunk); }
                }
                suspendProcessing (false);
            }
        }
    }
    dirty = true;
    rebuild();
}

// A saved slot as text: "tags|length|chord;chord...|note;note..."  chord = start,len,root,q,bass,changed,name,n1 n2 ...
juce::String MidiGenProcessor::encode (const Saved& sl)
{
    juce::String t;
    t << juce::String (sl.v.tags).replace ("|", "/") << "|" << juce::String (sl.length, 4) << "|";
    for (auto& c : sl.v.chords)
    {
        t << juce::String (c.start, 4) << "," << juce::String (c.len, 4) << "," << c.rootPc << "," << (int) c.q << "," << c.bassPc << ","
          << (c.changed ? 1 : 0) << "," << juce::String (c.name).replace (",", " ").replace (";", " ").replace ("|", " ") << ",";
        for (int p : c.notes) t << p << " ";
        t << ";";
    }
    t << "|";
    for (auto& n : sl.notes) t << juce::String (n.start, 4) << "," << juce::String (n.len, 4) << "," << n.pitch << "," << n.vel << ";";
    return t;
}

bool MidiGenProcessor::decode (const juce::String& text, Saved& sl)
{
    auto parts = juce::StringArray::fromTokens (text, "|", "");
    if (parts.size() < 4) return false;
    Saved out;
    out.v.tags = parts[0].toStdString();
    out.length = juce::jmax (1.0, parts[1].getDoubleValue());
    for (auto& tok : juce::StringArray::fromTokens (parts[2], ";", ""))
    {
        auto f = juce::StringArray::fromTokens (tok, ",", "");
        if (f.size() < 8) continue;
        hs::GenChord c;
        c.start = f[0].getDoubleValue(); c.len = f[1].getDoubleValue(); c.rootPc = f[2].getIntValue();
        c.q = (hs::Q) juce::jlimit (0, (int) hs::Q::Maj7Sus2, f[3].getIntValue());
        c.bassPc = f[4].getIntValue(); c.changed = f[5].getIntValue() != 0; c.name = f[6].toStdString();
        for (auto& p : juce::StringArray::fromTokens (f[7], " ", "")) if (p.isNotEmpty()) c.notes.push_back (p.getIntValue());
        out.v.chords.push_back (c);
    }
    for (auto& tok : juce::StringArray::fromTokens (parts[3], ";", ""))
    {
        auto f = juce::StringArray::fromTokens (tok, ",", "");
        if (f.size() == 4) out.notes.push_back ({ f[0].getDoubleValue(), f[1].getDoubleValue(), f[2].getIntValue(), f[3].getIntValue() });
    }
    if (out.v.chords.empty()) return false;
    sl = out;
    return true;
}

juce::AudioProcessorEditor* MidiGenProcessor::createEditor() { return new MidiGenEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new MidiGenProcessor(); }
