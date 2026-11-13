#include "Reharm.h"

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
        case Q::Dom7: case Q::Dom9: case Q::Dom13: case Q::Dom11: case Q::Sus7: return '7';
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
        c.notes = voiceChord (c.rootPc, intervals (c.q), st.voicingGenre, prev, 64);
        prev = c.notes;
        if (addBass)
        {
            const int bpc = c.bassPc >= 0 ? c.bassPc : c.rootPc;
            int b = 36 + wrap12 (bpc - 36);                 // C2..B2
            while (! c.notes.empty() && b > c.notes.front() - 5) b -= 12;
            c.notes.insert (c.notes.begin(), b);
        }
        const int rel = wrap12 (c.rootPc - tonic);
        const bool borrowedFlat = rel == 1 || rel == 3 || rel == 8 || rel == 10;   // bII, bIII, bVI, bVII are spelled with flats
        c.name = chordLabel (c.rootPc, c.q, c.bassPc, flats || (borrowedFlat && familyOf (c.q) != 'o'));
        c.roman = romanOf (wrap12 (c.rootPc - tonic), c.q);
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

std::vector<Variation> generateVariations (const ImportedProgression& src, const GenSettings& s, int count)
{
    std::vector<Variation> out;
    if (src.spans.empty()) return out;

    const Style& st = styleAt (s.style);
    const bool minor = src.minor;
    const int tonic = src.tonicPc;
    const int level = std::clamp (st.richness + (int) std::lround ((s.color - 0.5f) * 3.0f), 0, 3);
    const auto& vocab = minor ? st.min : st.maj;

    std::set<std::string> seen;
    {
        std::vector<GenChord> orig;
        for (auto& sp : src.spans) { GenChord c; c.start = sp.start; c.len = sp.len; c.rootPc = sp.rootPc; c.q = sp.q; orig.push_back (c); }
        seen.insert (signature (orig));
    }

    for (int vi = 0; vi < count; ++vi)
    {
        Variation best;
        for (int attempt = 0; attempt < 12; ++attempt)
        {
            Rng r (seedFor (s.seed, (uint64_t) vi * 31 + 7, (uint64_t) attempt));
            std::vector<GenChord> seq;
            std::vector<std::string> tags;
            int prevRel = -1;

            // 1. choose a chord for every chord of the original (same timing)
            for (size_t i = 0; i < src.spans.size(); ++i)
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
                    const char fam = familyOf (sp.q);
                    c.q = colour (fam, level, fn == Dom);
                }
                else
                {
                    // same job in the key, or (when the similarity is low) any job
                    Fn useFn = fn;
                    if (r.chance (0.35 * (1.0 - s.similarity))) useFn = (Fn) r.range (0, 2);
                    if (i == 0 && r.chance (0.6)) useFn = Tonic;
                    const Cand& cd = pick (vocab[useFn], r, prevRel);
                    c.rootPc = wrap12 (tonic + cd.rel);
                    c.q = colour (cd.fam, level, useFn == Dom);
                    c.changed = true;
                }
                if (! keep && wrap12 (c.rootPc - sp.rootPc) == 0 && familyOf (c.q) == familyOf (sp.q)) c.changed = false;
                prevRel = wrap12 (c.rootPc - tonic);
                seq.push_back (c);
            }

            // 2. tritone subs on dominant 7ths
            for (auto& c : seq)
                if (familyOf (c.q) == '7' && wrap12 (c.rootPc - tonic) == 7 && r.chance (s.spice * st.tritone * 1.6))
                {
                    c.rootPc = wrap12 (tonic + 1); c.changed = true;
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
                    if (r.chance (s.spice * st.secDom * 1.4))
                    {
                        GenChord d; d.start = c.start + c.len - moveLen; d.len = moveLen;
                        d.rootPc = wrap12 (next->rootPc + 7); d.q = colour ('7', level, true); d.changed = true;
                        c.len -= moveLen;
                        spiced.push_back (c); spiced.push_back (d);
                        tags.push_back ("secondary dominant");
                        continue;
                    }
                    if (r.chance (s.spice * st.passDim * 1.4))
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
                if ((fam == 'M' || fam == '7') && c.len >= 2.0 && r.chance (s.spice * st.sus * 1.4))
                {
                    GenChord a = c; a.len = c.len * 0.5; a.q = Q::Sus4; a.changed = true;
                    GenChord b = c; b.start = c.start + a.len; b.len = c.len - a.len;
                    spiced.push_back (a); spiced.push_back (b);
                    tags.push_back ("sus resolve");
                    continue;
                }
                // borrowed iv after a major IV (the classic soul / pop move)
                if (! minor && fam == 'M' && wrap12 (c.rootPc - tonic) == 5 && c.len >= 2.0 && r.chance (s.spice * 0.5))
                {
                    GenChord a = c; a.len = c.len * 0.5;
                    GenChord b = c; b.start = c.start + a.len; b.len = c.len - a.len; b.q = colour ('m', level, false); b.changed = true;
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
        best.label = std::string (1, (char) ('A' + vi));
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
            if (! has)
            {
                bool sounding = false;
                for (auto& h : hits) if (h.first < ch.start && h.first + h.second.first > ch.start + 0.05) sounding = true;
                if (sounding) hits.push_back ({ ch.start, { ch.len, 90 } });
            }
        }
        std::sort (hits.begin(), hits.end());
        for (auto& h : hits)
        {
            const auto* ch = chordAt (h.first);
            if (ch == nullptr) continue;
            double len = std::min (h.second.first, ch->start + ch->len - h.first);   // stop at the next chord
            if (len < 0.05) len = 0.05;
            for (int p : ch->notes) out.push_back ({ h.first, len, p, h.second.second });
        }
    }
    else
    {
        for (auto& ch : v.chords)
            for (int p : ch.notes) out.push_back ({ ch.start, std::max (0.05, ch.len - 0.02), p, 96 });
    }
    std::sort (out.begin(), out.end(), [] (const NoteEv& a, const NoteEv& b) { return a.start < b.start; });
    return out;
}
} // namespace hs
