#pragma once

// -----------------------------------------------------------------------------
// HOOKSMITH music engine. Plain C++17, no JUCE, so it can be unit tested on its
// own. Everything is deterministic: the same settings + seeds always give the
// same music, which is how a saved Ableton set brings back the exact idea.
// -----------------------------------------------------------------------------
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace hs
{
// ---- Random numbers ----------------------------------------------------------
inline uint64_t mix64 (uint64_t x)
{
    x += 0x9E3779B97F4A7C15ull;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
    return x ^ (x >> 31);
}

struct Rng
{
    explicit Rng (uint64_t seed) : s (mix64 (seed) | 1ull) {}
    uint64_t next()
    {
        s ^= s >> 12; s ^= s << 25; s ^= s >> 27;
        return s * 2685821657736338717ull;
    }
    double uni()                 { return (double) (next() >> 11) * (1.0 / 9007199254740992.0); }
    int    range (int lo, int hi){ return lo + (int) (uni() * (double) (hi - lo + 1)) % (hi - lo + 1); }
    bool   chance (double p)     { return uni() < p; }
    uint64_t s;
};

// ---- Lists shown in the UI ---------------------------------------------------
enum Lane { ChordsLane = 0, MelodyLane, BassLane, DrumsLane, kNumLanes };
enum Section { Intro = 0, Verse, PreChorus, Hook, Bridge, Outro, kNumSections };

const std::vector<std::string>& genreNames();
const std::vector<std::string>& sectionNames();
const std::vector<std::string>& scaleNames();     // index 0 = Auto
const std::vector<std::string>& keyNames();
const std::vector<std::string>& laneNames();
const std::vector<std::string>& feelNames();      // GROOVE module feels, index 0 = genre default
int genreDefaultBpm (int genre);

// ---- Chords ------------------------------------------------------------------
enum class Q { Maj, Min, Dim, Aug, Sus2, Sus4, Dom7, Maj7, Min7, HalfDim, Dim7, Add9, MinAdd9,
               Six, Min6, Dom9, Maj9, Min9, Min11, Dom13, Sus7, MinMaj7, Dom11,
               Sus2_7, SixNine, Dom7s9, Dom7b9, Maj7s11, Maj7Sus2 };

const std::vector<int>& intervals (Q);
std::string qualitySuffix (Q);

struct ParsedChord
{
    int rootRel = 0;        // semitones above the key's tonic
    Q   q = Q::Maj;
    bool explicitQ = false; // the progression spelled out the quality (e.g. "V7")
    int accidental = 0;     // -1 flat, +1 sharp (for spelling the name)
    int bassRel = -1;       // slash chord: bass note above the tonic (e.g. "i6/bIII"), -1 = root
    bool ok = false;
};

ParsedChord parseRoman (const std::string& token);
std::string noteName (int pc, bool flats);
bool keyPrefersFlats (int keyPc, bool minor);

// ---- Generated material ------------------------------------------------------
struct NoteEv
{
    double start = 0, len = 0;   // in beats (quarter notes)
    int pitch = 60, vel = 100;
};

struct ChordSpan
{
    double start = 0, len = 0;
    std::vector<int> notes;      // voiced MIDI notes
    std::string name;            // e.g. "Am9"
    std::string roman;           // e.g. "vi"
    int rootPc = 0;
    int bassPc = -1;             // slash chord bass, -1 = root
    bool spice = false;          // a passing / anticipated chord added by SPICE
    int bassNote() const { return bassPc >= 0 ? bassPc : rootPc; }
};

struct Pattern
{
    int bars = 4;
    double lengthBeats = 16;
    std::vector<ChordSpan> chords;
    std::array<std::vector<NoteEv>, kNumLanes> lanes;
    bool minor = false;
    int scale = 0;               // resolved scale id (1..7, same numbering as scaleNames)
    std::string progressionName;
    std::vector<std::pair<double, std::string>> markers;   // song mode: section starts
};

// A chord progression imported from a MIDI file (a Cthulhu / chord pack, your own idea...).
// HOOKSMITH detects its key and chords, then plays it in any key and genre.
struct ImportedProgression
{
    struct Span { double start = 0, len = 0; int rootPc = 0; Q q = Q::Maj; std::vector<int> pitches; };
    std::string name;
    int tonicPc = 0;
    bool minor = false;
    double lengthBeats = 16;
    std::vector<NoteEv> notes;     // the chord part exactly as in the file
    std::vector<Span> spans;       // detected chords
};

// keyHint: pitch class from the file name ("... Cm.mid") or -1; minorHint: 1 / 0 / -1 unknown
std::shared_ptr<ImportedProgression> analyseImport (std::vector<NoteEv> notes, double lengthBeats, const std::string& name,
                                                    int keyHint = -1, int minorHint = -1);
// Names a chord from its notes, e.g. { 57, 60, 64, 67 } -> "Am7". Empty if no notes.
std::string nameChord (const std::vector<int>& pitches, int* rootPc = nullptr);

// Finds a key written in a file name, e.g. "Pad - 125bpm - Emin", "KIT1_C#m", "(093 B)". Returns false if none.
bool keyFromName (const std::string& name, int& pc, bool& minor);

struct Settings
{
    int genre = 0;
    int section = Hook;
    int bars = 8;                // 4, 8 or 16
    int key = 0;                 // 0 = C
    int scale = 0;               // 0 = Auto
    int progression = 0;         // 0 = Auto (PROGRESSOR), 1.. = named progression of the genre
    float complexity = 0.5f;     // how rich the harmony / how busy the variations
    float spice = 0.35f;         // passing chords, pushes, sus resolves, extensions
    float density = 0.5f;        // how many notes
    float groove = 0.5f;         // SWING: 0 straight, 0.5 genre swing, 1 double swing
    float humanize = 0.25f;
    // GROOVE module
    int feel = 0;                // 0 = genre default, 1 = quantized, ... (see feelNames)
    float feelAmount = 0.6f;     // how strongly the feel is applied
    float pocket = 0.0f;         // -1 pushed (ahead of the beat) ... +1 laid back (behind)
    float dynamics = 0.5f;       // accents and phrasing in the velocities
    float strum = 0.3f;          // chords: spread the notes like a hand / a guitar strum
    uint32_t harmonySeed = 1, melodySeed = 2, bassSeed = 3, drumSeed = 4;
    // GENERATE takes: 0 = the idea itself, 1 and 2 = close variations of it (same seed)
    std::array<int, kNumLanes> variant { 0, 0, 0, 0 };
    // LENGTH dial per part: note length x 0.1 .. 2 (1 = as written)
    std::array<float, kNumLanes> noteLength { 1.0f, 1.0f, 1.0f, 1.0f };
    // COLOR > MACHINE per part: 0 off, 1 MPC60, 2 MPC2000, 3 MPC3000, 4 SP-1200, with its FLAVOR 0..1
    std::array<int, kNumLanes> machine { 0, 0, 0, 0 };
    std::array<float, kNumLanes> machineFlavor { 0.5f, 0.5f, 0.5f, 0.5f };
    std::array<float, kNumLanes> machineExaggerate { 0, 0, 0, 0 };   // push the machine past its real specs
    std::array<float, kNumLanes> machineFunk { 0, 0, 0, 0 };         // random push/pull and accents
    std::shared_ptr<const ImportedProgression> imported;   // set = use these chords instead of the PROGRESSOR
};

Pattern compose (const Settings&);
Pattern composeSong (const Settings&);   // Intro, Verse, Pre, Hook ... Outro back to back

// Named progressions of a genre (for the PROGRESSOR menu). Index 0 of the menu is "Auto".
std::vector<std::string> progressionNames (int genre);

// The 12-chord bank used by the chord pads / Chord Keys mode. Slot = pitch class of the key
// you press (C = slot 0 ... B = slot 11). White keys give the scale's own chords, black keys
// give borrowed "colour" chords.
struct PadChord
{
    std::string name, roman;
    int rootPc = 0;
    std::vector<int> intervals;  // relative to root
    std::vector<int> fixedNotes; // imported chords keep their own voicing
};
std::array<PadChord, 12> chordBank (const Settings&);

// Voice a chord close to the previous voicing (smooth voice leading).
std::vector<int> voiceChord (int rootPc, const std::vector<int>& intervals, int genre,
                             const std::vector<int>& previous, int centre = 62, bool wide = false, int bassPc = -1);
} // namespace hs
