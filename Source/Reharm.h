#pragma once

// -----------------------------------------------------------------------------
// MIDI GEN reharmonizer: learns a chord progression and writes new ones in the
// same key and the same rhythm, in the style of today's pop, rap, R&B and dance
// records. Plain C++ (no JUCE), deterministic per seed.
// -----------------------------------------------------------------------------
#include "Music.h"

namespace hs
{
struct GenChord
{
    double start = 0, len = 0;
    int rootPc = 0;
    Q q = Q::Maj;
    int bassPc = -1;             // slash bass, -1 = root
    std::vector<int> notes;      // voiced, includes the bass note if "add bass" is on
    std::string name, roman;
    bool changed = false;        // different from the original chord at this time
};

struct Variation
{
    std::string label;           // "A".."H"
    std::string tags;            // what it did, e.g. "borrowed iv, sec. dominant"
    std::vector<GenChord> chords;
};

struct GenSettings
{
    int style = 0;
    float similarity = 0.4f;     // 0 = all new chords, 1 = keep the original chords (just recoloured)
    float color = 0.5f;          // plain triads ... lush 9ths / 11ths / 13ths
    float spice = 0.4f;          // passing chords, secondary dominants, suspensions, tritone subs
    float bassMotion = 0.3f;     // inversions and pedal bass
    bool addBass = true;         // also turns on wide "melodic" voicings (low bass, spread chord on top)
    int flavor = 0;              // FLAVOR: 0 = the style's own chords, 1.. = a signature chord flavour
    uint32_t seed = 1;
};

const std::vector<std::string>& styleNames();
const std::vector<std::string>& styleBlurbs();
const std::vector<std::string>& flavorNames();   // [0] = "Auto"
const std::vector<std::string>& flavorBlurbs();
const std::vector<std::string>& gateNames();     // [0] = "Off"

// GATE + LENGTH: chops each chord into a rhythm (gate > 0) and sets how long each hit rings
// (length 0.1 .. 2, 1 = up to the next hit). Notes never overlap themselves.
std::vector<NoteEv> applyGate (const std::vector<NoteEv>& notes, int gate, float length, double loopLen);

// The learned progression, voiced and named (variation "ORIGINAL").
Variation originalVariation (const ImportedProgression& src, const GenSettings& s);

// New progressions. firstIndex picks where in the endless list to start (the IDEA dial: idea k = generateVariations (src, s, 1, k)).
std::vector<Variation> generateVariations (const ImportedProgression& src, const GenSettings& s, int count = 8, int firstIndex = 0);

// Turns chords into notes. keepRhythm = hit the chords where the original clip hit them.
std::vector<NoteEv> renderVariation (const ImportedProgression& src, const Variation& v, bool keepRhythm);
} // namespace hs
