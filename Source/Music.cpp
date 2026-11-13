#include "Music.h"
#include "Genres.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <functional>
#include <cctype>

namespace hs
{
using namespace detail;

// =============================================================================
// Lists
// =============================================================================
const std::vector<std::string>& genreNames()
{
    static const std::vector<std::string> n = []
    {
        std::vector<std::string> v;
        for (auto& g : genres()) v.push_back (g.name);
        return v;
    }();
    return n;
}

const std::vector<std::string>& sectionNames()
{
    static const std::vector<std::string> n { "Intro", "Verse", "Pre-Chorus", "Hook", "Bridge", "Outro" };
    return n;
}

const std::vector<std::string>& scaleNames()
{
    static const std::vector<std::string> n { "Auto", "Major", "Minor", "Dorian", "Phrygian", "Lydian", "Mixolydian", "Harmonic Minor" };
    return n;
}

const std::vector<std::string>& keyNames()
{
    static const std::vector<std::string> n { "C", "C#", "D", "Eb", "E", "F", "F#", "G", "Ab", "A", "Bb", "B" };
    return n;
}

const std::vector<std::string>& feelNames()
{
    static const std::vector<std::string> n { "Genre Default", "Quantized", "Tight", "Laid Back", "Pushed", "Dilla",
                                              "Shuffle", "Bounce", "Afro / Dembow", "Live Drummer", "Rubato" };
    return n;
}

const std::vector<std::string>& laneNames()
{
    static const std::vector<std::string> n { "Chords", "Melody", "Bass", "Drums" };
    return n;
}

int genreDefaultBpm (int genre)
{
    const auto& g = genres();
    return g[(size_t) std::clamp (genre, 0, (int) g.size() - 1)].bpm;
}

std::vector<std::string> progressionNames (int genre)
{
    const auto& g = genres()[(size_t) std::clamp (genre, 0, (int) genres().size() - 1)];
    std::vector<std::string> v;
    for (auto& p : g.progs)
        v.push_back (std::string (p.name) + "  (" + p.chords + ")");
    return v;
}

// =============================================================================
// Chord theory
// =============================================================================
const std::vector<int>& intervals (Q q)
{
    static const std::vector<int> t[] = {
        { 0, 4, 7 }, { 0, 3, 7 }, { 0, 3, 6 }, { 0, 4, 8 }, { 0, 2, 7 }, { 0, 5, 7 },
        { 0, 4, 7, 10 }, { 0, 4, 7, 11 }, { 0, 3, 7, 10 }, { 0, 3, 6, 10 }, { 0, 3, 6, 9 },
        { 0, 4, 7, 14 }, { 0, 3, 7, 14 }, { 0, 4, 7, 9 }, { 0, 3, 7, 9 },
        { 0, 4, 7, 10, 14 }, { 0, 4, 7, 11, 14 }, { 0, 3, 7, 10, 14 }, { 0, 3, 7, 10, 14, 17 },
        { 0, 4, 10, 14, 21 }, { 0, 5, 7, 10 }, { 0, 3, 7, 11 }, { 0, 7, 10, 14, 17 } };
    return t[(int) q];
}

std::string qualitySuffix (Q q)
{
    static const char* s[] = { "", "m", "dim", "aug", "sus2", "sus4", "7", "maj7", "m7", "m7b5", "dim7",
                               "add9", "madd9", "6", "m6", "9", "maj9", "m9", "m11", "13", "7sus4", "m(maj7)", "11" };
    return s[(int) q];
}

ParsedChord parseRoman (const std::string& token)
{
    ParsedChord pc;
    size_t i = 0;
    if (i < token.size() && token[i] == 'b') { pc.accidental = -1; ++i; }
    else if (i < token.size() && token[i] == '#') { pc.accidental = 1; ++i; }

    const size_t numStart = i;
    while (i < token.size() && (token[i] == 'I' || token[i] == 'V' || token[i] == 'i' || token[i] == 'v'))
        ++i;
    if (i == numStart) return pc;

    std::string num = token.substr (numStart, i - numStart);
    const bool upper = std::isupper ((unsigned char) num[0]) != 0;
    for (auto& c : num) c = (char) std::toupper ((unsigned char) c);

    static const std::map<std::string, int> deg { { "I", 0 }, { "II", 2 }, { "III", 4 }, { "IV", 5 },
                                                  { "V", 7 }, { "VI", 9 }, { "VII", 11 } };
    auto it = deg.find (num);
    if (it == deg.end()) return pc;

    pc.rootRel = ((it->second + pc.accidental) % 12 + 12) % 12;
    const std::string suf = token.substr (i);

    pc.explicitQ = ! suf.empty();
    if      (suf.empty())        pc.q = upper ? Q::Maj : Q::Min;
    else if (suf == "dim7")      pc.q = Q::Dim7;
    else if (suf == "dim")       pc.q = Q::Dim;
    else if (suf == "m7b5")      pc.q = Q::HalfDim;
    else if (suf == "+")         pc.q = Q::Aug;
    else if (suf == "7sus4")     pc.q = Q::Sus7;
    else if (suf == "sus2")      pc.q = Q::Sus2;
    else if (suf == "sus4")      pc.q = Q::Sus4;
    else if (suf == "maj9")      pc.q = Q::Maj9;
    else if (suf == "maj7")      pc.q = upper ? Q::Maj7 : Q::MinMaj7;
    else if (suf == "add9")      pc.q = upper ? Q::Add9 : Q::MinAdd9;
    else if (suf == "13")        pc.q = Q::Dom13;
    else if (suf == "11")        pc.q = upper ? Q::Dom11 : Q::Min11;
    else if (suf == "9")         pc.q = upper ? Q::Dom9 : Q::Min9;
    else if (suf == "7")         pc.q = upper ? Q::Dom7 : Q::Min7;
    else if (suf == "6")         pc.q = upper ? Q::Six : Q::Min6;
    else return pc;

    pc.ok = true;
    return pc;
}

std::string noteName (int pc, bool flats)
{
    static const char* sharp[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    static const char* flat[]  = { "C", "Db", "D", "Eb", "E", "F", "Gb", "G", "Ab", "A", "Bb", "B" };
    pc = (pc % 12 + 12) % 12;
    return flats ? flat[pc] : sharp[pc];
}

bool keyPrefersFlats (int keyPc, bool minor)
{
    static const std::set<int> majFlat { 5, 10, 3, 8, 1, 6 };
    static const std::set<int> minFlat { 2, 7, 0, 5, 10, 3, 8 };
    return minor ? minFlat.count (keyPc) > 0 : majFlat.count (keyPc) > 0;
}

namespace
{
const int kScale[7][7] = {
    { 0, 2, 4, 5, 7, 9, 11 },   // Major
    { 0, 2, 3, 5, 7, 8, 10 },   // Minor
    { 0, 2, 3, 5, 7, 9, 10 },   // Dorian
    { 0, 1, 3, 5, 7, 8, 10 },   // Phrygian
    { 0, 2, 4, 6, 7, 9, 11 },   // Lydian
    { 0, 2, 4, 5, 7, 9, 10 },   // Mixolydian
    { 0, 2, 3, 5, 7, 8, 11 } }; // Harmonic minor

bool scaleIsMinor (int id) { return id == 2 || id == 3 || id == 4 || id == 7; }

const Genre& genreAt (int i)
{
    const auto& g = genres();
    return g[(size_t) std::clamp (i, 0, (int) g.size() - 1)];
}

int wrap12 (int x) { return ((x % 12) + 12) % 12; }

std::vector<std::string> split (const std::string& s)
{
    std::vector<std::string> out;
    std::string cur;
    for (char c : s)
    {
        if (c == ' ') { if (! cur.empty()) out.push_back (cur); cur.clear(); }
        else cur += c;
    }
    if (! cur.empty()) out.push_back (cur);
    return out;
}

// Add 7ths / 9ths etc. to a plain triad, depending on genre colour + complexity.
Q extend (Q base, int rootRel, int ext, float complexity, Rng& r)
{
    int level = ext + (complexity > 0.66f ? 1 : 0) - (complexity < 0.2f ? 1 : 0);
    level = std::clamp (level, 0, 4);
    const bool dominant = rootRel == 7;

    if (base == Q::Dim) return level >= 2 ? Q::HalfDim : Q::Dim;

    if (base == Q::Maj)
    {
        switch (level)
        {
            case 0:  return (complexity > 0.5f && r.chance (0.25)) ? (dominant ? Q::Sus4 : Q::Add9) : Q::Maj;
            case 1:  return r.chance (0.5) ? (dominant ? Q::Dom7 : (r.chance (0.5) ? Q::Add9 : Q::Maj7)) : Q::Maj;
            case 2:  return dominant ? Q::Dom7 : Q::Maj7;
            case 3:  return dominant ? Q::Dom9 : Q::Maj9;
            default: return dominant ? Q::Dom13 : Q::Maj9;
        }
    }
    if (base == Q::Min)
    {
        switch (level)
        {
            case 0:  return (complexity > 0.5f && r.chance (0.2)) ? Q::MinAdd9 : Q::Min;
            case 1:  return r.chance (0.5) ? Q::Min7 : Q::Min;
            case 2:  return Q::Min7;
            case 3:  return Q::Min9;
            default: return r.chance (0.5) ? Q::Min11 : Q::Min9;
        }
    }
    return base;
}

std::string chordName (int rootPc, Q q, int keyPc, bool minor, int accidental)
{
    bool flats = keyPrefersFlats (keyPc, minor);
    if (accidental < 0) flats = true;
    if (accidental > 0) flats = false;
    return noteName (rootPc, flats) + qualitySuffix (q);
}

double movement (const std::vector<int>& a, const std::vector<int>& b)
{
    if (a.empty() || b.empty()) return 0.0;
    double cost = 0.0;
    for (int x : a) { int best = 128; for (int y : b) best = std::min (best, std::abs (x - y)); cost += best; }
    for (int y : b) { int best = 128; for (int x : a) best = std::min (best, std::abs (x - y)); cost += best; }
    return cost;
}

double mean (const std::vector<int>& v)
{
    double s = 0; for (int x : v) s += x;
    return v.empty() ? 0.0 : s / (double) v.size();
}
} // namespace

std::vector<int> voiceChord (int rootPc, const std::vector<int>& ivs, int genre,
                             const std::vector<int>& prev, int centre)
{
    const auto style = genreAt (genre).voicing;

    // Unique pitch classes, root first. Big chords drop the 5th to stay clear.
    std::vector<int> pcs;
    for (int iv : ivs)
    {
        const int pc = wrap12 (rootPc + iv);
        if (std::find (pcs.begin(), pcs.end(), pc) == pcs.end()) pcs.push_back (pc);
    }
    if (pcs.size() >= 5)
    {
        const int fifth = wrap12 (rootPc + 7);
        auto it = std::find (pcs.begin(), pcs.end(), fifth);
        if (it != pcs.end()) pcs.erase (it);
    }

    std::vector<std::vector<int>> candidates;
    auto stack = [] (const std::vector<int>& order, int lowest)
    {
        std::vector<int> v { lowest };
        for (size_t i = 1; i < order.size(); ++i)
        {
            int n = v.back() + 1;
            while (wrap12 (n) != order[i]) ++n;
            v.push_back (n);
        }
        return v;
    };

    const int n = (int) pcs.size();
    for (int rot = 0; rot < n; ++rot)
    {
        std::vector<int> order;
        for (int i = 0; i < n; ++i) order.push_back (pcs[(size_t) ((rot + i) % n)]);
        for (int oct = 3; oct <= 6; ++oct)
        {
            const int low = order[0] + 12 * oct;
            auto close = stack (order, low);

            if (style == Voicing::Drop2 && close.size() >= 4)
            {
                auto d = close;
                d[d.size() - 2] -= 12;
                std::sort (d.begin(), d.end());
                candidates.push_back (d);
            }
            else if (style == Voicing::Spread)
            {
                // Low root, upper structure on top (big, open sound)
                std::vector<int> upper;
                for (int x : close) if (wrap12 (x) != rootPc) upper.push_back (x);
                if (upper.size() < 2) upper = close;
                int bass = rootPc + 12 * 3;
                while (bass > upper.front() - 7) bass -= 12;
                while (bass < centre - 24) bass += 12;
                std::vector<int> v { bass };
                v.insert (v.end(), upper.begin(), upper.end());
                if (v.size() <= 4) v.push_back (upper.front() + 12);
                std::sort (v.begin(), v.end());
                candidates.push_back (v);
            }
            else
            {
                candidates.push_back (close);
            }
        }
    }

    std::vector<int> best;
    double bestCost = 1e9;
    for (auto& c : candidates)
    {
        if (c.front() < centre - 26 || c.back() > centre + 22) continue;
        double cost = 0.35 * std::abs (mean (c) - (double) centre) * (prev.empty() ? 3.0 : 1.0);
        cost += movement (prev, c);
        cost += 0.15 * (double) std::max (0, (c.back() - c.front()) - (style == Voicing::Spread ? 26 : 14));
        if (cost < bestCost) { bestCost = cost; best = c; }
    }
    if (best.empty() && ! candidates.empty()) best = candidates.front();
    return best;
}

// =============================================================================
// Composer
// =============================================================================
namespace
{
struct SlotChord
{
    int rootRel;
    Q q;
    std::string roman;
    int accidental;
};

struct Ctx
{
    const Genre& g;
    Settings s;
    bool minor = false;
    int scaleId = 1;
    int keyPc = 0;
    int tonicMidi = 60;
    float energy = 1.0f;
    double swing = 0.0;
    double length = 16.0;
    std::vector<std::vector<int>> chordPcs;   // pitch classes per chord span
    Pattern* p = nullptr;
    int importShift = 0;                      // semitones from the imported file's key to ours

    explicit Ctx (const Genre& genre) : g (genre) {}

    int chordIndexAt (double t) const
    {
        const auto& c = p->chords;
        for (int i = (int) c.size() - 1; i >= 0; --i)
            if (t >= c[(size_t) i].start - 1.0e-6) return i;
        return 0;
    }
    bool inChord (int pitch, int chordIdx) const
    {
        const auto& v = chordPcs[(size_t) chordIdx];
        return std::find (v.begin(), v.end(), wrap12 (pitch)) != v.end();
    }
    bool inScale (int pitch) const
    {
        const int rel = wrap12 (pitch - keyPc);
        for (int x : kScale[scaleId - 1]) if (x == rel) return true;
        return false;
    }
    int pitchOfStep (int step) const
    {
        const int oct = (int) std::floor ((double) step / 7.0);
        const int deg = step - oct * 7;
        return tonicMidi + 12 * oct + kScale[scaleId - 1][deg];
    }
    // Nearest scale step for a pitch (rounding down).
    int stepOfPitch (int pitch) const
    {
        int best = 0, bestD = 1000;
        for (int st = -21; st <= 28; ++st)
        {
            const int d = std::abs (pitchOfStep (st) - pitch);
            if (d < bestD) { bestD = d; best = st; }
        }
        return best;
    }
    double swingTime (double t) const
    {
        const double pos = t * 4.0;
        const double r = std::round (pos);
        if (std::abs (pos - r) < 1.0e-6 && ((long long) r) % 2 != 0)
            return t + swing * (0.25 / 3.0);
        return t;
    }
};

int sectionGroup (int section)
{
    switch (section)
    {
        case Verse:     return 1;
        case PreChorus: return 2;
        case Bridge:    return 3;
        default:        return 0;   // Intro, Hook and Outro share the hook's material
    }
}

int sectionMask (int section)
{
    switch (sectionGroup (section)) { case 1: return V; case 2: return P; case 3: return B; default: return H; }
}

float sectionEnergy (int section)
{
    static const float e[] = { 0.4f, 0.65f, 0.75f, 1.0f, 0.55f, 0.4f };
    return e[std::clamp (section, 0, 5)];
}

uint64_t seedFor (uint32_t seed, uint64_t a, uint64_t b = 0, uint64_t c = 0)
{
    return mix64 (mix64 (mix64 ((uint64_t) seed + 0x1234567ull) ^ (a * 0x9E3779B97F4A7C15ull)) ^ (b * 0xC2B2AE3D27D4EB4Full) ^ c);
}

void resolveMode (const Genre& g, const Settings& s, bool& minor, int& scaleId)
{
    if (s.scale > 0)
    {
        scaleId = std::clamp (s.scale, 1, 7);
        minor = scaleIsMinor (scaleId);
    }
    else
    {
        Rng r (seedFor (s.harmonySeed, 11, (uint64_t) s.genre));
        minor = r.chance (g.minorBias);
        scaleId = minor ? 2 : 1;
    }
}

const Prog& chooseProgression (const Genre& g, const Settings& s, bool minor)
{
    if (s.progression > 0 && s.progression <= (int) g.progs.size())
        return g.progs[(size_t) s.progression - 1];

    const int mask = sectionMask (s.section);
    std::vector<const Prog*> pool;
    for (auto& p : g.progs) if (p.minor == minor && (p.sections & mask)) pool.push_back (&p);
    if (pool.empty()) for (auto& p : g.progs) if (p.minor == minor) pool.push_back (&p);
    if (pool.empty()) for (auto& p : g.progs) pool.push_back (&p);

    Rng r (seedFor (s.harmonySeed, 23, (uint64_t) sectionGroup (s.section), (uint64_t) s.genre));
    return *pool[(size_t) r.range (0, (int) pool.size() - 1)];
}

bool isMinorQ (Q q)
{
    return q == Q::Min || q == Q::Min7 || q == Q::Min9 || q == Q::Min11 || q == Q::MinAdd9
        || q == Q::Min6 || q == Q::MinMaj7 || q == Q::Dim || q == Q::HalfDim || q == Q::Dim7;
}

// Functional substitutes, used to vary a progression the second time round.
bool substitute (int rootRel, Q q, int& outRoot, Q& outQ)
{
    const bool m = isMinorQ (q);
    struct S { int from; bool minorQ; int to; Q tq; };
    static const S table[] = {
        { 0, false, 9, Q::Min }, { 5, false, 2, Q::Min }, { 2, true, 5, Q::Maj }, { 9, true, 5, Q::Maj },
        { 4, true, 7, Q::Maj },  { 0, true, 8, Q::Maj },  { 8, false, 5, Q::Min }, { 10, false, 7, Q::Min },
        { 5, true, 8, Q::Maj },  { 3, false, 0, Q::Min } };
    for (auto& e : table)
        if (e.from == rootRel && e.minorQ == m)
        {
            outRoot = e.to; outQ = e.tq;
            return true;
        }
    return false;
}

std::string romanFor (int rootRel, Q q)
{
    static const char* up[] = { "I", "bII", "II", "bIII", "III", "IV", "#IV", "V", "bVI", "VI", "bVII", "VII" };
    std::string r = up[wrap12 (rootRel)];
    if (isMinorQ (q))
        for (size_t i = 0; i < r.size(); ++i)
            if (r[i] == 'I' || r[i] == 'V') r[i] = (char) std::tolower ((unsigned char) r[i]);
    if (q == Q::Dim) r += "dim";
    return r;
}


// ---------------------------------------------------------------- imported chords
void clampToLength (Ctx& c, std::vector<NoteEv>& notes);

int importShiftFor (int fileTonic, int key)
{
    int sh = wrap12 (key - fileTonic);
    if (sh > 6) sh -= 12;      // move the least distance, up or down
    return sh;
}

void buildImportedHarmony (Ctx& c)
{
    const auto& imp = *c.s.imported;
    c.minor = imp.minor;
    if (c.s.scale == 0 || scaleIsMinor (c.s.scale) != imp.minor) c.scaleId = imp.minor ? 2 : 1;
    c.p->minor = c.minor;
    c.p->scale = c.scaleId;
    c.p->progressionName = imp.name;
    c.importShift = importShiftFor (imp.tonicPc, c.keyPc);

    const double L = std::max (1.0, imp.lengthBeats);
    for (double off = 0.0; off < c.length - 1.0e-6; off += L)
        for (auto& sp : imp.spans)
        {
            const double start = off + sp.start;
            if (start >= c.length - 1.0e-6) break;
            ChordSpan span;
            span.start = start;
            span.len = std::min (sp.len, c.length - start);
            span.rootPc = wrap12 (sp.rootPc + c.importShift);
            span.name = chordName (span.rootPc, sp.q, c.keyPc, c.minor, 0);
            span.roman = romanFor (wrap12 (span.rootPc - c.keyPc), sp.q);
            for (int pch : sp.pitches) span.notes.push_back (pch + c.importShift);
            std::vector<int> pcs;
            for (int iv : intervals (sp.q)) pcs.push_back (wrap12 (span.rootPc + iv));
            c.chordPcs.push_back (pcs);
            c.p->chords.push_back (span);
        }
}

void buildImportedChordPart (Ctx& c)
{
    const auto& imp = *c.s.imported;
    auto& out = c.p->lanes[ChordsLane];
    const double L = std::max (1.0, imp.lengthBeats);
    for (double off = 0.0; off < c.length - 1.0e-6; off += L)
        for (auto n : imp.notes)
        {
            n.start += off;
            if (n.start >= c.length - 1.0e-6) continue;
            n.pitch += c.importShift;
            out.push_back (n);
        }
    clampToLength (c, out);
}

// ---------------------------------------------------------------- harmony
void buildHarmony (Ctx& c)
{
    if (c.s.imported != nullptr && ! c.s.imported->spans.empty()) { buildImportedHarmony (c); return; }
    const auto& g = c.g;
    const auto& s = c.s;

    const Prog& prog = chooseProgression (g, s, c.minor);
    if (prog.minor != c.minor)
    {
        // The progression decides major or minor. Keep the user's scale when it fits.
        c.minor = prog.minor;
        if (s.scale == 0 || scaleIsMinor (s.scale) != prog.minor)
            c.scaleId = prog.minor ? 2 : 1;
    }
    c.p->minor = c.minor;
    c.p->scale = c.scaleId;
    c.p->progressionName = prog.name;

    const auto tokens = split (prog.chords);
    const int n = std::max (1, (int) tokens.size());

    Rng rr (seedFor (s.harmonySeed, 37, (uint64_t) sectionGroup (s.section), (uint64_t) s.genre));

    // How many chords per bar
    double total = 0; for (auto& e : g.chordsPerBar) total += e.second;
    double pick = rr.uni() * total, cpb = g.chordsPerBar.front().first;
    for (auto& e : g.chordsPerBar) { if (pick < e.second) { cpb = e.first; break; } pick -= e.second; }
    cpb = std::max (cpb, (double) n / (double) s.bars);
    int slots = std::max (n, (int) std::lround (s.bars * cpb));
    while (slots % n != 0) ++slots;
    const double chordLen = c.length / (double) slots;

    // Parse + colour each token once, so repeats sound the same
    std::vector<SlotChord> base;
    for (int i = 0; i < n; ++i)
    {
        auto pc = parseRoman (tokens[(size_t) i]);
        if (! pc.ok) pc = parseRoman (c.minor ? "i" : "I");
        Rng er (seedFor (s.harmonySeed, 41, (uint64_t) i * 131u + (uint64_t) pc.rootRel, (uint64_t) s.genre));
        const Q q = pc.explicitQ ? pc.q : extend (pc.q, pc.rootRel, g.extensions, s.complexity, er);
        base.push_back ({ pc.rootRel, q, tokens[(size_t) i], pc.accidental });
    }

    std::vector<SlotChord> seq;
    const int reps = slots / n;
    for (int r = 0; r < reps; ++r)
        for (int i = 0; i < n; ++i)
        {
            auto sc = base[(size_t) i];
            const bool lastRep = reps > 1 && r == reps - 1;
            const bool varySection = s.section == Verse || s.section == Bridge || s.section == PreChorus;
            if (lastRep && i > 0 && varySection && s.complexity > 0.45f && rr.chance (s.complexity * 0.45))
            {
                int nr; Q nq;
                if (substitute (sc.rootRel, sc.q, nr, nq))
                {
                    Rng er (seedFor (s.harmonySeed, 43, (uint64_t) nr));
                    sc = { nr, extend (nq, nr, g.extensions, s.complexity, er), romanFor (nr, nq), nr == 8 || nr == 10 || nr == 3 ? -1 : 0 };
                }
            }
            seq.push_back (sc);
        }

    // Section-specific endings
    auto& last = seq.back();
    if ((s.section == PreChorus && rr.chance (0.55)) || (s.section == Bridge && rr.chance (0.35)))
    {
        if (last.rootRel != 7 && last.rootRel != 10 && last.rootRel != 8)
        {
            Rng er (seedFor (s.harmonySeed, 47));
            last = { 7, extend (Q::Maj, 7, g.extensions, s.complexity, er), "V", 0 };
        }
    }
    if (s.section == Outro && last.rootRel != 0)
    {
        Rng er (seedFor (s.harmonySeed, 53));
        const Q tq = c.minor ? Q::Min : Q::Maj;
        last = { 0, extend (tq, 0, g.extensions, s.complexity, er), c.minor ? "i" : "I", 0 };
    }

    // Voice everything with smooth voice leading
    std::vector<int> prev;
    for (int i = 0; i < (int) seq.size(); ++i)
    {
        const auto& sc = seq[(size_t) i];
        ChordSpan span;
        span.start = i * chordLen;
        span.len = chordLen;
        span.rootPc = wrap12 (c.keyPc + sc.rootRel);
        span.roman = sc.roman;
        span.name = chordName (span.rootPc, sc.q, c.keyPc, c.minor, sc.accidental);
        span.notes = voiceChord (span.rootPc, intervals (sc.q), s.genre, prev, 62);
        prev = span.notes;

        std::vector<int> pcs;
        for (int iv : intervals (sc.q)) pcs.push_back (wrap12 (span.rootPc + iv));
        c.chordPcs.push_back (pcs);
        c.p->chords.push_back (span);
    }
}

// ---------------------------------------------------------------- humanise
void humanise (Ctx& c, std::vector<NoteEv>& notes, uint64_t salt, bool timing, uint32_t laneSeed)
{
    Rng r (seedFor (laneSeed, salt));
    const double h = c.s.humanize;
    for (auto& n : notes)
    {
        n.vel = std::clamp (n.vel + (int) std::lround ((r.uni() * 2.0 - 1.0) * h * 14.0), 1, 127);
        if (timing && n.start > 0.001)
            n.start = std::max (0.0, n.start + (r.uni() * 2.0 - 1.0) * h * 0.012);
    }
}

void clampToLength (Ctx& c, std::vector<NoteEv>& notes)
{
    std::vector<NoteEv> out;
    for (auto n : notes)
    {
        if (n.start >= c.length - 1.0e-6) continue;
        n.len = std::min (n.len, c.length - n.start);
        if (n.len < 0.03) continue;
        n.pitch = std::clamp (n.pitch, 0, 127);
        out.push_back (n);
    }
    std::sort (out.begin(), out.end(), [] (const NoteEv& a, const NoteEv& b)
               { return a.start < b.start - 1.0e-9 || (std::abs (a.start - b.start) <= 1.0e-9 && a.pitch < b.pitch); });
    notes.swap (out);
}

int pickLevel (Ctx& c, int count, uint64_t salt, uint32_t laneSeed, float scale = 1.0f)
{
    if (count <= 1) return 0;
    Rng r (seedFor (laneSeed, salt, (uint64_t) c.s.section));
    float lvl = std::clamp (c.s.density * (0.55f + 0.65f * c.energy) * scale, 0.0f, 1.0f);
    int idx = (int) std::lround (lvl * (float) (count - 1));
    if (r.chance (0.3 * c.s.complexity)) idx += r.chance (0.5) ? 1 : -1;
    return std::clamp (idx, 0, count - 1);
}

// ---------------------------------------------------------------- chords lane
void buildChordRhythm (Ctx& c)
{
    if (c.s.imported != nullptr && ! c.s.imported->notes.empty()) { buildImportedChordPart (c); return; }
    const auto& rh = c.g.chordRhythms;
    int idx = pickLevel (c, (int) rh.size(), 61, c.s.harmonySeed);
    if (c.s.section == Intro || c.s.section == Outro) idx = std::min (idx, 1);
    const std::string pat = rh[(size_t) idx];

    auto& out = c.p->lanes[ChordsLane];
    const auto& chords = c.p->chords;

    int activeChord = -1;
    double activeStart = 0.0;
    int activeVel = 90;

    auto close = [&] (double t)
    {
        if (activeChord < 0) return;
        const double len = std::max (0.08, t - activeStart - 0.03);
        for (int note : chords[(size_t) activeChord].notes)
            out.push_back ({ c.swingTime (activeStart), len, note, activeVel });
        activeChord = -1;
    };
    auto open = [&] (double t, int chord, int vel)
    {
        activeChord = chord; activeStart = t; activeVel = vel;
    };

    const int steps = c.s.bars * 16;
    for (int st = 0; st < steps; ++st)
    {
        const double t = st * 0.25;
        const char ch = pat[(size_t) (st % 16)];
        const int here = c.chordIndexAt (t);
        const bool chordStarts = std::abs (chords[(size_t) here].start - t) < 1.0e-6;
        const int vel = (st % 4 == 0) ? 98 : 86;

        if (ch == 'x')
        {
            close (t);
            open (t, here, vel);
        }
        else if (ch == 'p')
        {
            close (t);
            const int next = c.chordIndexAt (std::min (c.length - 0.01, t + 1.0));
            open (t, next, 92);
        }
        else if (ch == '-')
        {
            if (chordStarts && activeChord != here)
            {
                close (t);
                open (t, here, vel);
            }
        }
        else // rest
        {
            close (t);
        }
    }
    close (c.length);
    humanise (c, out, 67, false, c.s.harmonySeed);
    clampToLength (c, out);
}

// ---------------------------------------------------------------- melody
struct MotifNote { double t, len; int off; bool strong; };

std::vector<MotifNote> makeMotif (Ctx& c, Rng& r, int bars)
{
    const auto& m = c.g.melody;
    const int S = 16 * bars;
    const double target = m.notesPerBar * bars * (0.45 + c.s.density * 1.1) * (0.65 + 0.45 * c.energy);
    const int N = std::clamp ((int) std::lround (target), 2, (int) (S * 0.7));

    std::vector<double> w ((size_t) S, 0.0);
    for (int s = 0; s < S; ++s)
    {
        double x;
        if (s % 16 == 0)      x = 1.3;
        else if (s % 4 == 0)  x = 1.0;
        else if (s % 2 == 0)  x = 0.45 + 0.5 * m.syncopation;
        else                  x = m.sixteenths ? 0.08 + 0.45 * m.syncopation * (0.5 + c.s.complexity) : 0.0;
        if (s >= S - 4) x *= 0.3;   // breathe at the end of the phrase
        w[(size_t) s] = x;
    }
    w[0] *= r.chance (0.75) ? 2.5 : 0.15;   // sometimes start with a pickup instead of on the one

    std::vector<double> onsets;
    for (int i = 0; i < N; ++i)
    {
        double tot = 0; for (double x : w) tot += x;
        if (tot <= 0) break;
        double pick = r.uni() * tot;
        for (int s = 0; s < S; ++s)
        {
            if (pick < w[(size_t) s]) { onsets.push_back (s * 0.25); w[(size_t) s] = 0; break; }
            pick -= w[(size_t) s];
        }
    }
    std::sort (onsets.begin(), onsets.end());

    // Triplet figures replace a beat here and there
    if (m.triplets > 0.0f)
    {
        std::vector<double> out;
        std::set<int> tripBeats;
        for (int b = 0; b < bars * 4 - 1; ++b)
            if (r.chance (m.triplets * (0.35 + c.s.complexity * 0.8)))
            {
                bool has = false;
                for (double t : onsets) if (t >= b && t < b + 1) has = true;
                if (has) tripBeats.insert (b);
            }
        for (double t : onsets) if (! tripBeats.count ((int) std::floor (t))) out.push_back (t);
        for (int b : tripBeats) for (int k = 0; k < 3; ++k) out.push_back (b + k / 3.0);
        std::sort (out.begin(), out.end());
        onsets.swap (out);
    }
    if (onsets.empty()) onsets.push_back (0.0);

    std::vector<MotifNote> mot;
    int off = 0, dir = r.chance (0.5) ? 1 : -1;
    bool afterLeap = false;
    for (size_t i = 0; i < onsets.size(); ++i)
    {
        const double t = onsets[i];
        const double next = i + 1 < onsets.size() ? onsets[i + 1] : bars * 4.0;
        const double gap = next - t;
        double len = r.chance (m.legato) ? gap * 0.95 : gap * (0.45 + 0.4 * r.uni());
        if (gap > 2.0 && ! r.chance (m.legato)) len = std::min (len, 1.0 + r.uni());
        if (i + 1 == onsets.size()) len = std::min (gap * 0.95, 2.5);
        len = std::max (len, 0.1);

        if (i > 0)
        {
            int delta;
            if (afterLeap)              { delta = -dir; afterLeap = false; }
            else if (r.chance (m.repeat)) delta = 0;
            else if (r.chance (m.leap))   { delta = dir * r.range (2, 4); afterLeap = true; }
            else                          delta = dir * (r.chance (0.75) ? 1 : 2);

            if (r.chance (0.3)) dir = -dir;
            off += delta;
            if (off > 8)  { off = 8 - 1;  dir = -1; }
            if (off < -6) { off = -6 + 1; dir = 1; }
            if (delta != 0 && r.chance (0.35)) dir = delta > 0 ? 1 : -1;
        }
        const bool strong = std::abs (t - std::round (t)) < 1.0e-6;
        mot.push_back ({ t, len, off, strong });
    }
    return mot;
}

std::vector<MotifNote> varyMotif (const std::vector<MotifNote>& a, Rng& r)
{
    auto v = a;
    const size_t from = v.size() - std::max<size_t> (1, (v.size() + 2) / 3);
    int off = from > 0 ? v[from - 1].off : 0;
    for (size_t i = from; i < v.size(); ++i)
    {
        off += r.range (-2, 2);
        v[i].off = std::clamp (off, -6, 8);
    }
    return v;
}

void buildMelody (Ctx& c)
{
    auto& out = c.p->lanes[MelodyLane];
    const int section = c.s.section;
    const int group = sectionGroup (section);

    const bool hookLike = group == 0 || section == PreChorus;
    const int letterBars = hookLike ? 1 : 2;
    const int L = std::max (1, c.s.bars / letterBars);

    // Register, relative to the tonic
    int targetOff = 7, lo = 0, hi = 17;
    if (section == Verse)       { targetOff = 2; lo = -5; hi = 12; }
    if (section == PreChorus)   { targetOff = 4; lo = -2; hi = 14; }
    if (section == Bridge)      { targetOff = 4; lo = -3; hi = 14; }
    const int lowP = c.tonicMidi + lo, highP = c.tonicMidi + hi;

    std::map<char, std::vector<MotifNote>> motifs;
    std::function<const std::vector<MotifNote>& (char)> motif;
    motif = [&] (char letter) -> const std::vector<MotifNote>&
    {
        auto it = motifs.find (letter);
        if (it != motifs.end()) return it->second;
        Rng r (seedFor (c.s.melodySeed, 71, (uint64_t) group * 17u + (uint64_t) (unsigned char) letter, (uint64_t) c.s.genre));
        if (letter == 'a')
        {
            auto base = motif ('A');
            return motifs[letter] = varyMotif (base, r);
        }
        return motifs[letter] = makeMotif (c, r, letterBars);
    };

    std::vector<char> form;
    for (int i = 0; i < L; ++i)
    {
        if (L == 1) { form.push_back ('A'); continue; }
        if (L == 2) { form.push_back (i == 0 ? 'A' : 'B'); continue; }
        const int pos = i % 4;
        const bool lastGroup = (i / 4) == (L - 1) / 4;
        if (section == Bridge)
            form.push_back (pos == 0 ? 'D' : pos == 1 ? 'd' : pos == 2 ? 'E' : 'F');
        else
            form.push_back (pos == 0 ? 'A' : pos == 1 ? 'a' : pos == 2 ? 'A' : (lastGroup ? 'C' : 'B'));
    }
    // 'd' is a variation of D
    if (section == Bridge)
        motifs['D'] = motif ('D');

    int prevPitch = c.tonicMidi + targetOff;
    for (int li = 0; li < L; ++li)
    {
        const double t0 = li * letterBars * 4.0;
        char letter = form[(size_t) li];

        if ((section == Intro || section == Outro) && (li % 2 == 1)) continue;   // sparse teaser

        std::vector<MotifNote> mot;
        if (letter == 'd')
        {
            Rng r (seedFor (c.s.melodySeed, 73, (uint64_t) group));
            mot = varyMotif (motif ('D'), r);
        }
        else mot = motif (letter);

        const int transpose = section == PreChorus ? std::min (li, 3) : 0;
        if (mot.empty()) continue;

        // Anchor: a chord tone near the target register that keeps the motif in range
        int minOff = 100, maxOff = -100;
        for (auto& n : mot) { minOff = std::min (minOff, n.off); maxOff = std::max (maxOff, n.off); }
        const int firstChord = c.chordIndexAt (t0 + mot[0].t);
        const int targetPitch = li == 0 ? c.tonicMidi + targetOff : (prevPitch + c.tonicMidi + targetOff) / 2;

        int anchor = 0; double bestCost = 1e9;
        for (int st = -14; st <= 21; ++st)
        {
            const int p = c.pitchOfStep (st + transpose);
            double cost = std::abs (p - targetPitch);
            if (! c.inChord (p, firstChord)) cost += 6;
            if (c.pitchOfStep (st + transpose + minOff) < lowP - 2) cost += 8;
            if (c.pitchOfStep (st + transpose + maxOff) > highP + 2) cost += 8;
            if (cost < bestCost) { bestCost = cost; anchor = st; }
        }

        for (auto& mn : mot)
        {
            const double t = t0 + mn.t;
            if (t >= c.length) break;
            const int ci = c.chordIndexAt (t);
            int step = anchor + mn.off + transpose;
            int p = c.pitchOfStep (step);

            if ((mn.strong || mn.len >= 0.74) && ! c.inChord (p, ci))
            {
                const int up = c.pitchOfStep (step + 1), dn = c.pitchOfStep (step - 1);
                if (c.inChord (dn, ci) && (! c.inChord (up, ci) || std::abs (dn - prevPitch) <= std::abs (up - prevPitch))) p = dn;
                else if (c.inChord (up, ci)) p = up;
                else if (c.inChord (c.pitchOfStep (step - 2), ci)) p = c.pitchOfStep (step - 2);
                else if (c.inChord (c.pitchOfStep (step + 2), ci)) p = c.pitchOfStep (step + 2);
            }
            // Borrowed chords: bend the scale note onto the chord's own note
            if (! c.inChord (p, ci))
                for (int d : { -1, 1 })
                    if (c.inChord (p + d, ci) && ! c.inScale (p + d)) { p += d; break; }

            while (p > highP) p -= 12;
            while (p < lowP)  p += 12;

            const int vel = mn.strong ? 100 : 88;
            const double start = std::abs (mn.t - std::round (mn.t * 4.0) / 4.0) < 1.0e-6 ? c.swingTime (t) : t;
            out.push_back ({ start, mn.len, p, vel });
            prevPitch = p;
        }
    }

    // Phrase ending
    if (! out.empty())
    {
        auto& lastN = out.back();
        const int ci = c.chordIndexAt (lastN.start);
        int want = c.p->chords[(size_t) ci].rootPc;
        if (section == PreChorus) want = wrap12 (c.keyPc + 7);
        else if (section == Verse && c.chordPcs[(size_t) ci].size() > 1) want = c.chordPcs[(size_t) ci][1];
        int best = lastN.pitch, bestD = 100;
        for (int p = lowP; p <= highP; ++p)
            if (wrap12 (p) == want && std::abs (p - lastN.pitch) < bestD) { bestD = std::abs (p - lastN.pitch); best = p; }
        lastN.pitch = best;
        lastN.len = std::max (lastN.len, std::min (2.0, c.length - lastN.start));
    }

    humanise (c, out, 79, true, c.s.melodySeed);
    clampToLength (c, out);
}

// ---------------------------------------------------------------- drums
int velOf (char ch)
{
    switch (ch) { case 'X': return 118; case 'x': return 96; case 'g': return 50; default: return 0; }
}

// A bar of drums as editable strings (so each GENERATE can vary the genre's beat)
struct BarLines { std::string kick, snare, clap, hat, openHat, perc, rim, tom; };

BarLines toLines (const DrumBar& d)
{
    return { d.kick, d.snare, d.clap, d.hat, d.openHat, d.perc, d.rim, d.tom };
}

bool isHit (char ch) { return ch == 'x' || ch == 'X' || ch == 'g'; }

// Varies the genre's template so the drums' own GENERATE gives a new beat in the same style.
void mutateDrums (BarLines& b, Rng& r, const Genre& g, float complexity, float density, bool variationBar)
{
    if (b.kick.size() == 16)
    {
        const bool fourOnFloor = isHit (b.kick[0]) && isHit (b.kick[4]) && isHit (b.kick[8]) && isHit (b.kick[12]);
        auto backbeat = [&] (int st)
        {
            return (b.snare.size() == 16 && (b.snare[(size_t) st] == 'x' || b.snare[(size_t) st] == 'X'))
                || (b.clap.size() == 16 && (b.clap[(size_t) st] == 'x' || b.clap[(size_t) st] == 'X'));
        };
        auto pickEmpty = [&] (bool sixteenths) -> int
        {
            std::vector<int> cand;
            for (int st = 1; st < 16; ++st)
            {
                if (b.kick[(size_t) st] != '.' || backbeat (st)) continue;
                if (st % 2 == 1 && ! sixteenths) continue;
                cand.push_back (st);
                if (st % 4 == 2 || st % 4 == 3) cand.push_back (st);   // offbeats and pickups sound best
            }
            return cand.empty() ? -1 : cand[(size_t) r.range (0, (int) cand.size() - 1)];
        };

        if (fourOnFloor)
        {
            if (r.chance (0.25 + 0.3 * complexity)) { const int st = r.chance (0.5) ? 15 : 7; if (b.kick[(size_t) st] == '.') b.kick[(size_t) st] = 'g'; }
        }
        else
        {
            if (r.chance (0.55))
                if (int st = pickEmpty (complexity > 0.45f); st > 0) b.kick[(size_t) st] = 'x';
            if (r.chance (0.4))
            {
                std::vector<int> hits;
                for (int st = 1; st < 16; ++st) if (isHit (b.kick[(size_t) st])) hits.push_back (st);
                if (! hits.empty())
                {
                    const int from = hits[(size_t) r.range (0, (int) hits.size() - 1)];
                    const int to = std::clamp (from + (r.chance (0.5) ? 1 : -1) * r.range (1, 2), 1, 15);
                    if (b.kick[(size_t) to] == '.' && ! backbeat (to)) { b.kick[(size_t) to] = b.kick[(size_t) from]; b.kick[(size_t) from] = '.'; }
                }
            }
            if (r.chance (0.3 * (1.0 - density)))
                for (int st = 15; st > 0; --st) if (b.kick[(size_t) st] == 'x') { b.kick[(size_t) st] = '.'; break; }
        }
    }

    // Ghost snares for the genres that swing / play live
    const bool ghosty = g.swing > 0.2f || g.fill == Fill::Toms;
    if (ghosty && b.snare.size() == 16 && r.chance (0.45 + 0.35 * complexity))
    {
        const int n = r.chance (0.5) ? 1 : 2;
        for (int k = 0; k < n; ++k)
        {
            const int st = r.range (1, 15);
            if (b.snare[(size_t) st] == '.' && st != 3 && st != 11) b.snare[(size_t) st] = 'g';
        }
    }

    // Hats: switch between 8ths and 16ths, move the accents
    if (b.hat.size() == 16 && ! g.hatRolls)
    {
        bool sixteenths = true;
        for (int st = 1; st < 16; st += 2) if (b.hat[(size_t) st] == '.') sixteenths = false;
        if (r.chance (0.4))
        {
            for (int st = 1; st < 16; st += 2)
                b.hat[(size_t) st] = sixteenths ? '.' : (density > 0.35f ? 'g' : '.');
        }
        if (r.chance (0.35))
            for (int st = 2; st < 16; st += 4)
                if (isHit (b.hat[(size_t) st])) b.hat[(size_t) st] = r.chance (0.5) ? 'x' : 'g';
    }
    else if (b.hat.size() == 16 && g.hatRolls && r.chance (0.5))
    {
        // trap / drill: 8ths <-> 16ths before the rolls are added
        for (int st = 1; st < 16; st += 2)
            if (b.hat[(size_t) st] == '.' && b.hat[(size_t) (st - 1)] == 'x' && r.chance (0.7)) b.hat[(size_t) st] = 'g';
    }

    if (variationBar && b.hat.size() == 16 && r.chance (0.3))
        for (int st = 13; st < 16; ++st) b.hat[(size_t) st] = '.';

    if (b.openHat.empty() && b.hat.size() == 16 && ! g.hatRolls && r.chance (0.3))
    {
        b.openHat = std::string (16, '.');
        b.openHat[variationBar ? 14 : 6] = 'x';
    }
}

void buildDrums (Ctx& c)
{
    auto& out = c.p->lanes[DrumsLane];
    if (! c.g.drums) return;

    const int section = c.s.section;
    const int bars = c.s.bars;
    Rng r (seedFor (c.s.drumSeed, 83, (uint64_t) section, (uint64_t) c.s.genre));
    const bool drill = std::string (c.g.name) == "Drill";

    auto add = [&] (double t, int note, int vel, double len = 0.2)
    {
        if (vel <= 0) return;
        out.push_back ({ t, len, note, std::clamp (vel, 1, 127) });
    };

    double rollProb = (0.12 + 0.4 * c.s.complexity) * (section == Hook ? 1.25 : section == Verse ? 0.8 : 0.55);

    // This drum seed's version of the genre's beat (same across sections, so the song holds together)
    Rng mr (seedFor (c.s.drumSeed, 211, (uint64_t) c.s.genre));
    BarLines barA = toLines (c.g.drumA), barB = toLines (c.g.drumB);
    mutateDrums (barA, mr, c.g, c.s.complexity, c.s.density, false);
    mutateDrums (barB, mr, c.g, c.s.complexity, c.s.density, true);

    for (int b = 0; b < bars; ++b)
    {
        const bool useB = drill ? (b % 2 == 1) : (b % 4 == 3);
        const BarLines& d = useB ? barB : barA;
        const double bar0 = b * 4.0;
        const bool firstHalf = b < bars / 2;
        const bool lastBar = b == bars - 1;

        auto line = [&] (const std::string& s, int note, double len, bool enabled)
        {
            if (! enabled || s.size() < 16) return;
            for (int st = 0; st < 16; ++st)
                add (c.swingTime (bar0 + st * 0.25), note, velOf (s[(size_t) st]), len);
        };

        bool kick = true, snare = true, hats = true, extras = true, kickOnlyOne = false;
        switch (section)
        {
            case Intro:     kick = false; snare = false; extras = false; break;
            case Verse:     extras = r.chance (0.6); break;
            case PreChorus: if (firstHalf) kickOnlyOne = true; break;
            case Bridge:    if (firstHalf) kickOnlyOne = true; extras = false; break;
            case Outro:     if (! firstHalf) kick = false; extras = false; break;
            default: break;
        }
        if (section == Outro && lastBar)
        {
            add (bar0, Drum::Kick, 118);
            add (bar0, Drum::Crash, 110, 3.0);
            continue;
        }

        if (kickOnlyOne) add (bar0, Drum::Kick, 112);
        else line (d.kick, Drum::Kick, 0.2, kick);

        const bool build = section == PreChorus && lastBar && c.g.fill != Fill::None;
        if (build)
        {
            for (int st = 0; st < 16; ++st)
                if (st < 8 ? st % 2 == 0 : true)
                    add (bar0 + st * 0.25, Drum::Snare, 55 + st * 4);
        }
        else
        {
            line (d.snare, Drum::Snare, 0.2, snare);
            line (d.clap, Drum::Clap, 0.2, snare);
            line (d.rim, Drum::Rim, 0.15, snare || section == Intro);
        }

        // Hats (bridge: thinned out)
        if (hats && d.hat[0] != 0)
        {
            std::string h = d.hat;
            if (section == Bridge) for (int st = 0; st < 16; ++st) if (st % 4 != 0 && st % 4 != 2) h[(size_t) st] = '.';
            if (section == Bridge) for (int st = 2; st < 16; st += 4) h[(size_t) st] = '.';

            std::vector<bool> rolled (4, false);
            if (c.g.hatRolls && section != Bridge)
                for (int beat = 0; beat < 4; ++beat)
                    if ((beat > 0 || b % 2 == 1) && r.chance (rollProb)) rolled[(size_t) beat] = true;

            for (int st = 0; st < 16; ++st)
                if (! rolled[(size_t) (st / 4)])
                    add (c.swingTime (bar0 + st * 0.25), Drum::ClosedHat, velOf (h[(size_t) st]) - (section == Intro ? 15 : 0), 0.12);

            for (int beat = 0; beat < 4; ++beat)
            {
                if (! rolled[(size_t) beat]) continue;
                const int type = r.range (0, 3);
                const double t0 = bar0 + beat;
                int count; double from = 0.0, span = 1.0;
                switch (type)
                {
                    case 0:  count = 6; break;                          // 1/16 triplets
                    case 1:  count = 8; break;                          // 1/32
                    case 2:  count = 3; break;                          // 1/8 triplets
                    default: count = 4; from = 0.5; span = 0.5; break;  // 1/32 burst on the and
                }
                if (type == 3) add (t0, Drum::ClosedHat, 92, 0.12);
                for (int k = 0; k < count; ++k)
                    add (t0 + from + span * k / count, Drum::ClosedHat, 62 + (int) (48.0 * k / std::max (1, count - 1)), 0.08);
            }
        }

        line (d.openHat, Drum::OpenHat, 0.45, hats && (extras || section == Hook));
        line (d.perc, Drum::Shaker, 0.12, extras || section == Hook);
        line (d.tom, Drum::FloorTom, 0.4, kick || section == Intro);

        if (section == Hook && b % 2 == 1 && c.s.density > 0.55f && d.openHat[0] == 0 && d.hat[0] != 0)
            add (c.swingTime (bar0 + 3.5), Drum::OpenHat, 84, 0.4);

        // Crash on the downbeat of the hook (and every 8 bars), and on the bridge
        if (b % 8 == 0 && (section == Hook || (section == Bridge && b == 0) || (section == Verse && b == 0 && r.chance (0.3))))
            add (bar0, Drum::Crash, section == Hook ? 112 : 95, 3.0);

        // Fills
        const bool phraseEnd = (b % 4 == 3) && c.s.density > 0.7f && ! lastBar;
        if ((lastBar && section != Intro && ! build) || phraseEnd)
        {
            const double f0 = bar0 + (phraseEnd ? 3.5 : 3.0);
            const int hits = phraseEnd ? 2 : 4;
            // clear what was there
            out.erase (std::remove_if (out.begin(), out.end(), [&] (const NoteEv& n)
                       { return n.start >= f0 - 1.0e-6 && n.start < bar0 + 4.0 && (n.pitch == Drum::Snare || n.pitch == Drum::Clap || n.pitch == Drum::Rim); }),
                       out.end());
            switch (c.g.fill)
            {
                case Fill::Toms:
                {
                    const int toms[] = { Drum::HighTom, Drum::HighTom, Drum::MidTom, Drum::LowTom };
                    for (int k = 0; k < hits; ++k)
                        add (f0 + k * 0.25, phraseEnd ? Drum::Snare : toms[k], 90 + k * 6, 0.2);
                    break;
                }
                case Fill::SnareRoll:
                    for (int k = 0; k < hits * 2; ++k) add (f0 + k * 0.125, Drum::Snare, 60 + k * 7, 0.1);
                    break;
                case Fill::HatRoll:
                    for (int k = 0; k < 6; ++k) add (f0 + 0.5 + k / 12.0, Drum::Snare, 55 + k * 10, 0.08);
                    break;
                case Fill::None: break;
            }
        }
    }

    humanise (c, out, 89, false, c.s.drumSeed);
    clampToLength (c, out);
}

// ---------------------------------------------------------------- bass
int bassRoot (int pc, bool sub)
{
    int n = 36 + wrap12 (pc - 36);        // C2..B2
    if (sub) { n -= 12; if (n < 28) n += 12; }   // E1..D#2 for 808s
    return n;
}

void buildBass (Ctx& c)
{
    auto& out = c.p->lanes[BassLane];
    const int section = c.s.section;
    const auto style = c.g.bass;
    const auto& chords = c.p->chords;
    Rng r (seedFor (c.s.bassSeed, 97, (uint64_t) section, (uint64_t) c.s.genre));

    if (style == BassStyle::KickFollow || style == BassStyle::Kick808)
    {
        std::vector<double> kicks;
        for (auto& n : c.p->lanes[DrumsLane]) if (n.pitch == Drum::Kick) kicks.push_back (n.start);
        std::sort (kicks.begin(), kicks.end());
        // also re-hit when the chord changes with no kick nearby
        for (auto& ch : chords)
        {
            bool near = false;
            for (double k : kicks) if (std::abs (k - ch.start) < 0.3) near = true;
            if (! near && ! kicks.empty()) kicks.push_back (ch.start);
        }
        std::sort (kicks.begin(), kicks.end());
        kicks.erase (std::unique (kicks.begin(), kicks.end(), [] (double a, double b) { return std::abs (a - b) < 0.05; }), kicks.end());

        const bool sub = style == BassStyle::Kick808;
        for (size_t i = 0; i < kicks.size(); ++i)
        {
            const double t = kicks[i];
            double end = i + 1 < kicks.size() ? kicks[i + 1] : c.length;
            const int ci = c.chordIndexAt (t + 0.01);
            const double chordEnd = chords[(size_t) ci].start + chords[(size_t) ci].len;
            end = std::min (end, chordEnd);
            if (! sub) end = std::min (end, t + 1.5);
            int p = bassRoot (chords[(size_t) ci].rootPc, sub);
            const bool onChord = std::abs (t - chords[(size_t) ci].start) < 0.3;
            if (! onChord && r.chance (0.15 + 0.25 * c.s.complexity)) p += r.chance (0.6) ? 12 : 7;
            out.push_back ({ t, std::max (0.1, end - t - 0.02), p, onChord ? 110 : 98 });
        }
        if (! out.empty() || section == Intro || sub) { humanise (c, out, 101, false, c.s.bassSeed); clampToLength (c, out); return; }
    }

    const auto& pats = c.g.bassRhythms;
    int idx = pickLevel (c, (int) pats.size(), 103, c.s.bassSeed);
    if (section == Intro || section == Outro || section == Bridge) idx = std::max (0, idx - 1);
    const std::string pat = pats[(size_t) idx];
    const bool walking = style == BassStyle::Walking;

    struct Open { bool on = false; double start = 0; int pitch = 0; int vel = 0; } cur;
    auto close = [&] (double t)
    {
        if (! cur.on) return;
        out.push_back ({ c.swingTime (cur.start), std::max (0.08, t - cur.start - 0.03), cur.pitch, cur.vel });
        cur.on = false;
    };

    const int steps = c.s.bars * 16;
    const int startStep = (section == Intro) ? (c.s.bars >= 8 ? steps / 2 : steps) : 0;
    for (int st = 0; st < steps; ++st)
    {
        const double t = st * 0.25;
        if (st < startStep) continue;
        const char ch = pat[(size_t) (st % 16)];
        const int ci = c.chordIndexAt (t);
        const bool chordStarts = std::abs (chords[(size_t) ci].start - t) < 1.0e-6;
        int root = bassRoot (chords[(size_t) ci].rootPc, false);

        auto noteFor = [&] (char sym)
        {
            int p = root;
            if (sym == 'o') p += 12;
            if (sym == '5') p += 7;
            if (walking && sym == 'x' && ! chordStarts && r.chance (0.25 + 0.4 * c.s.complexity))
                p += r.chance (0.5) ? 7 : 12;
            return p;
        };

        if (ch == 'x' || ch == 'o' || ch == '5')
        {
            close (t);
            cur = { true, t, noteFor (ch), (st % 4 == 0) ? 108 : 94 };
        }
        else if (ch == '-')
        {
            if (chordStarts && cur.on) { close (t); cur = { true, t, root, 104 }; }
            else if (chordStarts && ! cur.on) cur = { true, t, root, 104 };
        }
        else
        {
            close (t);
            if (chordStarts) cur = { true, t, root, 104 };
        }

        // Walking bass: approach the next chord from a half-step / scale step away
        if (walking && ci + 1 < (int) chords.size())
        {
            const double nextStart = chords[(size_t) ci + 1].start;
            if (std::abs (nextStart - (t + 0.5)) < 1.0e-6 && r.chance (0.35 + 0.4 * c.s.complexity))
            {
                close (t);
                const int target = bassRoot (chords[(size_t) ci + 1].rootPc, false);
                const int approach = r.chance (0.5) ? target - 1 : (r.chance (0.5) ? target + 1 : target - 2);
                cur = { true, t, approach, 90 };
                close (t + 0.45);
                st += 1;   // skip the next 16th, the approach note covers it
            }
        }
    }
    close (c.length);

    humanise (c, out, 107, false, c.s.bassSeed);
    clampToLength (c, out);
}


// ---------------------------------------------------------------- groove
// Timing offsets are in 16ths (+ = late), velocity factors per 16th step of the bar.
constexpr double kTwoPi = 6.283185307179586;
struct Feel { float t[16]; float v[16]; float drift; float jitter; };

const Feel& feelFor (int feel, int genre)
{
    static const Feel feels[] = {
        // 1 Quantized
        { { 0 }, { 1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1 }, 0.0f, 0.0f },
        // 2 Tight: on the grid, real accents
        { { 0 }, { 1,.72f,.86f,.72f, .96f,.72f,.86f,.72f, 1,.72f,.86f,.72f, .96f,.72f,.86f,.74f }, 0.0f, 0.15f },
        // 3 Laid back: offbeats sit behind
        { { .1f,.22f,.18f,.26f, .1f,.22f,.18f,.26f, .1f,.22f,.18f,.26f, .1f,.22f,.18f,.26f },
          { 1,.7f,.84f,.7f, .95f,.7f,.84f,.7f, 1,.7f,.84f,.7f, .95f,.7f,.84f,.72f }, 0.0f, 0.3f },
        // 4 Pushed: offbeats rush a little
        { { 0,-.14f,-.1f,-.16f, 0,-.14f,-.1f,-.16f, 0,-.14f,-.1f,-.16f, 0,-.14f,-.1f,-.16f },
          { 1,.76f,.95f,.76f, .92f,.76f,.95f,.76f, 1,.76f,.95f,.76f, .92f,.76f,.95f,.8f }, 0.0f, 0.2f },
        // 5 Dilla: loose, drunk, snare late
        { { 0,.3f,.12f,.4f, .2f,.32f,.06f,.44f, -.06f,.26f,.16f,.36f, .24f,.38f,.1f,.48f },
          { 1,.62f,.85f,.7f, .95f,.68f,.8f,.64f, 1,.7f,.86f,.74f, .95f,.66f,.82f,.72f }, 0.2f, 0.6f },
        // 6 Shuffle: triplet 8ths
        { { 0,.33f,.67f,.33f, 0,.33f,.67f,.33f, 0,.33f,.67f,.33f, 0,.33f,.67f,.33f },
          { 1,.66f,.8f,.66f, .95f,.66f,.8f,.66f, 1,.66f,.8f,.66f, .95f,.66f,.8f,.68f }, 0.0f, 0.2f },
        // 7 Bounce: trap / future bass hat accents, a touch of lean
        { { 0,.08f,.02f,.1f, 0,.08f,.02f,.1f, 0,.08f,.02f,.1f, 0,.08f,.02f,.1f },
          { 1,.58f,.82f,.6f, .96f,.6f,.84f,.64f, 1,.58f,.82f,.6f, .96f,.6f,.86f,.7f }, 0.0f, 0.15f },
        // 8 Afro / Dembow: 3-3-2 accents
        { { 0,.06f,.04f,-.05f, .04f,.06f,-.05f,.06f, 0,.06f,.04f,-.05f, .04f,.06f,-.05f,.06f },
          { 1,.66f,.72f,.96f, .76f,.66f,.96f,.7f, .98f,.66f,.72f,.96f, .76f,.66f,.96f,.72f }, 0.0f, 0.25f },
        // 9 Live drummer: small drift, real dynamics
        { { 0,.06f,.04f,.08f, .03f,.06f,.04f,.08f, 0,.06f,.04f,.08f, .03f,.06f,.04f,.08f },
          { 1,.68f,.84f,.7f, .97f,.68f,.84f,.7f, 1,.68f,.84f,.7f, .97f,.7f,.86f,.76f }, 0.5f, 0.6f },
        // 10 Rubato: phrasing that breathes (classical / cinematic)
        { { 0 }, { 1,.8f,.9f,.8f, .94f,.8f,.9f,.8f, .97f,.8f,.9f,.8f, .92f,.8f,.9f,.82f }, 1.6f, 0.4f },
    };
    static const int genreDefault[] = { 2, 2, 9, 10, 10, 2, 7, 2, 3, 7, 7, 3, 5, 5, 9, 8, 8 };
    int f = feel;
    if (f <= 0) f = genreDefault[std::clamp (genre, 0, (int) (sizeof (genreDefault) / sizeof (int)) - 1)];
    return feels[std::clamp (f, 1, 10) - 1];
}

double hashNoise (uint32_t seed, uint64_t a, uint64_t b)
{
    return (double) (seedFor (seed, 307, a, b) >> 11) * (1.0 / 9007199254740992.0) * 2.0 - 1.0;
}

// Spread chord notes like a hand on keys / a guitar strum, and balance the voicing.
void strumChords (Ctx& c)
{
    auto& notes = c.p->lanes[ChordsLane];
    const double amt = c.s.strum, dyn = c.s.dynamics;
    std::sort (notes.begin(), notes.end(), [] (const NoteEv& a, const NoteEv& b)
               { return a.start < b.start - 1.0e-9 || (std::abs (a.start - b.start) <= 1.0e-9 && a.pitch < b.pitch); });
    size_t i = 0;
    while (i < notes.size())
    {
        size_t j = i;
        while (j < notes.size() && std::abs (notes[j].start - notes[i].start) < 1.0e-6) ++j;
        const size_t n = j - i;
        if (n > 1)
        {
            const double t = notes[i].start;
            const bool onBeat = std::abs (t - std::round (t)) < 0.02;
            const bool up = ! onBeat && hashNoise (c.s.harmonySeed, 1, (uint64_t) std::llround (t * 96)) > 0.0;   // up-strokes on offbeats
            const double step = std::min (0.03 * amt, 0.12 / (double) (n - 1));
            for (size_t k = 0; k < n; ++k)
            {
                auto& nt = notes[i + k];
                const size_t order = up ? (n - 1 - k) : k;
                const double d = step * (double) order;
                nt.start += d;
                nt.len = std::max (0.05, nt.len - d);
                // top voice sings, inner voices sit back
                const double f = k == n - 1 ? 1.0 + 0.08 * dyn : k == 0 ? 1.0 - 0.04 * dyn : 1.0 - 0.1 * dyn;
                nt.vel = std::clamp ((int) std::lround (nt.vel * f), 1, 127);
            }
        }
        i = j;
    }
}

void applyGroove (Ctx& c)
{
    const Feel& f = feelFor (c.s.feel, c.s.genre);
    const double amt = c.s.feelAmount, dyn = c.s.dynamics, pocket = c.s.pocket;
    const double laneTiming[] = { 0.6, 0.8, 1.0, 1.0 };

    double melodyMean = 0;
    for (auto& n : c.p->lanes[MelodyLane]) melodyMean += n.pitch;
    if (! c.p->lanes[MelodyLane].empty()) melodyMean /= (double) c.p->lanes[MelodyLane].size();

    for (int l = 0; l < kNumLanes; ++l)
    {
        const uint32_t laneSeed = l == ChordsLane ? c.s.harmonySeed : l == MelodyLane ? c.s.melodySeed
                                : l == BassLane ? c.s.bassSeed : c.s.drumSeed;
        for (auto& n : c.p->lanes[(size_t) l])
        {
            const double pos = n.start * 4.0;
            const long long st = std::llround (pos);
            const bool onGrid = std::abs (pos - (double) st) < 0.4;
            const int step = (int) (((st % 16) + 16) % 16);
            const uint64_t key = (uint64_t) std::llround (n.start * 96.0);   // same for every note of a chord

            // ---- timing
            double off = 0.0;
            if (onGrid) off += amt * f.t[step] * 0.25 * laneTiming[l];

            double pw = 0.0;   // who sits where in the pocket: kick is the anchor
            if (l == ChordsLane) pw = 0.6;
            else if (l == MelodyLane) pw = 1.0;
            else if (l == BassLane) pw = 0.2;
            else if (n.pitch == Drum::Snare || n.pitch == Drum::Clap || n.pitch == Drum::Rim) pw = 1.0;
            else if (n.pitch == Drum::ClosedHat || n.pitch == Drum::OpenHat || n.pitch == Drum::Shaker) pw = 0.6;
            else if (n.pitch != Drum::Kick) pw = 0.4;
            off += pocket * 0.035 * pw;

            if (f.drift > 0.0f) off += amt * f.drift * 0.02 * std::sin (n.start * kTwoPi / 8.0 + l * 1.3);
            if (f.jitter > 0.0f) off += amt * f.jitter * 0.014 * hashNoise (laneSeed, (uint64_t) l + 11, key);

            // ---- dynamics
            const double accent = onGrid ? f.v[step] : 0.86;
            double m = 1.0 + (accent - 1.0) * (0.25 + 1.1 * dyn) * (0.5 + 0.5 * amt);
            const int barInPhrase = (int) std::floor (n.start / 4.0) % 4;
            if (step == 0) m *= 1.0 + dyn * (barInPhrase == 0 ? 0.08 : 0.03);
            if (f.drift > 1.0f) m *= 1.0 + dyn * 0.12 * std::sin (n.start * kTwoPi / 16.0 - 1.2);   // rubato swells
            if (l == MelodyLane) m *= 1.0 + dyn * 0.012 * ((double) n.pitch - melodyMean);              // high notes sing out
            if (l == DrumsLane && (n.pitch == Drum::ClosedHat || n.pitch == Drum::Shaker))
                m *= 1.0 + 0.08 * dyn * hashNoise (laneSeed, 41, key);

            n.vel = std::clamp ((int) std::lround (n.vel * m), 1, 127);
            n.start = std::clamp (n.start + off, 0.0, c.length - 0.02);
        }
        clampToLength (c, c.p->lanes[(size_t) l]);
    }
}

Pattern composeSection (const Settings& in)
{
    Settings s = in;
    s.genre   = std::clamp (s.genre, 0, (int) genres().size() - 1);
    s.section = std::clamp (s.section, 0, kNumSections - 1);
    s.bars    = s.bars >= 16 ? 16 : s.bars >= 8 ? 8 : 4;
    s.key     = wrap12 (s.key);

    const Genre& g = genres()[(size_t) s.genre];
    Pattern p;
    p.bars = s.bars;
    p.lengthBeats = s.bars * 4.0;

    Ctx c (g);
    c.s = s;
    c.p = &p;
    c.keyPc = s.key;
    c.length = p.lengthBeats;
    c.energy = sectionEnergy (s.section);
    c.tonicMidi = 60 + (s.key <= 5 ? s.key : s.key - 12);
    c.swing = std::clamp ((double) g.swing * std::min (1.0, s.groove * 2.0) + std::max (0.0, (double) s.groove - 0.5) * 0.6, 0.0, 1.0);

    resolveMode (g, s, c.minor, c.scaleId);
    buildHarmony (c);
    buildChordRhythm (c);
    buildMelody (c);
    buildDrums (c);
    buildBass (c);
    strumChords (c);
    applyGroove (c);
    return p;
}
} // namespace

Pattern compose (const Settings& s)
{
    return composeSection (s);
}

Pattern composeSong (const Settings& in)
{
    struct Part { int section; int bars; uint32_t salt; };
    static const Part parts[] = { { Intro, 4, 0 }, { Verse, 8, 0 }, { PreChorus, 4, 0 }, { Hook, 8, 0 },
                                  { Verse, 8, 1 }, { PreChorus, 4, 0 }, { Hook, 8, 0 }, { Bridge, 8, 0 },
                                  { Hook, 8, 0 }, { Outro, 4, 0 } };
    Pattern song;
    song.bars = 0;
    double offset = 0.0;
    for (auto& part : parts)
    {
        Settings s = in;
        s.section = part.section;
        s.bars = part.bars;
        if (part.salt) { s.melodySeed ^= 0x5a5a5a5au; s.drumSeed ^= 0x13579bdu; }
        Pattern p = composeSection (s);

        song.markers.push_back ({ offset, sectionNames()[(size_t) part.section] });
        for (auto ch : p.chords) { ch.start += offset; song.chords.push_back (ch); }
        for (int l = 0; l < kNumLanes; ++l)
            for (auto n : p.lanes[(size_t) l]) { n.start += offset; song.lanes[(size_t) l].push_back (n); }
        if (song.progressionName.empty()) { song.minor = p.minor; song.scale = p.scale; }
        song.progressionName = "Full song";
        offset += p.lengthBeats;
        song.bars += p.bars;
    }
    song.lengthBeats = offset;
    return song;
}


// =============================================================================
// MIDI import: find the key and the chords in a chord pack's MIDI file
// =============================================================================
std::string nameChord (const std::vector<int>& pitches, int* rootPc)
{
    if (pitches.empty()) return {};
    std::vector<NoteEv> n;
    for (int p : pitches) n.push_back ({ 0.0, 4.0, p, 90 });
    auto imp = analyseImport (n, 4.0, "");
    if (imp->spans.empty()) return {};
    const auto& sp = imp->spans.front();
    if (rootPc != nullptr) *rootPc = sp.rootPc;
    return noteName (sp.rootPc, sp.rootPc == 1 || sp.rootPc == 3 || sp.rootPc == 8 || sp.rootPc == 10) + qualitySuffix (sp.q);
}

bool keyFromName (const std::string& name, int& pc, bool& minor)
{
    std::string tok;
    bool found = false;
    auto check = [&] (const std::string& t)
    {
        if (t.empty() || t[0] < 'A' || t[0] > 'G') return;
        static const int base[] = { 9, 11, 0, 2, 4, 5, 7 };   // A B C D E F G
        int p = base[t[0] - 'A'];
        size_t i = 1;
        if (i < t.size() && (t[i] == '#' || t[i] == 'b')) { p += t[i] == '#' ? 1 : -1; ++i; }
        std::string rest = t.substr (i);
        for (auto& ch : rest) ch = (char) std::tolower ((unsigned char) ch);
        bool m;
        if (rest.empty() || rest == "maj" || rest == "major") m = false;
        else if (rest == "m" || rest == "min" || rest == "minor") m = true;
        else return;
        pc = wrap12 (p); minor = m; found = true;   // the last key-like word wins
    };
    for (char ch : name)
    {
        if (ch == ' ' || ch == '_' || ch == '-' || ch == '(' || ch == ')' || ch == '[' || ch == ']' || ch == '.' || ch == ',')
        { check (tok); tok.clear(); }
        else tok += ch;
    }
    check (tok);
    return found;
}

std::shared_ptr<ImportedProgression> analyseImport (std::vector<NoteEv> notes, double lengthBeats, const std::string& name,
                                                    int keyHint, int minorHint)
{
    auto imp = std::make_shared<ImportedProgression>();
    imp->name = name;
    notes.erase (std::remove_if (notes.begin(), notes.end(), [] (const NoteEv& n) { return n.len <= 0.0; }), notes.end());
    if (notes.empty()) return imp;

    // start on the first bar that has notes
    double first = 1e9;
    for (auto& n : notes) first = std::min (first, n.start);
    const double barShift = std::floor (first / 4.0 + 1.0e-6) * 4.0;
    double end = 0;
    for (auto& n : notes) { n.start -= barShift; end = std::max (end, n.start + n.len); }
    double len = std::max (lengthBeats - barShift, end);
    len = std::max (4.0, std::ceil (len / 4.0 - 0.05) * 4.0);
    imp->lengthBeats = len;
    std::sort (notes.begin(), notes.end(), [] (const NoteEv& a, const NoteEv& b) { return a.start < b.start; });
    imp->notes = notes;

    // ---- key: Krumhansl-Schmuckler on duration-weighted pitch classes
    double hist[12] = {};
    for (auto& n : notes) hist[wrap12 (n.pitch)] += n.len;
    static const double majP[] = { 6.35, 2.23, 3.48, 2.33, 4.38, 4.09, 2.52, 5.19, 2.39, 3.66, 2.29, 2.88 };
    static const double minP[] = { 6.33, 2.68, 3.52, 5.38, 2.60, 3.53, 2.54, 4.75, 3.98, 2.69, 3.34, 3.17 };
    auto corr = [&] (const double* prof, int tonic)
    {
        double mx = 0, my = 0;
        for (int i = 0; i < 12; ++i) { mx += hist[(i + tonic) % 12]; my += prof[i]; }
        mx /= 12; my /= 12;
        double sxy = 0, sxx = 0, syy = 0;
        for (int i = 0; i < 12; ++i)
        {
            const double x = hist[(i + tonic) % 12] - mx, y = prof[i] - my;
            sxy += x * y; sxx += x * x; syy += y * y;
        }
        return sxy / std::sqrt (std::max (1e-12, sxx * syy));
    };
    double best = -2;
    for (int t = 0; t < 12; ++t)
    {
        const double a = corr (majP, t), b = corr (minP, t);
        if (a > best) { best = a; imp->tonicPc = t; imp->minor = false; }
        if (b > best) { best = b; imp->tonicPc = t; imp->minor = true; }
    }

    // ---- chords: one guess per beat, then merge repeats
    static const Q candidates[] = { Q::Maj, Q::Min, Q::Dim, Q::Aug, Q::Sus2, Q::Sus4, Q::Dom7, Q::Maj7, Q::Min7,
                                    Q::HalfDim, Q::Dim7, Q::Add9, Q::MinAdd9, Q::Six, Q::Min6, Q::Dom9, Q::Maj9,
                                    Q::Min9, Q::Min11, Q::Sus7 };
    struct Guess { int root; Q q; double score; };
    std::vector<Guess> beats;
    const int nBeats = (int) std::lround (len);
    for (int bt = 0; bt < nBeats; ++bt)
    {
        double w[12] = {};
        int lowest = 128;
        for (auto& n : notes)
        {
            const double ov = std::min (n.start + n.len, bt + 1.0) - std::max (n.start, (double) bt);
            if (ov <= 0.01) continue;
            w[wrap12 (n.pitch)] += ov;
            lowest = std::min (lowest, n.pitch);
        }
        double total = 0, mxw = 0;
        for (double x : w) { total += x; mxw = std::max (mxw, x); }
        if (total <= 0.0)
        {
            beats.push_back (beats.empty() ? Guess { imp->tonicPc, imp->minor ? Q::Min : Q::Maj, 0 } : beats.back());
            continue;
        }
        Guess g { 0, Q::Maj, -1e9 };
        for (int r = 0; r < 12; ++r)
            for (Q q : candidates)
            {
                std::set<int> tones;
                for (int iv : intervals (q)) tones.insert (wrap12 (r + iv));
                double sc = 0;
                for (int pc = 0; pc < 12; ++pc) sc += tones.count (pc) ? w[pc] : -0.8 * w[pc];
                for (int t : tones) if (w[t] < 0.05 * mxw) sc -= 0.45 * mxw;   // missing chord tone
                sc -= 0.03 * mxw * (double) tones.size();                      // prefer simpler names
                if (lowest < 128 && wrap12 (lowest) == r) sc += 0.25 * mxw;    // the bass note is usually the root
                if (sc > g.score) g = { r, q, sc };
            }
        beats.push_back (g);
    }

    for (int bt = 0; bt < nBeats; ++bt)
    {
        const auto& g = beats[(size_t) bt];
        if (! imp->spans.empty() && imp->spans.back().rootPc == g.root && imp->spans.back().q == g.q)
        {
            imp->spans.back().len += 1.0;
            continue;
        }
        ImportedProgression::Span sp;
        sp.start = bt; sp.len = 1.0; sp.rootPc = g.root; sp.q = g.q;
        imp->spans.push_back (sp);
    }
    // absorb one-beat blips (passing notes in arps) into the chord before them
    for (size_t i = 1; i + 1 < imp->spans.size();)
    {
        if (imp->spans[i].len < 1.5 && imp->spans[i - 1].len >= 2.0)
        {
            imp->spans[i - 1].len += imp->spans[i].len;
            imp->spans.erase (imp->spans.begin() + (long) i);
            if (imp->spans[i - 1].rootPc == imp->spans[i].rootPc && imp->spans[i - 1].q == imp->spans[i].q)
            {
                imp->spans[i - 1].len += imp->spans[i].len;
                imp->spans.erase (imp->spans.begin() + (long) i);
            }
        }
        else ++i;
    }
    // Relative major / minor: the notes can't tell C major from A minor, the music can.
    // A key in the file name wins, as long as the notes don't clearly disagree (packs are sometimes mislabelled)
    bool hintUsed = false;
    if (keyHint >= 0)
    {
        const bool hm = minorHint == 1;
        const double hc = corr (hm ? minP : majP, wrap12 (keyHint));
        if (hc >= best - 0.15)
        {
            imp->tonicPc = wrap12 (keyHint);
            if (minorHint >= 0) imp->minor = hm;
            hintUsed = true;
        }
    }
    if (! hintUsed && ! imp->spans.empty())
    {
        const auto& f = imp->spans.front();
        const bool fMinor = f.q == Q::Min || f.q == Q::Min7 || f.q == Q::Min9 || f.q == Q::Min11 || f.q == Q::MinAdd9 || f.q == Q::Min6;
        if (! imp->minor && fMinor && f.rootPc == wrap12 (imp->tonicPc + 9)) { imp->tonicPc = f.rootPc; imp->minor = true; }
        else if (imp->minor && ! fMinor && f.rootPc == wrap12 (imp->tonicPc + 3)) { imp->tonicPc = f.rootPc; imp->minor = false; }
    }

    // the actual notes of each chord (its voicing in the pack)
    for (auto& sp : imp->spans)
    {
        std::set<int> ps;
        for (auto& n : notes)
            if (n.start < sp.start + sp.len - 0.05 && n.start + n.len > sp.start + 0.05) ps.insert (n.pitch);
        if (ps.empty()) for (int iv : intervals (sp.q)) ps.insert (60 + wrap12 (sp.rootPc) + iv);
        sp.pitches.assign (ps.begin(), ps.end());
        if (sp.pitches.size() > 8) sp.pitches.resize (8);
    }
    return imp;
}

std::array<PadChord, 12> chordBank (const Settings& in)
{
    Settings s = in;
    s.genre = std::clamp (s.genre, 0, (int) genres().size() - 1);
    const Genre& g = genres()[(size_t) s.genre];

    bool minor = false; int scaleId = 1;
    resolveMode (g, s, minor, scaleId);
    if (s.progression > 0 && s.progression <= (int) g.progs.size())
        minor = g.progs[(size_t) s.progression - 1].minor;

    // white keys: the scale's own chords. black keys: borrowed colour chords.
    static const char* majW[] = { "I", "ii", "iii", "IV", "V", "vi", "viidim" };
    // black keys sit on the key that matches the borrowed chord's root (in C: Eb, Ab, Bb)
    static const char* majB[] = { "II", "bIII", "iv", "bVI", "bVII" };
    static const char* minW[] = { "i", "iidim", "bIII", "iv", "v", "bVI", "bVII" };
    static const char* minB[] = { "bII", "I", "IV", "V", "viidim7" };
    static const int whiteSlots[] = { 0, 2, 4, 5, 7, 9, 11 };
    static const int blackSlots[] = { 1, 3, 6, 8, 10 };

    std::array<PadChord, 12> bank;

    if (s.imported != nullptr && ! s.imported->spans.empty())
    {
        // Like loading a chord pack into Cthulhu: the file's chords go on C, D, E, F, G, A, B, then the black keys
        static const int order[] = { 0, 2, 4, 5, 7, 9, 11, 1, 3, 6, 8, 10 };
        const auto& imp = *s.imported;
        const int shift = importShiftFor (imp.tonicPc, wrap12 (s.key));
        std::vector<const ImportedProgression::Span*> unique;
        for (auto& sp : imp.spans)
        {
            bool seen = false;
            for (auto* u : unique) if (u->rootPc == sp.rootPc && u->q == sp.q) seen = true;
            if (! seen && unique.size() < 12) unique.push_back (&sp);
        }
        for (size_t i = 0; i < 12; ++i)
        {
            const auto* sp = unique[i % unique.size()];
            PadChord pc;
            pc.rootPc = wrap12 (sp->rootPc + shift);
            pc.name = chordName (pc.rootPc, sp->q, wrap12 (s.key), imp.minor, 0);
            pc.roman = i < unique.size() ? romanFor (wrap12 (pc.rootPc - s.key), sp->q) : "";
            pc.intervals = intervals (sp->q);
            for (int pch : sp->pitches) pc.fixedNotes.push_back (pch + shift);
            if (i >= unique.size()) { pc.name = ""; pc.fixedNotes.clear(); pc.intervals.clear(); }
            bank[(size_t) order[i]] = pc;
        }
        return bank;
    }

    auto fill = [&] (int slot, const char* token)
    {
        auto pc = parseRoman (token);
        Rng er (seedFor (s.harmonySeed, 41, (uint64_t) slot * 7u + 3u, (uint64_t) s.genre));
        const Q q = pc.explicitQ ? pc.q : extend (pc.q, pc.rootRel, g.extensions, s.complexity, er);
        PadChord p;
        p.rootPc = wrap12 (s.key + pc.rootRel);
        p.roman = token;
        p.name = chordName (p.rootPc, q, s.key, minor, pc.accidental);
        p.intervals = intervals (q);
        bank[(size_t) slot] = p;
    };
    for (int i = 0; i < 7; ++i) fill (whiteSlots[i], minor ? minW[i] : majW[i]);
    for (int i = 0; i < 5; ++i) fill (blackSlots[i], minor ? minB[i] : majB[i]);
    return bank;
}

} // namespace hs
