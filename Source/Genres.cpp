#include "Genres.h"

// -----------------------------------------------------------------------------
// The genre library: harmony language, rhythms, melody feel, bass and drums.
// All progressions are written in Roman numerals so they work in every key.
// Upper case = major, lower case = minor, b/# = borrowed root, suffixes such as
// 7, maj7, 9, add9, sus4, dim, m7b5 spell the chord exactly. Plain triads get
// extensions added automatically according to the genre and the COMPLEXITY knob.
// -----------------------------------------------------------------------------
namespace hs::detail
{
namespace
{
constexpr const char* _ = "";   // empty drum line

std::vector<Genre> build()
{
    std::vector<Genre> g;

    // ------------------------------------------------------------------ POP
    g.push_back ({ "Pop", 110, 0.35f, 0.05f, 1, Voicing::Close,
        { { 1.0, 0.85 }, { 2.0, 0.15 } },
        { "x---------------", "x-------x-------", "x-----x-----p---", "x--x--x---x--x--", "x.x.x.x.x.x.x.x." },
        { 5.0f, 0.4f, true, 0.2f, 0.2f, 0.5f, 0.05f },
        BassStyle::Pattern,
        { "x---------------", "x-------x-------", "x---x---x-x-x---", "x.x.x.x.x.x.x.x.", "x.xox.x.x.xox.x." },
        true,
        { "X.......x.x.....", "....X.......X...", _, "x.x.x.x.x.x.x.x.", _, _, _, _ },
        { "X.......x.x...x.", "....X.......X..g", _, "x.x.x.x.x.x.x.x.", "..............x.", _, _, _ },
        false, Fill::Toms,
        {
            { "Four Chord Anthem",   "I V vi IV",        false, H | V },
            { "Sensitive",           "vi IV I V",        false, H | V },
            { "Doo-Wop",             "I vi IV V",        false, V },
            { "Lift Off",            "IV I V vi",        false, H },
            { "Royal Road",          "IVmaj7 V iii vi",  false, H | V },
            { "Climb",               "ii iii IV V",      false, P },
            { "Build Up",            "IV IV V V",        false, P },
            { "Turnaround",          "vi V IV V",        false, B },
            { "Borrowed Lift",       "bVI bVII I I",     false, B | P },
            { "Midnight",            "i bVI bIII bVII",  true,  H | V },
            { "Heartbreak",          "i bVII bVI bVII",  true,  V | H },
            { "Rise",                "bVI bVII i i",     true,  P },
            { "Night Bridge",        "bIII bVII iv bVI", true,  B },
            { "Tension",             "iv bVI bVII bVII", true,  P | B },
        } });

    // ------------------------------------------------------------ DANCE POP
    g.push_back ({ "Dance Pop", 122, 0.45f, 0.0f, 1, Voicing::Close,
        { { 1.0, 0.9 }, { 2.0, 0.1 } },
        { "x---------------", "..x...x...x...x.", "x--x--x-x--x--x-", "..xx..xx..xx..xx" },
        { 5.0f, 0.5f, true, 0.2f, 0.3f, 0.4f, 0.0f },
        BassStyle::Pattern,
        { "x-------x-------", "..x...x...x...x.", "x.x.x.x.x.x.x.x.", ".xxx.xxx.xxx.xxx" },
        true,
        { "X...X...X...X...", _, "....X.......X...", "xg.gxg.gxg.gxg.g", "..x...x...x...x.", _, _, _ },
        { "X...X...X...X...", _, "....X.......X.x.", "xg.gxg.gxg.gxg.g", "..x...x...x...x.", "x.x.x.x.x.x.x.x.", _, _ },
        false, Fill::SnareRoll,
        {
            { "Club Anthem",         "I V vi IV",        false, H },
            { "Feel It",             "vi IV I V",        false, H | V },
            { "Sunset Drive",        "IV V vi I",        false, V | H },
            { "Glow",                "I iii IV V",       false, V },
            { "Push",                "IV V",             false, P },
            { "Night Out",           "i bVI bIII bVII",  true,  H | V },
            { "Obsessed",            "i bVII bVI bVII",  true,  H },
            { "Pulse",               "i iv bVII bIII",   true,  V },
            { "Climb Minor",         "bVI bVII",         true,  P },
            { "Breakdown",           "IV vi V V",        false, B },
            { "Minor Breakdown",     "iv bVI bVII bVII", true,  B },
        } });

    // -------------------------------------------------------------- POP ROCK
    g.push_back ({ "Pop Rock", 128, 0.3f, 0.0f, 0, Voicing::Close,
        { { 1.0, 0.8 }, { 2.0, 0.2 } },
        { "x---------------", "x-------x-------", "x.x.x.x.x.x.x.x.", "x-xx-xx-x-xx-xx-" },
        { 4.0f, 0.3f, false, 0.2f, 0.2f, 0.6f, 0.0f },
        BassStyle::Pattern,
        { "x-------x-------", "x.x.x.x.x.x.x.x.", "x.x.x.x.x.x.x5x.", "xxxxxxxxxxxxxxxx" },
        true,
        { "X.....x.X.x.....", "....X.......X...", _, "x.x.x.x.x.x.x.x.", _, _, _, _ },
        { "X.....x.X.x...x.", "....X.......X.gX", _, "x.x.x.x.x.x.x.x.", "..............x.", _, _, _ },
        false, Fill::Toms,
        {
            { "Stadium",             "I V vi IV",        false, H },
            { "Mixolydian Rock",     "I bVII IV I",      false, V | H },
            { "Open Road",           "I IV vi V",        false, V },
            { "Ballad",              "vi IV I V",        false, H },
            { "Lift",                "IV V",             false, P },
            { "Climb",               "ii IV V V",        false, P },
            { "Big Bridge",          "bVI bVII I I",     false, B },
            { "Fall",                "vi V IV IV",       false, B },
            { "Descent",             "i bVII bVI V",     true,  H | V },
            { "Garage",              "i bIII bVII IV",   true,  V },
            { "Dark Climb",          "bVI bVII",         true,  P },
        } });

    // ------------------------------------------------------------- CLASSICAL
    g.push_back ({ "Classical", 90, 0.4f, 0.0f, 0, Voicing::Close,
        { { 1.0, 0.5 }, { 2.0, 0.5 } },
        { "x---------------", "x-------x-------", "x---x---x---x---" },
        { 6.0f, 0.1f, true, 0.15f, 0.1f, 0.8f, 0.0f },
        BassStyle::Pattern,
        { "x---------------", "x-------5-------", "x---5---o---5---" },
        false,
        { _, _, _, _, _, _, _, _ }, { _, _, _, _, _, _, _, _ },
        false, Fill::None,
        {
            { "Perfect Cadence",     "I IV V7 I",                 false, V | H },
            { "Canon",               "I V vi iii IV I IV V",      false, H | V },
            { "Circle of Fifths",    "I IV viidim iii vi ii V7 I", false, B | H },
            { "Romance",             "I vi ii V7",                false, V },
            { "Plagal",              "I IV I V7",                 false, H },
            { "Half Cadence",        "I ii V V",                  false, P },
            { "Lament",              "i iv V7 i",                 true,  H | V },
            { "Minor Circle",        "i iv bVII bIII bVI iidim V7 i", true, B | H },
            { "Neapolitan",          "i bII V7 i",                true,  B },
            { "Andalusian",          "i bVII bVI V",              true,  H | V },
            { "Dominant Prep",       "iv iv V V",                 true,  P },
        } });

    // ------------------------------------------------------------- CINEMATIC
    g.push_back ({ "Cinematic", 80, 0.75f, 0.0f, 0, Voicing::Spread,
        { { 0.5, 0.5 }, { 1.0, 0.5 } },
        { "x---------------", "x-------x-------", "x--x--x-x--x--x-" },
        { 2.5f, 0.1f, false, 0.3f, 0.1f, 0.95f, 0.0f },
        BassStyle::Pattern,
        { "x---------------", "x-------o-------" },
        true,
        { "X.......X.......", _, _, _, _, _, _, "....x.x.....x.x." },
        { "X.......X...x...", "............X.X.", _, _, _, _, _, "....x.x...x.x.xx" },
        false, Fill::Toms,
        {
            { "Epic Minor",          "i bVI bIII bVII",  true,  H },
            { "Hero Rises",          "bVI bVII i i",     true,  P | H },
            { "Dark Pulse",          "i i bVI bVI",      true,  V },
            { "Storm",               "i bIII bVII iv",   true,  V | H },
            { "Longing",             "i bVI bVII bVII",  true,  V },
            { "Fallen",              "iv bVI i bVII",    true,  B },
            { "Triumph",             "I bVII IV I",      false, H },
            { "Wonder (Lydian)",     "I II I II",        false, V },
            { "Hope",                "IV I V vi",        false, H | B },
            { "Climb to the Light",  "IV V",             false, P },
        } });

    // ----------------------------------------------------------- HOUSE / EDM
    g.push_back ({ "House / EDM", 124, 0.5f, 0.15f, 1, Voicing::Close,
        { { 1.0, 0.7 }, { 0.5, 0.3 } },
        { "x---------------", "..x...x...x...x.", "..x..x..x..x..x.", "x..x..x...x..x.." },
        { 4.0f, 0.6f, true, 0.25f, 0.3f, 0.3f, 0.0f },
        BassStyle::Pattern,
        { "..x...x...x...x.", "..x.o.x...x.o.x.", "x..x..x...x..x..", ".xox.xox.xox.xox" },
        true,
        { "X...X...X...X...", _, "....X.......X...", "xg.gxg.gxg.gxg.g", "..x...x...x...x.", "g.g.g.g.g.g.g.g.", _, _ },
        { "X...X...X...X...", _, "....X.......X..g", "xg.gxg.gxg.gxg.g", "..x...x...x...x.", "g.g.g.g.g.g.g.g.", "...x......x.....", _ },
        false, Fill::SnareRoll,
        {
            { "Festival",            "vi IV I V",        false, H },
            { "Soulful House",       "ii7 V7 Imaj7 vi7", false, V | H },
            { "Deep House",          "IVmaj7 iii7 ii7 Imaj7", false, V },
            { "Peak Time",           "i bVII bVI bVII",  true,  H },
            { "Warehouse",           "i bVI bIII bVII",  true,  H | V },
            { "Two Chord Groove",    "i iv",             true,  V },
            { "Night Drive",         "i iv bVII bIII",   true,  V },
            { "Riser",               "bVI bVII",         true,  P },
            { "Lift",                "IV V",             false, P },
            { "Breakdown",           "IV IV V V",        false, B },
            { "Minor Breakdown",     "bVI bIII bVII bVII", true, B },
        } });

    // ----------------------------------------------------------- FUTURE BASS
    g.push_back ({ "Future Bass", 150, 0.3f, 0.0f, 2, Voicing::Spread,
        { { 1.0, 1.0 } },
        { "x---------------", "x--x--x---x--x--", "x--x--x-x--x--x-", "x.xx.xx.x.xx.xx." },
        { 5.0f, 0.6f, true, 0.3f, 0.25f, 0.4f, 0.05f },
        BassStyle::Pattern,
        { "x---------------", "x-----x---x-----", "x--x--x---x--x--" },
        true,
        { "X.........x.....", "........X.......", "........X.......", "x.x.x.x.x.x.x.x.", _, _, _, _ },
        { "X.....x...x.....", "........X.....g.", "........X.......", "x.x.x.xxx.x.x.xx", _, _, _, _ },
        false, Fill::SnareRoll,
        {
            { "Kawaii",              "IVmaj7 V iii vi",  false, H },
            { "Emotional Drop",      "vi IV I V",        false, H | V },
            { "Colour Shift",        "I iii IV iv",      false, H | B },
            { "Glide",               "IV V vi vi",       false, V },
            { "Before the Drop",     "IV V iii vi",      false, P },
            { "Bittersweet",         "bVI bVII i v",     true,  H | V },
            { "Sky Bridge",          "IV iv I I",        false, B },
        } });

    // ---------------------------------------------------------------- TRANCE
    g.push_back ({ "Trance", 138, 0.85f, 0.0f, 0, Voicing::Spread,
        { { 1.0, 0.6 }, { 0.5, 0.4 } },
        { "x---------------", "..x...x...x...x.", ".xxx.xxx.xxx.xxx" },
        { 6.0f, 0.4f, true, 0.3f, 0.2f, 0.3f, 0.0f },
        BassStyle::Pattern,
        { "..x...x...x...x.", ".xxx.xxx.xxx.xxx" },
        true,
        { "X...X...X...X...", _, "....X.......X...", "g.g.g.g.g.g.g.g.", "..X...X...X...X.", _, _, _ },
        { "X...X...X...X...", _, "....X.......X...", "g.g.g.g.g.g.g.g.", "..X...X...X...X.", _, "x..x..x...x..x..", _ },
        false, Fill::SnareRoll,
        {
            { "Uplifting",           "i bVI bIII bVII",  true,  H },
            { "Euphoria",            "i bVII bVI bVII",  true,  H | V },
            { "Driving",             "i iv bVI bVII",    true,  V },
            { "Ascend",              "bVI bVII i i",     true,  P },
            { "Breakdown",           "iv bVI bVII i",    true,  B },
            { "Anthem Major",        "vi IV I V",        false, H },
        } });

    // ----------------------------------------------------- HIP HOP (BOOM BAP)
    g.push_back ({ "Hip Hop (Boom Bap)", 90, 0.7f, 0.55f, 2, Voicing::Drop2,
        { { 1.0, 0.5 }, { 0.5, 0.5 } },
        { "x---------------", "x-------x-------", "x-----x---x-----" },
        { 3.0f, 0.4f, true, 0.2f, 0.2f, 0.5f, 0.0f },
        BassStyle::KickFollow,
        { "x---------------", "x-----x---x-----" },
        true,
        { "X.........X.x...", "....X.......X...", _, "x.x.x.x.x.x.x.x.", _, _, _, _ },
        { "X......x..X....x", "....X.......X..g", _, "x.x.x.x.x.x.x.x.", "..............x.", _, _, _ },
        false, Fill::None,
        {
            { "Jazzy Loop",          "i7 iv7",                true,  V | H },
            { "Smoky",               "i7 bVImaj7",            true,  V },
            { "Golden Era",          "i7 iv7 bVII7 bIIImaj7", true,  H | V },
            { "Soul Sample",         "ii7 V7 Imaj7 vi7",      false, H | V },
            { "Dusty",               "vi7 ii7 V7 Imaj7",      false, V },
            { "Cypher",              "i7 bVI7 V7 i7",         true,  H },
            { "Wind Up",             "iv7 V7",                true,  P },
            { "Flip",                "bVImaj7 bVII7 i7 i7",   true,  B },
        } });

    // ------------------------------------------------------------------ TRAP
    g.push_back ({ "Trap", 140, 0.95f, 0.0f, 1, Voicing::Close,
        { { 0.5, 0.6 }, { 1.0, 0.4 } },
        { "x---------------", "x---------------", "x-------x-------" },
        { 4.0f, 0.5f, true, 0.25f, 0.25f, 0.3f, 0.3f },
        BassStyle::Kick808,
        { "x---------------" },
        true,
        { "X......x..x.....", _, "........X.......", "x.x.x.x.x.x.x.x.", _, _, _, _ },
        { "X......x..x...x.", _, "........X.......", "x.x.x.x.x.x.x.x.", _, _, _, _ },
        true, Fill::HatRoll,
        {
            { "Dark Two",            "i bVI",            true,  V | H },
            { "Night Shift",         "i bVI bIII bVII",  true,  H },
            { "Menace",              "i v bVI v",        true,  V },
            { "Cold",                "i iv bVI v",       true,  H | V },
            { "Phrygian Threat",     "i bII",            true,  V | H },
            { "Harmonic Minor",      "i bVI iv V",       true,  H },
            { "Villain",             "i i bVI V",        true,  V },
            { "Lift",                "bVI V",            true,  P },
            { "Switch Up",           "iv bVI v V",       true,  B },
            { "Melodic Trap",        "vi IV I V",        false, H },
        } });

    // ----------------------------------------------------------------- DRILL
    g.push_back ({ "Drill", 142, 1.0f, 0.0f, 0, Voicing::Close,
        { { 0.5, 0.5 }, { 1.0, 0.5 } },
        { "x---------------", "x-------x-------" },
        { 4.0f, 0.6f, true, 0.3f, 0.2f, 0.35f, 0.4f },
        BassStyle::Kick808,
        { "x---------------" },
        true,
        { "X.....x...x.....", _, "........X.......", "x..x..x.x..x..x.", _, _, _, _ },
        { "X.......x.....x.", _, "........X....X..", "x..x..x.x..x..x.", _, _, _, _ },
        true, Fill::HatRoll,
        {
            { "Sliding Minor",       "i bVI V V",        true,  H | V },
            { "Phrygian Slide",      "i bII",            true,  V },
            { "Block",               "i iv V i",         true,  H },
            { "Road",                "i bVI bVII V",     true,  H | V },
            { "Opps",                "i bVI iv V",       true,  V },
            { "Lift",                "bVI V",            true,  P },
            { "Switch",              "iv bII V V",       true,  B },
        } });

    // ------------------------------------------------------------------- R&B
    g.push_back ({ "R&B", 95, 0.4f, 0.3f, 2, Voicing::Drop2,
        { { 1.0, 0.7 }, { 2.0, 0.3 } },
        { "x---------------", "x-----------p---", "x-----x---x-----", "x--x---x--x-x---" },
        { 4.0f, 0.5f, true, 0.2f, 0.15f, 0.7f, 0.1f },
        BassStyle::Walking,
        { "x---------------", "x-----x---x-----", "x--x---x--x-x---" },
        true,
        { "X......x..X.....", "....X.......X...", _, "xgxgxgxgxgxgxgxg", _, _, _, _ },
        { "X..x......X..x..", "....X..g....X...", _, "xgxgxgxgxgxgxgxg", "..............x.", _, _, _ },
        false, Fill::Toms,
        {
            { "Slow Jam",            "Imaj7 vi7 ii7 V7",      false, V | H },
            { "Late Night",          "IVmaj7 V7 iii7 vi7",    false, H },
            { "Smooth",              "vi7 ii7 V7 Imaj7",      false, V },
            { "Velvet",              "ii7 V7 Imaj7 Imaj7",    false, H },
            { "Climb",               "ii7 iii7 IVmaj7 V7",    false, P },
            { "Borrowed Heart",      "IVmaj7 iv7 iii7 vi7",   false, B },
            { "Moody",               "i7 iv7 bVIImaj7 bIIImaj7", true, V | H },
            { "Lowkey",              "i9 iv9",                true,  V },
            { "After Hours",         "bVImaj7 bVII7 i7 i7",   true,  P | B },
        } });

    // -------------------------------------------------------------- NEO-SOUL
    g.push_back ({ "Neo-Soul", 88, 0.35f, 0.5f, 3, Voicing::Drop2,
        { { 1.0, 0.6 }, { 2.0, 0.4 } },
        { "x---------------", "x-----x---------", "x--x-----x--x---" },
        { 4.0f, 0.6f, true, 0.25f, 0.1f, 0.6f, 0.15f },
        BassStyle::Walking,
        { "x-------x-------", "x-----x-.-x-----", "x--x--x---x--x--" },
        true,
        { "X.....x...X.....", _, _, "x.xgx.xgx.xgx.xg", _, _, "....X.......X...", _ },
        { "X.......x.x..x..", _, _, "x.xgx.xgx.xgx.xg", _, _, "....X..g....X.g.", _ },
        false, Fill::Toms,
        {
            { "Sunday Morning",      "IVmaj9 iii7 ii9 Imaj9",  false, V | H },
            { "Classic Two Five",    "ii9 V13 Imaj9 vi9",      false, H | V },
            { "Float",               "Imaj9 IVmaj9",           false, V },
            { "Dilla Minor",         "i9 iv9",                 true,  V | H },
            { "Rainy",               "i9 bVImaj9 iv9 V7",      true,  H },
            { "Rise Up",             "ii9 iii7 IVmaj9 V13",    false, P },
            { "Gold",                "bVImaj9 bVII13 Imaj9 Imaj9", false, B },
        } });

    // ----------------------------------------------------------------- LO-FI
    g.push_back ({ "Lo-Fi", 80, 0.45f, 0.55f, 2, Voicing::Drop2,
        { { 1.0, 0.7 }, { 2.0, 0.3 } },
        { "x---------------", "x-------------p-", "x-----x---------" },
        { 3.0f, 0.4f, true, 0.2f, 0.15f, 0.6f, 0.0f },
        BassStyle::Walking,
        { "x---------------", "x-------x-------", "x-----x---x-----" },
        true,
        { "X.........X.....", "....X.......X...", _, "x.x.x.x.x.x.x.x.", _, _, _, _ },
        { "X......x..X..x..", "....X.......X..g", _, "x.x.x.x.x.x.x.x.", _, _, _, _ },
        false, Fill::None,
        {
            { "Study Session",       "ii7 V7 Imaj7 vi7",       false, V | H },
            { "Rainy Window",        "IVmaj7 iii7 ii7 Imaj7",  false, H },
            { "Bittersweet",         "Imaj7 iii7 IVmaj7 iv7",  false, V | B },
            { "Night Walk",          "i9 bVImaj7 bIIImaj7 bVII7", true, V | H },
            { "Drift",               "i7 iv7",                 true,  V },
            { "Lean In",             "IVmaj7 V7",              false, P },
            { "Tape Stop",           "bVImaj7 V7 i7 i7",       true,  B },
        } });

    // ---------------------------------------------------------------- GOSPEL
    g.push_back ({ "Gospel", 100, 0.1f, 0.35f, 3, Voicing::Drop2,
        { { 1.0, 0.5 }, { 2.0, 0.5 } },
        { "x---------------", "x-------x-------", "x--x--x---x-x---", "x-xx-x-xx-x-x-x-" },
        { 4.0f, 0.4f, true, 0.25f, 0.15f, 0.7f, 0.1f },
        BassStyle::Walking,
        { "x-------x-------", "x--x--x---x-x---", "x-xx-x-xx-x-x-x-" },
        true,
        { "X.....x...x.....", "....X.......X...", _, "xgxgxgxgxgxgxgxg", _, _, _, _ },
        { "X..x..x...x..x..", "....X..g.g..X.gg", _, "xgxgxgxgxgxgxgxg", "..............x.", _, _, _ },
        false, Fill::Toms,
        {
            { "Praise",              "I iii7 IV V",                 false, V | H },
            { "Shout",               "IV V iii vi",                 false, H },
            { "Church Circle",       "ii7 V7 iii7 vi7 ii7 V7 I I",  false, H | B },
            { "Blessed",             "I I7 IV iv",                  false, H },
            { "Passing Diminished",  "IV #ivdim V vi",              false, V | P },
            { "Testimony",           "vi ii V I",                   false, V },
            { "Climb",               "IV V vi V",                   false, P },
            { "Bridge Climb",        "IV V iii vi ii V I I",        false, B },
            { "Minor Hymn",          "i iv bVII bIII",              true,  V | H },
        } });

    // ------------------------------------------------------------- AFROBEATS
    g.push_back ({ "Afrobeats", 106, 0.3f, 0.3f, 1, Voicing::Close,
        { { 1.0, 0.8 }, { 2.0, 0.2 } },
        { "x---------------", "x..x..x...x.x...", "..x..x....x..x.." },
        { 5.0f, 0.6f, true, 0.2f, 0.3f, 0.4f, 0.0f },
        BassStyle::Pattern,
        { "x..x..x...x.....", "x.....x.x...x...", "x..x..x.x..x..x." },
        true,
        { "X......x..X.....", _, _, "x.x.x.x.x.x.x.x.", _, "xgxgxgxgxgxgxgxg", "...x..x....x..x.", "......x.....x.x." },
        { "X..x...x..X.....", _, _, "x.x.x.x.x.x.x.x.", "..............x.", "xgxgxgxgxgxgxgxg", "...x..x....x.x..", "......x.....x.x." },
        false, Fill::Toms,
        {
            { "Lagos Nights",        "vi IV I V",        false, H | V },
            { "Sweet Vamp",          "ii I",             false, V },
            { "Sunshine",            "I vi ii V",        false, H },
            { "Groove",              "IV V iii vi",      false, V | H },
            { "Two Step",            "I IV",             false, V },
            { "Hold Me",             "IV V",             false, P },
            { "Climb",               "ii iii IV V",      false, B | P },
            { "Midnight Afro",       "i bVII bVI bVII",  true,  H | V },
        } });

    // ------------------------------------------------------------- REGGAETON
    g.push_back ({ "Reggaeton", 94, 0.7f, 0.0f, 1, Voicing::Close,
        { { 1.0, 1.0 } },
        { "x---------------", "x--x--x-x--x--x-", "...x..x....x..x." },
        { 5.0f, 0.6f, true, 0.2f, 0.3f, 0.4f, 0.0f },
        BassStyle::Pattern,
        { "x---x---x---x---", "x--x--x-x--x--x-" },
        true,
        { "X...X...X...X...", "...x..x....x..x.", _, "x.x.x.x.x.x.x.x.", _, _, _, _ },
        { "X...X...X...X...", "...x..x....x.xx.", _, "x.x.x.x.x.x.x.x.", "..............x.", _, _, _ },
        false, Fill::SnareRoll,
        {
            { "Perreo",              "i bVI bIII bVII",  true,  H | V },
            { "Callejero",           "i bVII bVI bVII",  true,  H },
            { "Noche",               "i iv bVII bIII",   true,  V },
            { "Fuego",               "i bVI bVII bVII",  true,  V | P },
            { "Subida",              "bVI bVII",         true,  P },
            { "Puente",              "iv bVI V V",       true,  B },
            { "Romantico",           "vi IV I V",        false, H | V },
        } });

    return g;
}
} // namespace

const std::vector<Genre>& genres()
{
    static const std::vector<Genre> g = build();
    return g;
}
} // namespace hs::detail
