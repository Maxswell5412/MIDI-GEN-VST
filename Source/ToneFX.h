#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>
#include <array>

// -----------------------------------------------------------------------------
// MIDI GEN tone section, after the instrument (built-in or your plugin):
//   HIGH PASS / LOW PASS filters (12 dB/oct, a touch of resonance)
//   MULTIBAND: 3 bands (crossovers 200 Hz and 2.5 kHz, Linkwitz-Riley so they sum flat),
//              a gain per band and SQUASH, a 3-band compressor with make-up gain.
// All settings glide so turning a knob never clicks.
// -----------------------------------------------------------------------------
namespace mg
{
struct ToneSettings
{
    float hp = 0.0f;       // 0 = off .. 1 = 2 kHz
    float lp = 1.0f;       // 1 = off .. 0 = 200 Hz
    float res = 0.2f;      // filter resonance 0..1
    float low = 0.0f, mid = 0.0f, high = 0.0f;   // band gains in dB (-12 .. +12)
    float squash = 0.0f;   // multiband compression 0..1
};

class ToneFX
{
public:
    void prepare (double sampleRate)
    {
        sr = sampleRate;
        reset();
    }

    void reset()
    {
        for (auto& c : ch) c = Channel {};
        for (auto& e : env) e = 0.0f;
        sm = {};
        first = true;
    }

    // hz for the knobs (for the display)
    static float hpHz (float k) { return 20.0f * std::pow (100.0f, k); }           // 20 Hz .. 2 kHz
    static float lpHz (float k) { return 200.0f * std::pow (100.0f, k); }          // 200 Hz .. 20 kHz

    void process (float* L, float* R, int n, const ToneSettings& target)
    {
        if (n <= 0) return;
        if (first) { sm = target; first = false; }
        const bool mbOn = std::abs (target.low) > 0.05f || std::abs (target.mid) > 0.05f || std::abs (target.high) > 0.05f
                       || target.squash > 0.005f || std::abs (sm.low) > 0.05f || std::abs (sm.mid) > 0.05f
                       || std::abs (sm.high) > 0.05f || sm.squash > 0.005f;
        const float glide = 1.0f - std::exp (-1.0f / (0.03f * (float) sr));   // ~30 ms

        // filter coefficients once per block (the smoothed value is close enough per 64 samples)
        for (int i = 0; i < n; ++i)
        {
            sm.hp += (target.hp - sm.hp) * glide;  sm.lp += (target.lp - sm.lp) * glide;
            sm.res += (target.res - sm.res) * glide;
            sm.low += (target.low - sm.low) * glide; sm.mid += (target.mid - sm.mid) * glide;
            sm.high += (target.high - sm.high) * glide; sm.squash += (target.squash - sm.squash) * glide;
            if ((i & 15) == 0) updateCoeffs();

            float x[2] = { L[i], R != nullptr ? R[i] : L[i] };
            float bands[2][3] {};
            for (int c = 0; c < 2; ++c)
            {
                auto& s = ch[(size_t) c];
                float v = x[c];
                if (sm.hp > 0.002f) v = s.hp.highpass (v, hpC);
                if (sm.lp < 0.998f) v = s.lp.lowpass (v, lpC);
                x[c] = v;
                if (mbOn)
                {
                    // Linkwitz-Riley 4: two Butterworth stages per side
                    const float lo  = s.l2.lowpass (s.l1.lowpass (v, x1), x1);
                    const float hi1 = s.h2.highpass (s.h1.highpass (v, x1), x1);
                    const float md  = s.m2.lowpass (s.m1.lowpass (hi1, x2), x2);
                    const float hi  = s.g2.highpass (s.g1.highpass (hi1, x2), x2);
                    // the low band gets the same phase turn the 2.5 kHz split gave the others
                    const float loAp = s.a2.lowpass (s.a1.lowpass (lo, x2), x2) + s.b2.highpass (s.b1.highpass (lo, x2), x2);
                    bands[c][0] = loAp; bands[c][1] = md; bands[c][2] = hi;
                }
            }
            if (mbOn)
            {
                const float gains[3] = { dbToGain (sm.low), dbToGain (sm.mid), dbToGain (sm.high) };
                for (int b = 0; b < 3; ++b)
                {
                    float g = gains[b];
                    if (sm.squash > 0.005f)
                    {
                        // linked stereo compressor per band: threshold falls and ratio rises with SQUASH
                        const float level = std::max (std::abs (bands[0][b]), std::abs (bands[1][b]));
                        const float coef = level > env[(size_t) b] ? atk[(size_t) b] : rel[(size_t) b];
                        env[(size_t) b] += (level - env[(size_t) b]) * coef;
                        const float lvlDb = 20.0f * std::log10 (env[(size_t) b] + 1.0e-6f);
                        const float thr = -8.0f - 28.0f * sm.squash;
                        const float ratio = 1.0f + 7.0f * sm.squash;
                        const float over = std::max (0.0f, lvlDb - thr);
                        const float reduce = over - over / ratio;
                        const float makeup = 0.45f * (-thr - (-thr) / ratio);
                        g *= dbToGain (makeup - reduce);
                    }
                    bands[0][b] *= g; bands[1][b] *= g;
                }
                x[0] = bands[0][0] + bands[0][1] + bands[0][2];
                x[1] = bands[1][0] + bands[1][1] + bands[1][2];
                if (sm.squash > 0.005f)   // keep it out of the red
                    for (auto& v : x) v = std::tanh (v * 0.9f) / 0.9f;
            }
            L[i] = x[0];
            if (R != nullptr) R[i] = x[1];
        }
    }

private:
    struct Coeff { float g = 0.1f, k = 1.414f, a1 = 0, a2 = 0, a3 = 0; };
    struct SVF
    {
        float ic1 = 0, ic2 = 0;
        float tick (float v0, const Coeff& c, float& lp, float& bp)
        {
            const float v3 = v0 - ic2;
            const float v1 = c.a1 * ic1 + c.a2 * v3;
            const float v2 = ic2 + c.a2 * ic1 + c.a3 * v3;
            ic1 = 2.0f * v1 - ic1; ic2 = 2.0f * v2 - ic2;
            lp = v2; bp = v1;
            return v0;
        }
        float lowpass (float v, const Coeff& c)  { float lp, bp; tick (v, c, lp, bp); return lp; }
        float highpass (float v, const Coeff& c) { float lp, bp; tick (v, c, lp, bp); return v - c.k * bp - lp; }
    };
    struct Channel { SVF hp, lp, l1, l2, h1, h2, m1, m2, g1, g2, a1, a2, b1, b2; };

