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
    bool addBass = true;
    uint32_t seed = 1;
};

const std::vector<std::string>& styleNames();
const std::vector<std::string>& styleBlurbs();

// The learned progression, voiced and named (variation "ORIGINAL").
Variation originalVariation (const ImportedProgression& src, const GenSettings& s);

// Eight new progressions.
std::vector<Variation> generateVariations (const ImportedProgression& src, const GenSettings& s, int count = 8);

// Turns chords into notes. keepRhythm = hit the chords where the original clip hit them.
std::vector<NoteEv> renderVariation (const ImportedProgression& src, const Variation& v, bool keepRhythm);
} // namespace hs
