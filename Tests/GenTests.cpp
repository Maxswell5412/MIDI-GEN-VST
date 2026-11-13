#include "../Source/Reharm.h"
#include <cmath>
#include <cstdio>
#include <set>
using namespace hs;
static int failures = 0;
#define CHECK(c, ...) do { if (!(c)) { ++failures; std::printf ("FAIL %d: ", __LINE__); std::printf (__VA_ARGS__); std::printf ("\n"); } } while (0)

int main()
{
    // Source: Am F C G, one bar each, pushed 8th-note rhythm like a HOOKSMITH clip
    std::vector<NoteEv> n;
    const int ch[4][4] = { { 57, 60, 64, 69 }, { 53, 57, 60, 65 }, { 55, 60, 64, 67 }, { 55, 59, 62, 67 } };
    for (int bar = 0; bar < 4; ++bar)
        for (double t : { 0.0, 1.5, 3.0 })
            for (int p : ch[bar]) n.push_back ({ bar * 4 + t, 1.2, p, 96 });
    auto src = analyseImport (n, 16, "Am F C G");
    std::printf ("learned: key %d %s, %zu chords\n", src->tonicPc, src->minor ? "minor" : "major", src->spans.size());
    CHECK (src->spans.size() == 4, "4 chords learned");

    std::set<double> origHits; for (auto& x : n) origHits.insert (x.start);
    const int scaleMin[] = { 0, 2, 3, 5, 7, 8, 10 };

    for (int style = 0; style < (int) styleNames().size(); ++style)
    {
        GenSettings s; s.style = style; s.seed = 1234;
        auto vars = generateVariations (*src, s, 8);
        CHECK (vars.size() == 8, "8 variations");
        std::set<std::string> sigs;
        int inKey = 0, total = 0;
        for (auto& v : vars)
        {
            std::string sig; double t = 0;
            for (auto& c : v.chords)
            {
                CHECK (std::abs (c.start - t) < 1e-6, "%s: chords are back to back", styleNames()[(size_t) style].c_str());
                t = c.start + c.len;
                sig += c.name + " ";
                CHECK (! c.name.empty() && c.notes.size() >= 3, "named, voiced");
                for (int p : c.notes) { CHECK (p >= 24 && p <= 96, "note range %d", p); ++total; for (int k : scaleMin) if ((p - 9 + 120) % 12 == k) { ++inKey; break; } }
            }
            CHECK (std::abs (t - 16.0) < 1e-6, "covers the loop");
            sigs.insert (sig);
            // rhythm kept
            for (auto& e : renderVariation (*src, v, true))
            {
                bool ok = origHits.count (e.start) > 0;
                for (auto& c : v.chords) if (std::abs (c.start - e.start) < 1e-6) ok = true;
                CHECK (ok, "hit at %.2f follows the original rhythm", e.start);
            }
        }
        CHECK (sigs.size() == 8, "%s: 8 different progressions (%zu)", styleNames()[(size_t) style].c_str(), sigs.size());
        CHECK (inKey >= total * 0.6, "%s: mostly in key (%d/%d)", styleNames()[(size_t) style].c_str(), inKey, total);
        std::printf ("\n%s:\n", styleNames()[(size_t) style].c_str());
        for (size_t i = 0; i < 4; ++i)
        {
            std::printf ("  %s  ", vars[i].label.c_str());
            for (auto& c : vars[i].chords) std::printf ("%-9s", c.name.c_str());
            std::printf ("  [%s]\n", vars[i].tags.c_str());
        }
    }

    // similarity 1, no spice, no bass motion: same roots as the original
    {
        GenSettings s; s.similarity = 1.0f; s.spice = 0.0f; s.bassMotion = 0.0f; s.style = 1;
        auto vars = generateVariations (*src, s, 3);
        for (auto& v : vars)
            for (size_t i = 0; i < v.chords.size(); ++i)
                CHECK (v.chords[i].rootPc == src->spans[i].rootPc, "similarity 1 keeps the roots");
        std::printf ("\nsimilarity 1 (recolour only): ");
        for (auto& c : vars[0].chords) std::printf ("%s ", c.name.c_str());
        std::printf ("\n");
    }
    // similarity 0: mostly new chords
    {
        GenSettings s; s.similarity = 0.0f; s.spice = 0.0f;
        int changed = 0, totalC = 0;
        for (auto& v : generateVariations (*src, s, 8)) for (auto& c : v.chords) { ++totalC; if (c.changed) ++changed; }
        CHECK (changed >= totalC / 2, "similarity 0 changes most chords (%d/%d)", changed, totalC);
    }
    std::printf ("%d failures\n", failures);
    return failures ? 1 : 0;
}