    static Coeff make (double sr, double hz, double q)
    {
        Coeff c;
        hz = std::min (hz, sr * 0.45);
        c.g = (float) std::tan (juce::MathConstants<double>::pi * hz / sr);
        c.k = (float) (1.0 / q);
        c.a1 = 1.0f / (1.0f + c.g * (c.g + c.k));
        c.a2 = c.g * c.a1;
        c.a3 = c.g * c.a2;
        return c;
    }
    static float dbToGain (float db) { return std::pow (10.0f, db / 20.0f); }

    void updateCoeffs()
    {
        const double q = 0.707 + 2.2 * sm.res;
        hpC = make (sr, hpHz (sm.hp), q);
        lpC = make (sr, lpHz (sm.lp), q);
        if (! xReady) { x1 = make (sr, 200.0, 0.7071); x2 = make (sr, 2500.0, 0.7071); xReady = true; }
        for (int b = 0; b < 3; ++b)
        {
            static const float atkMs[] = { 18.0f, 8.0f, 3.0f }, relMs[] = { 180.0f, 120.0f, 80.0f };
            atk[(size_t) b] = 1.0f - std::exp (-1.0f / (0.001f * atkMs[b] * (float) sr));
            rel[(size_t) b] = 1.0f - std::exp (-1.0f / (0.001f * relMs[b] * (float) sr));
        }
    }

    double sr = 44100.0;
    std::array<Channel, 2> ch {};
    Coeff hpC, lpC, x1, x2;
    bool xReady = false, first = true;
    std::array<float, 3> env {}, atk {}, rel {};
    ToneSettings sm;
};
} // namespace mg
