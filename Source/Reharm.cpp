#include "Reharm.h"
#include "Genres.h"

#include <algorithm>
#include <cmath>
#include <set>

namespace hs
{
namespace
{
int wrap12 (int x) { return ((x % 12) + 12) % 12; }

enum Fn { Tonic = 0, Pre, Dom };

// Chord "families"; the COLOR knob decides how rich each one gets.
//  'M' major   'm' minor   '7' dominant   'o' diminished   's' suspended
struct Cand { int rel; char fam; float w; };

struct Style
{
    const char* name;
    const char* blurb;
    int voicingGenre;            // borrows a HOOKSMITH genre's voicing style
    int richness;                // 0 triads ... 3 lush
    std::vector<Cand> maj[3], min[3];
    float secDom, passDim, tritone, sus, inversion, pedal;
};

const std::vector<Style>& styles()
{
    static const std::vector<Style> s = {
        { "Radio Pop", "Big, bright, singable. Four-chord anthems, sus chords and smooth inversions.", 0, 1,
          { { { 0, 'M', 3 }, { 9, 'm', 3 }, { 4, 'm', 1 } },
            { { 5, 'M', 4 }, { 2, 'm', 2 }, { 8, 'M', 0.6f } },
            { { 7, 'M', 4 }, { 7, 's', 1.2f }, { 10, 'M', 0.6f } } },
          { { { 0, 'm', 4 }, { 3, 'M', 2 }, { 8, 'M', 2 } },
            { { 5, 'm', 2 }, { 8, 'M', 3 }, { 10, 'M', 2 } },
            { { 10, 'M', 3 }, { 7, 'M', 1 }, { 7, 'm', 1 } } },
          0.15f, 0.05f, 0.0f, 0.3f, 0.3f, 0.1f },

        { "Moody Rap (OVO)", "Late-night Toronto sound: minor 9ths, major 7ths, pedal bass, Phrygian colour.", 11, 3,
          { { { 0, 'M', 2 }, { 9, 'm', 3 }, { 4, 'm', 2 } },
            { { 5, 'M', 3 }, { 2, 'm', 3 }, { 5, 'm', 1.5f } },
            { { 7, 's', 2 }, { 10, 'M', 1 }, { 4, 'm', 1 } } },
          { { { 0, 'm', 4 }, { 3, 'M', 2 }, { 8, 'M', 3 } },
            { { 5, 'm', 3 }, { 1, 'M', 1.2f }, { 10, 'M', 2 }, { 2, 'o', 0.6f } },
            { { 7, 'm', 2 }, { 10, 'M', 2 }, { 3, 'M', 1 } } },
          0.05f, 0.05f, 0.0f, 0.25f, 0.15f, 0.35f },

        { "Dark Trap", "Cold and menacing: minor triads, flat-II, harmonic-minor V, diminished tension.", 9, 0,
          { { { 0, 'M', 1 }, { 9, 'm', 4 }, { 4, 'm', 1 } },
            { { 5, 'M', 2 }, { 5, 'm', 2 }, { 2, 'm', 1 } },
            { { 7, 'M', 2 }, { 11, 'o', 1 }, { 1, 'M', 1 } } },
          { { { 0, 'm', 4 }, { 8, 'M', 2 }, { 3, 'M', 1 } },
            { { 5, 'm', 3 }, { 1, 'M', 2 }, { 2, 'o', 1 } },
            { { 7, 'M', 3 }, { 11, 'o', 1.5f }, { 7, 'm', 1 } } },
          0.05f, 0.25f, 0.0f, 0.05f, 0.05f, 0.25f },

        { "Soul Ballad", "Big-voice ballad: gospel passing chords, secondary dominants, IV to iv, walking bass.", 14, 2,
          { { { 0, 'M', 3 }, { 9, 'm', 3 }, { 4, 'm', 2 } },
            { { 5, 'M', 3 }, { 2, 'm', 3 }, { 5, 'm', 2 } },
            { { 7, '7', 4 }, { 1, '7', 0.8f }, { 11, 'o', 0.8f } } },
          { { { 0, 'm', 3 }, { 3, 'M', 2 }, { 8, 'M', 2 } },
            { { 5, 'm', 3 }, { 2, 'o', 1 }, { 8, 'M', 2 } },
            { { 7, '7', 4 }, { 1, '7', 0.6f }, { 11, 'o', 1 } } },
          0.45f, 0.3f, 0.2f, 0.25f, 0.45f, 0.05f },

        { "House / Deep House", "Club stabs: minor 7ths and 9ths, two-chord vamps, ii-V lifts.", 5, 3,
          { { { 9, 'm', 3 }, { 0, 'M', 2 }, { 4, 'm', 2 } },
            { { 2, 'm', 4 }, { 5, 'M', 2 } },
            { { 7, '7', 1.5f }, { 4, 'm', 1.5f }, { 10, 'M', 1 } } },
          { { { 0, 'm', 5 }, { 3, 'M', 1.5f } },
            { { 5, 'm', 3 }, { 10, 'M', 2 }, { 8, 'M', 1.5f } },
            { { 7, 'm', 2 }, { 10, 'M', 2 } } },
          0.15f, 0.0f, 0.05f, 0.2f, 0.1f, 0.2f },

        { "Afro / Amapiano", "Warm and bouncy: major 7ths, minor 9ths, IV-V-iii-vi movement.", 15, 2,
          { { { 0, 'M', 2 }, { 4, 'm', 3 }, { 9, 'm', 3 } },
            { { 5, 'M', 4 }, { 2, 'm', 3 } },
            { { 7, 'M', 3 }, { 7, '7', 1 }, { 4, 'm', 1 } } },
          { { { 0, 'm', 3 }, { 3, 'M', 2 }, { 8, 'M', 2 } },
            { { 5, 'm', 3 }, { 10, 'M', 2 }, { 2, 'o', 0.5f } },
            { { 7, 'm', 2 }, { 10, 'M', 2 } } },
          0.1f, 0.05f, 0.0f, 0.15f, 0.2f, 0.1f },

        { "Neo-Soul / R&B", "Lush and smooth: 9ths, 11ths, 13ths, tritone subs, chromatic slides.", 12, 3,
          { { { 0, 'M', 2 }, { 4, 'm', 3 }, { 9, 'm', 3 } },
            { { 2, 'm', 3 }, { 5, 'M', 3 }, { 5, 'm', 1.5f }, { 8, 'M', 1 } },
            { { 7, '7', 3 }, { 1, '7', 1.2f }, { 10, 'M', 1 } } },
          { { { 0, 'm', 3 }, { 3, 'M', 2 }, { 8, 'M', 2 } },
            { { 5, 'm', 3 }, { 2, 'o', 1 }, { 1, 'M', 1 } },
            { { 7, '7', 3 }, { 1, '7', 1 }, { 10, 'M', 1.5f } } },
          0.35f, 0.2f, 0.35f, 0.2f, 0.3f, 0.05f },

        { "Cinematic Pop", "Epic and emotional: borrowed bVI and bVII, suspensions, open voicings.", 4, 1,
          { { { 0, 'M', 3 }, { 9, 'm', 2 }, { 8, 'M', 1.5f } },
            { { 5, 'M', 3 }, { 8, 'M', 2 }, { 10, 'M', 2 } },
            { { 7, 's', 2 }, { 10, 'M', 2 }, { 7, 'M', 1 } } },
          { { { 0, 'm', 3 }, { 8, 'M', 3 }, { 3, 'M', 2 } },
            { { 5, 'm', 2 }, { 10, 'M', 3 }, { 8, 'M', 2 } },
            { { 10, 'M', 3 }, { 7, 'M', 1 }, { 7, 's', 1 } } },
          0.1f, 0.05f, 0.0f, 0.4f, 0.2f, 0.3f },

        { "Gangsta Rap (G-Funk)", "West Coast sun: minor 7 sus vamps, 6ths over a rolling bass, a lazy bVII. Think 2Pac and Dre.", 8, 2,
          { { { 0, 'M', 3 }, { 9, 'm', 2 } },
            { { 2, 'm', 3 }, { 5, 'M', 2 } },
            { { 7, '7', 3 }, { 10, 'M', 1 } } },
          { { { 0, 'm', 4 }, { 3, 'M', 1 } },
            { { 5, 'm', 3 }, { 8, 'M', 1.5f }, { 10, 'M', 1 } },
            { { 7, 'm', 3 }, { 10, 'M', 2 }, { 7, '7', 1 } } },
          0.1f, 0.05f, 0.05f, 0.3f, 0.35f, 0.1f },

        { "Boom Bap", "Golden-era loops: jazzy minor 7ths, dusty diminished turns, sample-style chops.", 8, 2,
          { { { 0, 'M', 2 }, { 9, 'm', 3 }, { 4, 'm', 2 } },
            { { 2, 'm', 3 }, { 5, 'M', 2 } },
            { { 7, '7', 3 }, { 1, '7', 1 } } },
          { { { 0, 'm', 4 }, { 8, 'M', 2 } },
            { { 5, 'm', 3 }, { 2, 'o', 1 }, { 1, 'M', 1 } },
            { { 7, '7', 2 }, { 7, 'm', 2 }, { 10, 'M', 1 } } },
          0.2f, 0.2f, 0.2f, 0.15f, 0.25f, 0.15f },

        { "House Rap", "Rap over a four-on-the-floor: hard minor stabs, two-chord vamps, pedal bass.", 5, 2,
          { { { 9, 'm', 3 }, { 0, 'M', 2 } },
            { { 2, 'm', 3 }, { 5, 'M', 2 } },
            { { 7, 'M', 1 }, { 4, 'm', 2 } } },
          { { { 0, 'm', 5 } },
            { { 5, 'm', 3 }, { 8, 'M', 2 }, { 1, 'M', 1 } },
            { { 10, 'M', 3 }, { 7, 'm', 2 } } },
          0.1f, 0.0f, 0.0f, 0.15f, 0.1f, 0.35f },

        { "EDM / Big Room", "Festival drops: huge triads, bVI-bVII-i lifts, sus builds that explode into the hook.", 7, 0,
          { { { 0, 'M', 3 }, { 9, 'm', 3 } },
            { { 5, 'M', 4 }, { 8, 'M', 1 } },
            { { 7, 'M', 3 }, { 7, 's', 1.5f }, { 10, 'M', 1 } } },
          { { { 0, 'm', 4 }, { 8, 'M', 2 }, { 3, 'M', 2 } },
            { { 5, 'm', 2 }, { 8, 'M', 3 }, { 10, 'M', 2 } },
            { { 10, 'M', 3 }, { 7, 'M', 1 } } },
          0.05f, 0.0f, 0.0f, 0.45f, 0.2f, 0.2f },

        { "Future Bass", "Bright and emotional: major 7ths and 9ths, wide supersaw chords, sus shimmer.", 6, 3,
          { { { 0, 'M', 2 }, { 9, 'm', 3 }, { 4, 'm', 2 } },
            { { 5, 'M', 4 }, { 2, 'm', 2 } },
            { { 7, 'M', 2 }, { 7, 's', 1.5f }, { 4, 'm', 1 } } },
          { { { 0, 'm', 3 }, { 3, 'M', 2 }, { 8, 'M', 2 } },
            { { 5, 'm', 2 }, { 8, 'M', 3 }, { 10, 'M', 2 } },
            { { 10, 'M', 3 }, { 7, 'm', 1 } } },
          0.1f, 0.05f, 0.0f, 0.4f, 0.3f, 0.1f },

        { "Melodic Trap", "Sad piano trap: minor triads and 7ths, bVI-bIII movement, a flat-II for drama.", 9, 1,
          { { { 9, 'm', 4 }, { 0, 'M', 2 } },
            { { 5, 'M', 3 }, { 2, 'm', 1 } },
            { { 7, 'M', 2 }, { 4, 'm', 1 } } },
          { { { 0, 'm', 4 }, { 8, 'M', 3 }, { 3, 'M', 1 } },
            { { 5, 'm', 3 }, { 8, 'M', 2 }, { 1, 'M', 1 } },
            { { 7, 'M', 2 }, { 10, 'M', 2 }, { 7, 'm', 1 } } },
          0.1f, 0.1f, 0.0f, 0.2f, 0.1f, 0.2f },

        { "Drill (UK / NY)", "Sliding and sinister: minor triads, flat-II, harmonic-minor V and diminished edges.", 10, 0,
          { { { 9, 'm', 4 }, { 0, 'M', 1 } },
            { { 5, 'M', 2 }, { 5, 'm', 1 } },
            { { 4, 'M', 2 }, { 8, 'M', 1 } } },
          { { { 0, 'm', 5 }, { 8, 'M', 1 } },
            { { 1, 'M', 2 }, { 5, 'm', 2 }, { 8, 'M', 2 } },
            { { 7, 'M', 3 }, { 11, 'o', 1.5f }, { 10, 'M', 1 } } },
          0.05f, 0.3f, 0.0f, 0.05f, 0.05f, 0.3f },

        { "Lo-Fi Hip Hop", "Dusty jazz keys: ii-V-I in 9ths, minor 11 vamps, tritone subs, lazy 13ths.", 13, 3,
          { { { 0, 'M', 3 }, { 4, 'm', 2 }, { 9, 'm', 2 } },
            { { 2, 'm', 4 }, { 5, 'M', 3 } },
            { { 7, '7', 3 }, { 1, '7', 1 } } },
          { { { 0, 'm', 4 }, { 3, 'M', 1 } },
            { { 5, 'm', 3 }, { 2, 'o', 1 } },
            { { 7, '7', 3 }, { 10, 'M', 1 } } },
          0.4f, 0.2f, 0.35f, 0.2f, 0.2f, 0.05f },

        { "Gospel", "Church chords: walk-ups, passing diminished, secondary dominants and big 7#9 shouts.", 14, 3,
          { { { 0, 'M', 3 }, { 9, 'm', 2 }, { 4, 'm', 2 } },
            { { 5, 'M', 4 }, { 2, 'm', 3 } },
            { { 7, '7', 3 }, { 4, '7', 1 }, { 11, 'o', 1 } } },
          { { { 0, 'm', 3 }, { 8, 'M', 2 } },
            { { 5, 'm', 3 }, { 2, 'o', 1 } },
            { { 7, '7', 4 }, { 11, 'o', 1 } } },
          0.55f, 0.45f, 0.25f, 0.3f, 0.55f, 0.05f },

        { "Reggaeton / Latin", "Perreo loops: minor i-bVI-bVII-bIII vamps, simple triads that bounce on the dembow.", 16, 0,
          { { { 9, 'm', 3 }, { 0, 'M', 2 } },
            { { 5, 'M', 3 } },
            { { 7, 'M', 3 }, { 4, 'm', 1 } } },
          { { { 0, 'm', 4 }, { 8, 'M', 3 } },
            { { 5, 'm', 2 }, { 3, 'M', 2 }, { 10, 'M', 2 } },
            { { 7, 'M', 2 }, { 10, 'M', 3 } } },
          0.05f, 0.0f, 0.0f, 0.1f, 0.05f, 0.1f },

        { "90s R&B", "Slow-jam gold: ii9-V13 turns, sus 7ths, smooth inversions and minor 9 vamps.", 11, 3,
          { { { 0, 'M', 2 }, { 4, 'm', 2 }, { 9, 'm', 3 } },
            { { 2, 'm', 4 }, { 5, 'M', 3 } },
            { { 7, '7', 2 }, { 7, 's', 2 }, { 10, 'M', 1 } } },
          { { { 0, 'm', 4 }, { 3, 'M', 2 } },
            { { 5, 'm', 3 }, { 8, 'M', 2 } },
            { { 7, 'm', 2 }, { 10, 'M', 2 }, { 7, '7', 1 } } },
          0.3f, 0.15f, 0.2f, 0.35f, 0.35f, 0.05f },

        { "Trance", "Euphoric minor: i-bVI-bIII-bVII climbs, big sustained triads, a pedal under the breakdown.", 7, 0,
          { { { 0, 'M', 3 }, { 9, 'm', 3 } },
            { { 5, 'M', 3 } },
            { { 7, 'M', 3 }, { 7, 's', 1 } } },
          { { { 0, 'm', 4 }, { 8, 'M', 2 } },
            { { 8, 'M', 3 }, { 5, 'm', 2 }, { 3, 'M', 2 } },
            { { 10, 'M', 4 }, { 7, 'M', 1 } } },
          0.0f, 0.0f, 0.0f, 0.4f, 0.15f, 0.3f },

        { "Phonk", "Memphis dark: minor triads, flat-II, harmonic-minor V7 and a bass that never leaves home.", 9, 1,
          { { { 9, 'm', 5 } },
            { { 5, 'M', 2 }, { 10, 'M', 1 } },
            { { 4, '7', 3 } } },
          { { { 0, 'm', 5 } },
            { { 5, 'm', 3 }, { 1, 'M', 2 } },
            { { 7, '7', 3 }, { 11, 'o', 1 } } },
          0.05f, 0.2f, 0.0f, 0.0f, 0.05f, 0.3f },

        { "Country", "Nashville gold: I-IV-V truth, the vi heartache, bass walkdowns and a two-five into the chorus.", 17, 0,
          { { { 0, 'M', 4 }, { 9, 'm', 1.5f } },
            { { 5, 'M', 4 }, { 2, 'm', 1.5f } },
            { { 7, 'M', 4 }, { 7, '7', 1.5f }, { 10, 'M', 0.7f } } },
          { { { 0, 'm', 4 }, { 3, 'M', 1 } },
            { { 5, 'm', 2 }, { 10, 'M', 2 }, { 8, 'M', 2 } },
            { { 7, 'M', 3 }, { 10, 'M', 1 } } },
          0.3f, 0.05f, 0.0f, 0.3f, 0.45f, 0.05f },
    };
    return s;
}

const Style& styleAt (int i)
{
    const auto& s = styles();
    return s[(size_t) std::clamp (i, 0, (int) s.size() - 1)];
}

// What job a chord does in the key (by its root, relative to the tonic)
Fn functionOf (int rel, bool minor)
{
    static const Fn maj[12] = { Tonic, Dom, Pre, Tonic, Tonic, Pre, Dom, Dom, Pre, Tonic, Pre, Dom };
    static const Fn min[12] = { Tonic, Pre, Pre, Tonic, Tonic, Pre, Dom, Dom, Pre, Tonic, Dom, Dom };
    return (minor ? min : maj)[wrap12 (rel)];
}

char familyOf (Q q)
{
    switch (q)
    {
        case Q::Min: case Q::Min7: case Q::Min9: case Q::Min11: case Q::MinAdd9: case Q::Min6: case Q::MinMaj7: return 'm';
        case Q::Dom7: case Q::Dom9: case Q::Dom13: case Q::Dom11: case Q::Sus7: case Q::Sus2_7:
        case Q::Dom7s9: case Q::Dom7b9: return '7';
        case Q::Dim: case Q::HalfDim: case Q::Dim7: return 'o';
        case Q::Sus2: case Q::Sus4: return 's';
        default: return 'M';
    }
}

Q colour (char fam, int level, bool dominantJob)
{
    level = std::clamp (level, 0, 3);
    switch (fam)
    {
        case 'm': { static const Q t[] = { Q::Min, Q::Min7, Q::Min9, Q::Min11 }; return t[level]; }
        case '7': { static const Q t[] = { Q::Dom7, Q::Dom7, Q::Dom9, Q::Dom13 }; return t[level]; }
        case 'o': return level >= 2 ? Q::Dim7 : Q::Dim;
        case 's': return dominantJob ? (level >= 2 ? Q::Sus7 : Q::Sus4) : Q::Sus2;
        default:  { static const Q t[] = { Q::Maj, Q::Add9, Q::Maj7, Q::Maj9 }; return t[level]; }
    }
}

// The "awesome" version of a chord: picks between the lush colours a producer would use there.
//   alt = how often to reach for the spicy option (FLAVOR), rel = root above the tonic
// Every note of the chord's colour (7th, 9th, 11th...) is in the key (major, or natural minor)
bool coloursInKey (Q q, int rel, bool minor)
{
    static const int majS[] = { 0, 2, 4, 5, 7, 9, 11 }, minS[] = { 0, 2, 3, 5, 7, 8, 10 };
    const auto& ivs = intervals (q);
    for (size_t i = 3; i < ivs.size() || i == 3; ++i)
    {
        if (i >= ivs.size()) break;
        const int pc = wrap12 (rel + ivs[i]);
        bool ok = false;
        for (int k = 0; k < 7; ++k) if ((minor ? minS : majS)[k] == pc) ok = true;
        if (! ok) return false;
    }
    // the 2nd (sus2 / add9) counts too
    if (q == Q::Add9 || q == Q::MinAdd9 || q == Q::Sus2 || q == Q::Maj7Sus2 || q == Q::Sus2_7 || q == Q::SixNine)
    {
        const int pc = wrap12 (rel + 2);
        bool ok = false;
        for (int k = 0; k < 7; ++k) if ((minor ? minS : majS)[k] == pc) ok = true;
        if (! ok) return false;
    }
    return true;
}

Q richColourRaw (char fam, int level, bool dominantJob, int rel, bool minor, float alt, Rng& r);

// The rich colour, stepped down until its extra notes sit in the key (dominants may bite: b9 / #9 are on purpose)
Q richColour (char fam, int level, bool dominantJob, int rel, bool minor, float alt, Rng& r)
{
    Q q = richColourRaw (fam, level, dominantJob, rel, minor, alt, r);
    if (fam == '7' || fam == 'o' || coloursInKey (q, rel, minor)) return q;
    static const Q majChain[] = { Q::Maj9, Q::Maj7, Q::Add9, Q::Six, Q::Sus2, Q::Maj };
    static const Q minChain[] = { Q::Min11, Q::Min9, Q::Min7, Q::MinAdd9, Q::Min };
    if (fam == 'm') { for (Q c : minChain) if (coloursInKey (c, rel, minor)) return c; return Q::Min; }
    if (fam == 'M') { for (Q c : majChain) if (coloursInKey (c, rel, minor)) return c; return Q::Maj; }
    if (fam == 's') return coloursInKey (Q::Sus2, rel, minor) ? Q::Sus2 : Q::Sus4;
    return q;
}

Q richColourRaw (char fam, int level, bool dominantJob, int rel, bool minor, float alt, Rng& r)
{
    level = std::clamp (level, 0, 3);
    const Q plain = colour (fam, level, dominantJob);
    if (level == 0) return plain;
    const bool spicy = r.chance (alt);
    switch (fam)
    {
        case 'M':
            if (level == 1) return spicy ? Q::Maj7 : (r.chance (0.5) ? Q::Add9 : Q::Maj7);
            if (rel == 0 && spicy) return r.chance (0.5) ? Q::SixNine : Q::Maj9;
            if ((rel == 5 || rel == 8 || rel == 1 || rel == 3) && spicy) return level >= 3 ? Q::Maj7s11 : Q::Maj9;   // lydian shine on IV, bVI, bII, bIII
            if (spicy && level >= 2) return r.chance (0.35) ? Q::Maj7Sus2 : Q::Maj9;
            return level >= 3 ? Q::Maj9 : Q::Maj7;
        case 'm':
            if (level == 1) return Q::Min7;
            if (level == 2) return spicy ? Q::Min9 : (r.chance (0.5) ? Q::Min9 : Q::Min7);
            return spicy ? Q::Min11 : Q::Min9;
        case '7':
            if (dominantJob && minor && spicy) return level >= 3 && r.chance (0.4) ? Q::Dom7s9 : Q::Dom7b9;   // harmonic-minor bite into i
            if (level == 1) return Q::Dom7;
            if (spicy && level >= 3) return r.chance (0.5) ? Q::Dom13 : (r.chance (0.5) ? Q::Dom7s9 : Q::Sus7);
            return level >= 3 ? Q::Dom13 : Q::Dom9;
        case 's':
            if (! dominantJob && level >= 2) return Q::Sus2_7;
            return plain;
        default:
            return plain;
    }
}

// FLAVOR: signature progressions (Roman numerals in the key) plus how much spice / colour it adds
struct Flavor
{
    const char* name;
    const char* blurb;
    std::vector<const char*> maj, min;
    int colourBoost;      // extra COLOR levels
    float alt;            // how often a chord gets the spicy colour
    float sigShare;       // how many of the 8 come straight from the signature bank
};

const std::vector<Flavor>& flavors()
{
    static const std::vector<Flavor> f = {
        { "Auto", "The style's own chord moves, from the HOOKSMITH progression bank.", {}, {}, 0, 0.35f, 0.45f },
        { "Soul Keys", "Piano soul: minor 7th vamps, IV-iii-ii-I walk-downs, gospel turns into the one.",
          { "IVmaj7 iii7 ii7 Imaj7", "Imaj7 vi9 ii9 V7sus4", "ii9 V13 Imaj9 vi9", "Imaj7 iii7 IVmaj7 iv6", "IVmaj7 #ivdim7 Imaj7/V vi9", "vi9 ii9 IVmaj7 V7sus4" },
          { "i7 v7", "i9 bVImaj7 iv9 v7", "i7 iv9 bVII13 bIIImaj7", "iv9 v7 i7 i7", "i9 bVImaj7 bVII6 i9", "i7 bIIImaj7 iv7 V7#9" },
          1, 0.5f, 0.6f },
        { "West Coast", "G-funk sun: minor 7 sus vamps, 6th chords over a moving bass, a lazy bVII.",
          { "Imaj7 vi7 ii7 V9", "I6 IV9 I6 V7sus4", "ii9 V9 Imaj9 Imaj9", "Imaj9 bVII9 IVmaj9 Imaj9", "vi7 ii9 vi7 V7sus4" },
          { "i7sus2 v7 i6/bIII v7", "i9 bVII9 i9 bVII9", "i7 iv9 i7 v7", "i7 bVI7 bVII9 i7", "i6 bIII6 iv9 V7b9", "i7 iv7 bVIImaj7 bIIImaj7" },
          1, 0.4f, 0.6f },
        { "Dance Kiss", "Club glow: Phrygian bII, bVI-bVII lifts and sus2 sparkle, built for a four-on-the-floor.",
          { "vi9 IVmaj9 Iadd9 Vsus2", "IVmaj7 V vi7 bVIImaj7", "Imaj7 bVImaj7 bVIImaj7 Imaj7", "vi7 IVmaj7 I V7sus4", "IVadd9 Iadd9 V vi7" },
          { "i bIImaj7 bVImaj7 bvii9", "i bVI bIII bVII", "i7 bVImaj9 bIIImaj9 bVIIsus2", "iv9 bVImaj7 bVII i", "i9 bIImaj7 i9 bVIImaj7", "bVImaj7 bVII i9 i9" },
          1, 0.45f, 0.6f },
        { "Neo Gold", "Neo-soul luxury: 9ths, 11ths, 13ths, 6/9s, tritone subs and chromatic slides.",
          { "Imaj9 bIImaj7 ii9 V13", "IVmaj9 iii7 bIII9 ii9", "ii9 bII7 Imaj9 vi11", "Imaj7#11 vi11 ii9 V7#9", "iii7 VI7b9 ii9 V13", "Imaj9 iv9 iii7 bIII9" },
          { "i11 iv9 bVII13 bIIImaj9", "i9 bII7 i9 bVImaj7#11", "iv11 bVII13 bIII69 bVImaj7", "i11 iim7b5 V7b9 i11", "bVImaj9 v9 iv11 i9" },
          2, 0.7f, 0.65f },
        { "Dark Trap", "Cold menace: flat-II, harmonic-minor V7b9, diminished passing tension, bare triads.",
          { "vi bVI IV V", "vi IV bIImaj7 V", "I bVI bVII I", "vi iii IV bII" },
          { "i bII i bVI", "i bVI bII V7b9", "i iv bII V", "i viidim7 bVI V", "i bIImaj7 iv V7b9", "i i bVI viidim7" },
          -1, 0.3f, 0.6f },
        { "Gospel Lift", "Church chords: passing diminished, 7#9 shouts, IV to #iv dim, big walk-ups.",
          { "IVmaj7 #ivdim7 Imaj7/V VI7#9", "ii9 iii7 IVmaj7 #ivdim7", "I6 III7#9 vi9 V7sus4", "I I7 IV iv6", "ii9 V13 iii7 VI7b9", "IVmaj9 V13 iii7 vi9" },
          { "bVImaj7 V7#9 i9 i9", "i9 iv9 #ivdim7 V7b9", "iv9 bVII13 bIIImaj9 bVI7", "i7 IV9 bVImaj7 V7#9", "iv9 v7 i9 VI7#9" },
          2, 0.6f, 0.6f },
        { "Cinema", "Epic lift: borrowed bVI and bVII, sus2 shimmer, the IV to iv heartbreak.",
          { "I bVI bVII I", "IV iv6 I I", "vi IV Iadd9 Vsus4", "Iadd9 bVIIadd9 IVadd9 iv6", "IV I/III ii Vsus4" },
          { "i bVI bIII bVII", "isus2 bVI iv bVII", "bVI bVII i i", "i bIII bVII iv", "i bVI bIIIadd9 bVIIsus2" },
          0, 0.35f, 0.55f },
        { "Lo-Fi Jazz", "Dusty jazz keys: ii-V-I in 9ths, minor 11 vamps, lazy 13ths.",
          { "ii9 V13 Imaj9 vi9", "Imaj9 vi9 ii9 V7b9", "iii7 vi9 ii9 V13", "IVmaj9 iii7 ii9 bII7", "Imaj9 VI7b9 ii9 V13" },
          { "i11 iv9 i11 iv9", "iim7b5 V7b9 i9 i9", "i9 bVImaj7 iim7b5 V7#9", "iv9 bVII13 i11 i11", "i9 iv11 bVII13 bIIImaj9" },
          2, 0.65f, 0.6f },
    };
    return f;
}

const Flavor& flavorAt (int i)
{
    const auto& f = flavors();
    return f[(size_t) std::clamp (i, 0, (int) f.size() - 1)];
}

// The progressions a variation can be built from: the flavour's bank, or (Auto) the HOOKSMITH genre bank
std::vector<std::pair<std::string, std::string>> signatureBank (const Flavor& fl, int genre, bool minor)
{
    std::vector<std::pair<std::string, std::string>> out;   // name, chords
    if (! fl.maj.empty())
    {
        for (auto* c : minor ? fl.min : fl.maj) out.push_back ({ fl.name, c });
        return out;
    }
    const auto& gs = detail::genres();
    const auto& g = gs[(size_t) std::clamp (genre, 0, (int) gs.size() - 1)];
    for (auto& p : g.progs) if (p.minor == minor) out.push_back ({ p.name, p.chords });
    return out;
}

std::vector<std::string> splitWords (const std::string& s)
{
    std::vector<std::string> v;
    std::string cur;
    for (char ch : s)
    {
        if (ch == ' ') { if (! cur.empty()) v.push_back (cur); cur.clear(); }
        else cur += ch;
    }
    if (! cur.empty()) v.push_back (cur);
    return v;
}

std::string chordLabel (int rootPc, Q q, int bassPc, bool flats)
{
    std::string n = noteName (rootPc, flats) + qualitySuffix (q);
    if (bassPc >= 0 && bassPc != wrap12 (rootPc)) n += "/" + noteName (bassPc, flats);
    return n;
}

std::string romanOf (int rel, Q q)
{
    static const char* up[] = { "I", "bII", "II", "bIII", "III", "IV", "#IV", "V", "bVI", "VI", "bVII", "VII" };
    std::string r = up[wrap12 (rel)];
    const char f = familyOf (q);
    if (f == 'm' || f == 'o')
        for (auto& ch : r) if (ch == 'I' || ch == 'V') ch = (char) std::tolower ((unsigned char) ch);
    if (f == 'o') r += "dim";
    if (f == '7') r += "7";
    return r;
}

uint64_t seedFor (uint32_t seed, uint64_t a, uint64_t b = 0)
{
    return mix64 (mix64 ((uint64_t) seed + 0x51ED27ull) ^ (a * 0x9E3779B97F4A7C15ull) ^ (b * 0xC2B2AE3D27D4EB4Full));
}

// Voice the chords smoothly; put the (slash) bass underneath.
void voice (std::vector<GenChord>& chords, const Style& st, bool addBass, bool flats, int tonic, bool minor)
{
    std::vector<int> prev;
    for (auto& c : chords)
    {
        if (addBass)
        {
            // "melodic chords": the bass note low, the chord spread an octave and more above it
            c.notes = voiceChord (c.rootPc, intervals (c.q), st.voicingGenre, prev, 64, true, c.bassPc);
            const int bpc = c.bassPc >= 0 ? c.bassPc : c.rootPc;
            if (c.notes.empty() || wrap12 (c.notes.front()) != wrap12 (bpc))
            {
                int b = 36 + wrap12 (bpc - 36);
                while (! c.notes.empty() && b > c.notes.front() - 5) b -= 12;
                c.notes.insert (c.notes.begin(), b);
            }
        }
        else c.notes = voiceChord (c.rootPc, intervals (c.q), st.voicingGenre, prev, 64);
        // the voice leading follows the chord on top, not the bass
        prev.assign (c.notes.begin() + (addBass && c.notes.size() > 3 ? 1 : 0), c.notes.end());
        const int rel = wrap12 (c.rootPc - tonic);
        const bool borrowedFlat = rel == 1 || rel == 3 || rel == 8 || rel == 10;   // bII, bIII, bVI, bVII are spelled with flats
        c.name = chordLabel (c.rootPc, c.q, c.bassPc, flats || (borrowedFlat && familyOf (c.q) != 'o'));
        c.roman = romanOf (rel, c.q);
        (void) minor;
    }
}

bool keyFlats (int tonic, bool minor)
{
    return keyPrefersFlats (tonic, minor);
}

const Cand& pick (const std::vector<Cand>& v, Rng& r, int avoidRel)
{
    float total = 0;
    for (auto& c : v) total += c.rel == avoidRel ? c.w * 0.15f : c.w;
    float x = (float) r.uni() * total;
    for (auto& c : v)
    {
        const float w = c.rel == avoidRel ? c.w * 0.15f : c.w;
        if (x < w) return c;
        x -= w;
    }
    return v.back();
}

std::string signature (const std::vector<GenChord>& v)
{
    std::string s;
    for (auto& c : v) s += std::to_string (c.rootPc) + qualitySuffix (c.q) + "/" + std::to_string (c.bassPc) + "@" + std::to_string ((int) std::lround (c.start * 4)) + " ";
    return s;
}
} // namespace

const std::vector<std::string>& styleNames()
{
    static const std::vector<std::string> n = [] { std::vector<std::string> v; for (auto& s : styles()) v.push_back (s.name); return v; }();
    return n;
}

const std::vector<std::string>& styleBlurbs()
{
    static const std::vector<std::string> n = [] { std::vector<std::string> v; for (auto& s : styles()) v.push_back (s.blurb); return v; }();
    return n;
}

Variation originalVariation (const ImportedProgression& src, const GenSettings& s)
{
    Variation v;
    v.label = "ORIGINAL";
    for (auto& sp : src.spans)
    {
        GenChord c;
        c.start = sp.start; c.len = sp.len; c.rootPc = sp.rootPc; c.q = sp.q;
        c.notes = sp.pitches;
        c.name = chordLabel (sp.rootPc, sp.q, -1, keyFlats (src.tonicPc, src.minor));
        c.roman = romanOf (wrap12 (sp.rootPc - src.tonicPc), sp.q);
        v.chords.push_back (c);
    }
    (void) s;
    return v;
}

std::vector<Variation> generateVariations (const ImportedProgression& src, const GenSettings& s, int count, int firstIndex)
{
    std::vector<Variation> out;
    if (src.spans.empty()) return out;

    const Style& st = styleAt (s.style);
    const Flavor& fl = flavorAt (s.flavor);
    const bool minor = src.minor;
    const int tonic = src.tonicPc;
    const int level = std::clamp (st.richness + fl.colourBoost + (int) std::lround ((s.color - 0.5f) * 3.0f), 0, 3);
    const float alt = std::clamp (fl.alt * (0.5f + s.color), 0.0f, 0.95f);
    // the style's chord vocabulary, plus the flavour's favourite borrowed chords
    std::vector<Cand> vocab[3];
    for (int f = 0; f < 3; ++f) vocab[f] = (minor ? st.min : st.maj)[f];
    {
        static const std::vector<Cand> extraMaj[3] = { { { 4, 'm', 1 } }, { { 5, 'm', 1 }, { 8, 'M', 1 } }, { { 10, 'M', 1 }, { 1, '7', 0.5f } } };
        static const std::vector<Cand> extraMin[3] = { { { 3, 'M', 1 } }, { { 1, 'M', 1 }, { 8, 'M', 1 } }, { { 7, '7', 1.5f }, { 11, 'o', 0.6f } } };
        if (s.flavor > 0)
            for (int f = 0; f < 3; ++f)
                for (auto c : (minor ? extraMin : extraMaj)[f]) { c.w *= 1.0f + alt; vocab[f].push_back (c); }
    }
    const auto bank = signatureBank (fl, st.voicingGenre, minor);
    const int n = (int) src.spans.size();

    std::set<std::string> seen;
    {
        std::vector<GenChord> orig;
        for (auto& sp : src.spans) { GenChord c; c.start = sp.start; c.len = sp.len; c.rootPc = sp.rootPc; c.q = sp.q; orig.push_back (c); }
        seen.insert (signature (orig));
    }
    std::set<size_t> usedBank;

    for (int vi = firstIndex; vi < firstIndex + count; ++vi)
    {
        Variation best;
        for (int attempt = 0; attempt < 12; ++attempt)
        {
            Rng r (seedFor (s.seed + (uint32_t) s.flavor * 7919u, (uint64_t) vi * 31 + 7, (uint64_t) attempt));
            std::vector<GenChord> seq;
            std::vector<std::string> tags;
            int prevRel = -1;

            // 1a. a signature progression from the bank, laid over the original's chord changes
            const float share = std::clamp (fl.sigShare * std::clamp ((1.0f - s.similarity) * 1.6f, 0.0f, 1.2f), 0.0f, 0.9f);
            const bool useBank = ! bank.empty() && (vi % 2 == 0 ? r.chance (std::min (1.0f, share * 1.4f)) : r.chance (share * 0.6f));
            if (useBank)
            {
                size_t bi = (size_t) r.range (0, (int) bank.size() - 1);
                for (int k = 0; k < (int) bank.size() && usedBank.count (bi) && attempt < 6; ++k) bi = (bi + 1) % bank.size();
                const auto words = splitWords (bank[bi].second);
                std::vector<ParsedChord> pcs;
                for (auto& w : words) { auto pc = parseRoman (w); if (pc.ok) pcs.push_back (pc); }
                if (! pcs.empty())
                {
                    const int m = (int) pcs.size();
                    // where the new chords go: on the original's changes, or (more new chords than old) split them evenly
                    std::vector<std::pair<double, double>> slots;
                    if (m > n && m % n == 0)
                    {
                        const int per = m / n;
                        for (auto& sp : src.spans) for (int k = 0; k < per; ++k) slots.push_back ({ sp.start + sp.len * k / per, sp.len / per });
                    }
                    else for (auto& sp : src.spans) slots.push_back ({ sp.start, sp.len });
                    for (size_t i = 0; i < slots.size(); ++i)
                    {
                        const auto& pc = pcs[i % (size_t) m];
                        GenChord c;
                        c.start = slots[i].first; c.len = slots[i].second;
                        c.rootPc = wrap12 (tonic + pc.rootRel);
                        const char fam = familyOf (pc.q);
                        const bool dom = functionOf (pc.rootRel, minor) == Dom;
                        if (! pc.explicitQ || (level >= 2 && (pc.q == Q::Maj || pc.q == Q::Min)))
                            c.q = richColour (fam, level, dom, pc.rootRel, minor, alt, r);
                        else if (s.color < 0.2f) c.q = colour (fam, 0, dom);   // COLOR all the way down = plain triads
                        else c.q = pc.q;
                        if (pc.bassRel >= 0) c.bassPc = wrap12 (tonic + pc.bassRel);
                        // SIMILARITY keeps some of your own chords
                        const auto* orig = &src.spans[0];
                        for (auto& sp : src.spans) if (sp.start <= c.start + 1.0e-6) orig = &sp;
                        if (r.chance (s.similarity * 0.5) && std::abs (orig->start - c.start) < 1.0e-6)
                        {
                            c.rootPc = orig->rootPc;
                            c.q = richColour (familyOf (orig->q), level, functionOf (wrap12 (orig->rootPc - tonic), minor) == Dom,
                                              wrap12 (orig->rootPc - tonic), minor, alt, r);
                            c.bassPc = -1;
                        }
                        c.changed = ! (wrap12 (c.rootPc - orig->rootPc) == 0 && familyOf (c.q) == familyOf (orig->q));
                        seq.push_back (c);
                    }
                    tags.push_back (bank[bi].first);
                    if (attempt < 6) usedBank.insert (bi);
                }
            }

            // 1b. choose a chord for every chord of the original (same timing)
            const bool fromBank = ! seq.empty();
            for (size_t i = 0; ! fromBank && i < src.spans.size(); ++i)
            {
                const auto& sp = src.spans[i];
                const int rel = wrap12 (sp.rootPc - tonic);
                const Fn fn = functionOf (rel, minor);
                GenChord c;
                c.start = sp.start; c.len = sp.len;

                const bool keep = r.chance (s.similarity);
                if (keep)
                {
                    c.rootPc = sp.rootPc;
                    c.q = richColour (familyOf (sp.q), level, fn == Dom, rel, minor, alt, r);
                }
                else
                {
                    // same job in the key, or (when the similarity is low) any job
                    Fn useFn = fn;
                    if (r.chance (0.35 * (1.0 - s.similarity))) useFn = (Fn) r.range (0, 2);
                    if (i == 0 && r.chance (0.6)) useFn = Tonic;
                    const Cand& cd = pick (vocab[useFn], r, prevRel);
                    c.rootPc = wrap12 (tonic + cd.rel);
                    c.q = richColour (cd.fam, level, useFn == Dom, cd.rel, minor, alt, r);
                    c.changed = true;
                }
                if (! keep && wrap12 (c.rootPc - sp.rootPc) == 0 && familyOf (c.q) == familyOf (sp.q)) c.changed = false;
                prevRel = wrap12 (c.rootPc - tonic);
                seq.push_back (c);
            }
            // the last chord turns back home: a dominant into a minor tonic gets its bite
            if (seq.size() > 1 && wrap12 (src.spans[0].rootPc - tonic) == 0)
            {
                auto& last = seq.back();
                if (wrap12 (last.rootPc - tonic) == 7 && minor && familyOf (last.q) == 'm' && r.chance (alt * 0.8))
                {
                    last.q = level >= 3 && r.chance (0.4) ? Q::Dom7s9 : Q::Dom7b9; last.changed = true;
                    tags.push_back ("harmonic-minor V");
                }
            }

            const float sp = useBank ? s.spice * 0.5f : s.spice;   // signature progressions are already spicy
            // 2. tritone subs on dominant 7ths
            for (auto& c : seq)
                if (familyOf (c.q) == '7' && wrap12 (c.rootPc - tonic) == 7 && r.chance (sp * st.tritone * 1.6))
                {
                    c.rootPc = wrap12 (tonic + 1); c.changed = true;
                    if (level >= 2) c.q = Q::Dom9;
                    tags.push_back ("tritone sub");
                }

            // 3. spice moves that add chords inside long chords
            std::vector<GenChord> spiced;
            for (size_t i = 0; i < seq.size(); ++i)
            {
                GenChord c = seq[i];
                const GenChord* next = i + 1 < seq.size() ? &seq[i + 1] : (seq.size() > 1 ? &seq[0] : nullptr);
                const double moveLen = c.len >= 4.0 ? 1.0 : c.len >= 2.0 ? 0.5 * c.len : 0.0;

                if (next != nullptr && moveLen > 0 && next->rootPc != c.rootPc)
                {
                    if (r.chance (sp * st.secDom * 1.4))
                    {
                        GenChord d; d.start = c.start + c.len - moveLen; d.len = moveLen;
                        d.rootPc = wrap12 (next->rootPc + 7); d.q = richColour ('7', level, true, wrap12 (d.rootPc - tonic), familyOf (next->q) == 'm', alt, r); d.changed = true;
                        c.len -= moveLen;
                        spiced.push_back (c); spiced.push_back (d);
                        tags.push_back ("secondary dominant");
                        continue;
                    }
                    if (r.chance (sp * st.passDim * 1.4))
                    {
                        GenChord d; d.start = c.start + c.len - moveLen; d.len = moveLen;
                        d.rootPc = wrap12 (next->rootPc - 1); d.q = Q::Dim7; d.changed = true;
                        c.len -= moveLen;
                        spiced.push_back (c); spiced.push_back (d);
                        tags.push_back ("passing diminished");
                        continue;
                    }
                }
                const char fam = familyOf (c.q);
                if ((fam == 'M' || fam == '7') && c.len >= 2.0 && r.chance (sp * st.sus * 1.4))
                {
                    GenChord a = c; a.len = c.len * 0.5; a.q = fam == '7' && level >= 2 ? Q::Sus7 : Q::Sus4; a.changed = true;
                    GenChord b = c; b.start = c.start + a.len; b.len = c.len - a.len;
                    spiced.push_back (a); spiced.push_back (b);
                    tags.push_back ("sus resolve");
                    continue;
                }
                // borrowed iv after a major IV (the classic soul / pop move)
                if (! minor && fam == 'M' && wrap12 (c.rootPc - tonic) == 5 && c.len >= 2.0 && r.chance (sp * 0.5))
                {
                    GenChord a = c; a.len = c.len * 0.5;
                    GenChord b = c; b.start = c.start + a.len; b.len = c.len - a.len; b.q = level >= 1 ? Q::Min6 : Q::Min; b.changed = true;
                    spiced.push_back (a); spiced.push_back (b);
                    tags.push_back ("borrowed iv");
                    continue;
                }
                spiced.push_back (c);
            }

            // 4. bass: inversions for a stepwise line, or a pedal under moody chords
            int prevBass = -1;
            for (auto& c : spiced)
            {
                if (c.bassPc >= 0) { prevBass = c.bassPc; continue; }   // a slash chord from the bank keeps its bass
                const int root = wrap12 (c.rootPc);
                int bass = root;
                if (prevBass >= 0 && r.chance (s.bassMotion * st.inversion * 2.2))
                {
                    int bestD = 12;
                    for (int iv : intervals (c.q))
                    {
                        if (iv != 3 && iv != 4 && iv != 7) continue;
                        const int pc = wrap12 (root + iv);
                        int d = std::abs (pc - prevBass); d = std::min (d, 12 - d);
                        if (d >= 1 && d <= 2 && d < bestD) { bestD = d; bass = pc; }
                    }
                    if (bass != root) tags.push_back ("inversion");
                }
                else if (r.chance (s.bassMotion * st.pedal * 2.2) && root != tonic)
                {
                    bool tonicInChord = false;
                    for (int iv : intervals (c.q)) if (wrap12 (root + iv) == tonic) tonicInChord = true;
                    if (tonicInChord || st.pedal >= 0.25f) { bass = tonic; tags.push_back ("pedal bass"); }
                }
                c.bassPc = bass == root ? -1 : bass;
                prevBass = bass;
            }

            // merge a chord that repeats straight after itself
            std::vector<GenChord> merged;
            for (auto& c : spiced)
            {
                if (! merged.empty() && merged.back().rootPc == c.rootPc && merged.back().q == c.q && merged.back().bassPc == c.bassPc)
                    merged.back().len += c.len;
                else merged.push_back (c);
            }

            Variation v;
            v.chords = merged;
            const auto sig = signature (v.chords);
            bool anyChange = false;
            for (auto& c : v.chords) anyChange = anyChange || c.changed || c.bassPc >= 0;
            std::sort (tags.begin(), tags.end());
            tags.erase (std::unique (tags.begin(), tags.end()), tags.end());
            for (auto& t : tags) v.tags += (v.tags.empty() ? "" : ", ") + t;
            best = v;
            if (anyChange && ! seen.count (sig)) { seen.insert (sig); break; }
        }
        best.label = vi < 26 ? std::string (1, (char) ('A' + vi)) : std::to_string (vi + 1);
        voice (best.chords, st, s.addBass, keyFlats (tonic, minor), tonic, minor);
        out.push_back (best);
    }
    return out;
}

std::vector<NoteEv> renderVariation (const ImportedProgression& src, const Variation& v, bool keepRhythm)
{
    std::vector<NoteEv> out;
    auto chordAt = [&] (double t) -> const GenChord*
    {
        const GenChord* c = nullptr;
        for (auto& ch : v.chords) if (t + 1.0e-6 >= ch.start) c = &ch;
        return c;
    };

    if (keepRhythm && ! src.notes.empty())
    {
        // the original's hits: groups of notes that start together
        std::vector<std::pair<double, std::pair<double, int>>> hits;   // start, (length, velocity)
        for (auto& n : src.notes)
        {
            bool merged = false;
            for (auto& h : hits)
                if (std::abs (h.first - n.start) < 0.06)
                {
                    h.second.first = std::max (h.second.first, n.len);
                    h.second.second = std::max (h.second.second, n.vel);
                    merged = true;
                }
            if (! merged) hits.push_back ({ n.start, { n.len, n.vel } });
        }
        std::sort (hits.begin(), hits.end());
        // a chord change with no hit on it still gets a new attack
        for (auto& ch : v.chords)
        {
            bool has = false;
            for (auto& h : hits) if (std::abs (h.first - ch.start) < 0.06) has = true;
            if (! has) hits.push_back ({ ch.start, { ch.len, 90 } });   // a chord change always speaks
        }
        std::sort (hits.begin(), hits.end());
        const double loopEnd = std::max (src.lengthBeats, v.chords.empty() ? 0.0 : v.chords.back().start + v.chords.back().len);
        for (size_t i = 0; i < hits.size(); ++i)
        {
            const auto& h = hits[i];
            const auto* ch = chordAt (h.first);
            if (ch == nullptr) continue;
            // legato: every hit rings right up to the next one (no silent gaps); LENGTH shortens it later
            const double next = i + 1 < hits.size() ? hits[i + 1].first : loopEnd;
            double len = std::max (0.05, next - h.first);
            for (int p : ch->notes) out.push_back ({ h.first, len, p, h.second.second });
        }
    }
    else
    {
        for (auto& ch : v.chords)
            for (int p : ch.notes) out.push_back ({ ch.start, std::max (0.05, ch.len), p, 96 });
    }
    std::sort (out.begin(), out.end(), [] (const NoteEv& a, const NoteEv& b) { return a.start < b.start; });
    return out;
}

const std::vector<std::string>& flavorNames()
{
    static const std::vector<std::string> n = [] { std::vector<std::string> v; for (auto& f : flavors()) v.push_back (f.name); return v; }();
    return n;
}

const std::vector<std::string>& flavorBlurbs()
{
    static const std::vector<std::string> n = [] { std::vector<std::string> v; for (auto& f : flavors()) v.push_back (f.blurb); return v; }();
    return n;
}

namespace
{
// GATE rhythms: 16 steps per bar ('x' = hit), or a straight rate in beats
struct Gate { const char* name; const char* steps; double rate; };
const std::vector<Gate>& gates()
{
    static const std::vector<Gate> g = {
        { "Off",        nullptr, 0 },
        { "1/4",        nullptr, 1.0 },
        { "1/8",        nullptr, 0.5 },
        { "1/8 Dotted", nullptr, 0.75 },
        { "1/8 Triplet", nullptr, 1.0 / 3.0 },
        { "1/16",       nullptr, 0.25 },
        { "Offbeat",    "..x...x...x...x.", 0 },
        { "Bounce",     "x..x..x...x..x..", 0 },
        { "Skip",       "x.xx.x.xx.x.xx.x", 0 },
        { "House Stab", "x..x..x.x..x..x.", 0 },
        { "Trance",     "xx.xx.x.xx.xx.x.", 0 },
        { "Pump",       "x.x.x.x.x.x.x.xx", 0 },
    };
    return g;
}
} // namespace

const std::vector<std::string>& gateNames()
{
    static const std::vector<std::string> n = [] { std::vector<std::string> v; for (auto& g : gates()) v.push_back (g.name); return v; }();
    return n;
}

std::vector<NoteEv> applyGate (const std::vector<NoteEv>& notes, int gate, float length, double loopLen)
{
    const auto& gs = gates();
    const Gate& g = gs[(size_t) std::clamp (gate, 0, (int) gs.size() - 1)];
    length = std::clamp (length, 0.1f, 2.0f);
    std::vector<NoteEv> out;
    if (loopLen <= 0) loopLen = 16;

    // the gate's hit times across the loop
    std::vector<double> hits;
    if (g.steps != nullptr)
    {
        for (double bar = 0; bar < loopLen - 1.0e-9; bar += 4.0)
            for (int k = 0; k < 16; ++k) if (g.steps[k] == 'x') hits.push_back (bar + k * 0.25);
    }
    else if (g.rate > 0)
        for (double t = 0; t < loopLen - 1.0e-9; t += g.rate) hits.push_back (t);

    for (auto& nt : notes)
    {
        const double end = nt.start + nt.len;
        if (hits.empty())
        {
            NoteEv e = nt;
            e.len = std::max (0.03, nt.len * (double) length);
            out.push_back (e);
            continue;
        }
        // a chord change always speaks, then the gate chops the rest of it
        std::vector<double> ts { nt.start };
        for (double h : hits) if (h > nt.start + 0.06 && h < end - 0.03) ts.push_back (h);
        for (size_t i = 0; i < ts.size(); ++i)
        {
            const auto nextHit = std::upper_bound (hits.begin(), hits.end(), ts[i] + 1.0e-6);
            const double gap = nextHit != hits.end() ? *nextHit - ts[i] : (hits.front() + loopLen) - ts[i];
            NoteEv e = nt;
            e.start = ts[i];
            e.len = gap * (double) length;
            if (length <= 1.0f) e.len = std::min (e.len, end - ts[i]);   // stays inside its chord
            e.len = std::max (0.03, e.len);
            if (i > 0) e.vel = std::max (1, (int) std::lround (nt.vel * 0.86));
            out.push_back (e);
        }
    }
    std::sort (out.begin(), out.end(), [] (const NoteEv& a, const NoteEv& b) { return a.start < b.start || (a.start == b.start && a.pitch < b.pitch); });
    // never overlap the same note (stop just before it plays again, wrapping round the loop)
    for (size_t i = 0; i < out.size(); ++i)
    {
        double nextSame = -1;
        for (size_t j = i + 1; j < out.size(); ++j) if (out[j].pitch == out[i].pitch && out[j].start > out[i].start + 1.0e-6) { nextSame = out[j].start; break; }
        if (nextSame < 0)
            for (auto& o : out) if (o.pitch == out[i].pitch) { nextSame = o.start + loopLen; break; }
        out[i].len = std::max (0.03, std::min (out[i].len, nextSame - out[i].start));   // ends exactly as it plays again: no gap
        out[i].len = std::min (out[i].len, loopLen - out[i].start + (nextSame > loopLen ? nextSame - loopLen : 0) );
    }
    return out;
}
} // namespace hs
