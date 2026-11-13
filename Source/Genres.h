#pragma once
#include "Music.h"

// Internal genre data used by the composer.
namespace hs::detail
{
enum class Voicing { Close, Spread, Drop2 };
enum class BassStyle { Pattern, Walking, KickFollow, Kick808 };
enum class Fill { None, Toms, SnareRoll, HatRoll };

// Section masks for progressions
constexpr int V = 1, P = 2, H = 4, B = 8, ALL = V | P | H | B;

struct Prog
{
    const char* name;
    const char* chords;   // Roman numerals relative to the key's tonic, space separated
    bool minor;
    int sections;
};

struct DrumBar
{
    // 16 steps per bar. '.' none, 'g' ghost, 'x' hit, 'X' accent
    const char* kick;
    const char* snare;
    const char* clap;
    const char* hat;
    const char* openHat;
    const char* perc;
    const char* rim;
    const char* tom;   // floor tom / taiko
};

struct Melody
{
    float notesPerBar;
    float syncopation;
    bool  sixteenths;
    float leap;
    float repeat;
    float legato;
    float triplets;
};

struct Genre
{
    const char* name;
    int bpm;
    float minorBias;
    float swing;
    int extensions;            // 0 triads ... 3 lush 9ths/11ths/13ths
    Voicing voicing;
    std::vector<std::pair<double, double>> chordsPerBar;   // (chords per bar, weight)
    std::vector<const char*> chordRhythms;                 // sparse -> busy. x hit, - hold, . rest, p push
    Melody melody;
    BassStyle bass;
    std::vector<const char*> bassRhythms;                  // x root, o octave, 5 fifth, - hold, . rest
    bool drums;
    DrumBar drumA, drumB;
    bool hatRolls;
    Fill fill;
    std::vector<Prog> progs;
};

const std::vector<Genre>& genres();

// GM / Ableton Drum Rack notes
namespace Drum
{
    constexpr int Kick = 36, Rim = 37, Snare = 38, Clap = 39, ClosedHat = 42, OpenHat = 46,
                  LowTom = 45, MidTom = 47, HighTom = 50, Crash = 49, Ride = 51, Shaker = 70,
                  FloorTom = 41;
}
} // namespace hs::detail
