#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <cmath>

// -----------------------------------------------------------------------------
// Built-in "sketch" sounds so HOOKSMITH makes noise straight away, before you
// load your own instruments: pianos, pads, strings, plucks, organ, leads,
// basses and a synthesized drum kit on the General MIDI / Drum Rack notes.
// Allocation-free; everything runs on the audio thread.
// -----------------------------------------------------------------------------
namespace hs
{
class SketchSynth
{
public:
    enum Type { Keys = 0, Lead, Bass, Drums };

    // Built-in sounds. Each part picks one (or Auto = the genre's sound).
    enum Patch { EPiano = 0, Piano, Pad, Strings, Pluck, Organ, SuperSaw, LoFiKeys,
                 FMBell, SawLead, SquareLead, Flute, Bass808, SubBass, Reese, PluckBass, SawBass, Kit, kNumPatches };

    static const char* patchName (int p)
    {
        static const char* n[] = { "ELECTRIC PIANO", "GRAND PIANO", "WARM PAD", "STRINGS", "PLUCK", "ORGAN", "SUPER SAW", "LO-FI KEYS",
                                   "FM BELL", "SAW LEAD", "SQUARE LEAD", "FLUTE", "808", "SUB BASS", "REESE", "PLUCK BASS", "SAW BASS", "DRUM KIT" };
        return juce::isPositiveAndBelow (p, (int) kNumPatches) ? n[p] : "";
    }

    void prepare (double sampleRate, Type t)
    {
        sr = sampleRate;
        type = t;
        patch = t == Keys ? EPiano : t == Lead ? FMBell : t == Bass ? Bass808 : Kit;
        for (auto& v : voices) v = Voice {};
    }

    // Switch sound; notes already ringing finish with the old one.
    void setPatch (int p)
    {
        if (type == Drums || ! juce::isPositiveAndBelow (p, (int) kNumPatches) || p == patch) return;
        patch = p;
        for (auto& v : voices) if (v.active) v.released = true;
    }
    int getPatch() const noexcept { return patch; }

    void allNotesOff()
    {
        for (auto& v : voices)
            if (v.active) v.released = true;
    }

    // Renders the block (adds into the buffers) and consumes the MIDI.
    void render (const juce::MidiBuffer& midi, float* L, float* R, int numSamples, float gain)
    {
        int pos = 0;
        for (const auto meta : midi)
        {
            const int at = juce::jlimit (0, numSamples, meta.samplePosition);
            renderSpan (L + pos, R + pos, at - pos, gain);
            pos = at;
            const auto m = meta.getMessage();
            if (m.isNoteOn())                        noteOn (m.getNoteNumber(), m.getFloatVelocity());
            else if (m.isNoteOff())                  noteOff (m.getNoteNumber());
            else if (m.isAllNotesOff() || m.isAllSoundOff()) allNotesOff();
        }
        renderSpan (L + pos, R + pos, numSamples - pos, gain);
    }

private:
    struct Voice
    {
        bool active = false, released = false;
        int note = 0, patch = 0;
        float vel = 0, env = 0, pan = 0.5f;
        double phase = 0, phase2 = 0, freq = 440, targetFreq = 440, t = 0;
        std::array<double, 5> uni {};          // unison oscillators
        float lp = 0, hp = 0, hpIn = 0, bp1 = 0, bp2 = 0;
        float ic1 = 0, ic2 = 0, ic3 = 0, ic4 = 0;   // filter state (two SVFs)
        juce::uint32 noise = 22222;
        int drum = 0;
    };

    static float midiHz (double n) { return (float) (440.0 * std::pow (2.0, (n - 69.0) / 12.0)); }

    Voice& freeVoice()
    {
        Voice* best = &voices[0];
        for (auto& v : voices)
        {
            if (! v.active) return v;
            if (v.released && (! best->released || v.env < best->env)) best = &v;
            else if (! best->released && v.env < best->env) best = &v;
        }
        return *best;
    }

    bool monoPatch() const { return patch == Bass808 || patch == Reese || patch == SawBass || patch == SubBass; }

    void noteOn (int note, float vel)
    {
        if (type == Bass && monoPatch())
        {
            // Mono with glide, like an 808
            for (auto& v : voices)
                if (v.active && ! v.released && v.patch == patch)
                {
                    v.note = note; v.vel = vel; v.targetFreq = midiHz (note);
                    if (patch == Bass808) v.t = 0.0;
                    return;
                }
        }
        if (type == Drums)
        {
            // Hats choke each other
            if (note == 42 || note == 46 || note == 44)
                for (auto& v : voices) if (v.active && (v.drum == 42 || v.drum == 46 || v.drum == 44)) v.released = true;
        }
        if (type != Drums)
            for (auto& v : voices) if (v.active && v.note == note && ! v.released) v.released = true;

        auto& v = freeVoice();
        v = Voice {};
        v.active = true;
        v.note = note;
        v.drum = note;
        v.patch = patch;
        v.vel = vel;
        v.freq = v.targetFreq = midiHz (note);
        v.pan = type == Keys ? 0.35f + 0.3f * (float) ((note * 7) % 12) / 11.0f : 0.5f;
        v.noise = (juce::uint32) note * 2654435761u + 12345u;
        v.env = type == Drums ? 1.0f : 0.0f;
        for (size_t k = 0; k < v.uni.size(); ++k) v.uni[k] = (double) ((v.noise >> (k * 5)) & 1023) / 1023.0;   // free-running phases
        if (patch == Bass808) v.freq = midiHz (note + 12);   // punchy pitch drop
    }

    void noteOff (int note)
    {
        if (type == Drums) return;   // drums play out their own decay
        for (auto& v : voices) if (v.active && v.note == note) v.released = true;
    }

    float nextNoise (Voice& v)
    {
        v.noise = v.noise * 1664525u + 1013904223u;
        return (float) (v.noise >> 8) / 8388608.0f - 1.0f;
    }

    void renderSpan (float* L, float* R, int n, float gain)
    {
        if (n <= 0) return;
        const float dt = (float) (1.0 / sr);
        for (auto& v : voices)
        {
            if (! v.active) continue;
            for (int i = 0; i < n; ++i)
            {
                const float s = (type == Drums ? drum (v, dt) : voice (v, dt)) * gain;
                v.t += dt;
                L[i] += s * std::sqrt (1.0f - v.pan) * 1.2f;
                R[i] += s * std::sqrt (v.pan) * 1.2f;
                if (! v.active) break;
            }
        }
    }

    static float adsr (Voice& v, float dt, float a, float d, float s, float r)
    {
        if (v.released)
        {
            v.env -= v.env * dt / r + dt * 0.002f;
            if (v.env <= 0.0005f) { v.env = 0; v.active = false; }
        }
        else if (v.t < a) v.env = (float) (v.t / a);
        else              v.env = s + (v.env - s) * std::exp (-dt / d);
        return v.env;
    }

    // ---- oscillator / filter helpers ------------------------------------------
    static double polyBlep (double t, double dt)
    {
        if (t < dt) { t /= dt; return t + t - t * t - 1.0; }
        if (t > 1.0 - dt) { t = (t - 1.0) / dt; return t * t + t + t + 1.0; }
        return 0.0;
    }
    static float saw (double& ph, double inc)
    {
        ph += inc; if (ph >= 1.0) ph -= 1.0;
        return (float) (2.0 * ph - 1.0 - polyBlep (ph, inc));
    }
    static float square (double& ph, double inc, double width = 0.5)
    {
        ph += inc; if (ph >= 1.0) ph -= 1.0;
        double s = ph < width ? 1.0 : -1.0;
        s += polyBlep (ph, inc);
        double t2 = ph + (1.0 - width); if (t2 >= 1.0) t2 -= 1.0;
        s -= polyBlep (t2, inc);
        return (float) s;
    }
    // TPT state-variable low-pass (2-pole), cutoff in Hz
    float svfLow (float x, float cutoff, float res, float& ic1, float& ic2) const
    {
        cutoff = juce::jlimit (20.0f, (float) (sr * 0.45), cutoff);
        const float g = std::tan (juce::MathConstants<float>::pi * cutoff / (float) sr);
        const float k = 2.0f - 2.0f * res;
        const float a1 = 1.0f / (1.0f + g * (g + k)), a2 = g * a1, a3 = g * a2;
        const float v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
        ic1 = 2.0f * v1 - ic1; ic2 = 2.0f * v2 - ic2;
        return v2;
    }
    static float vibrato (const Voice& v, float depth, float rate = 5.2f, float delay = 0.25f)
    {
        const float amt = juce::jlimit (0.0f, 1.0f, ((float) v.t - delay) * 2.0f) * depth;
        return 1.0f + amt * std::sin (juce::MathConstants<float>::twoPi * rate * (float) v.t);
    }

    float voice (Voice& v, float dt)
    {
        const double inc = v.freq * dt;
        const float t = (float) v.t;
        switch (v.patch)
        {
            case EPiano:
            {
                const float e = adsr (v, dt, 0.004f, 1.6f, 0.35f, 0.35f);
                const double w = juce::MathConstants<double>::twoPi * inc;
                v.phase += w; v.phase2 += w * 1.0035;
                const float bright = std::exp (-t * 2.5f);
                const float s = (float) (std::sin (v.phase) + 0.5 * std::sin (v.phase2 * 2.0) * bright
                                         + 0.15 * std::sin (v.phase * 3.0) * bright * bright);
                return s * e * v.vel * 0.12f;
            }
            case LoFiKeys:
            {
                const float e = adsr (v, dt, 0.006f, 1.2f, 0.3f, 0.3f);
                const double w = juce::MathConstants<double>::twoPi * inc * (1.0 + 0.004 * std::sin (t * 4.1));
                v.phase += w; v.phase2 += w * 4.0;
                const float tine = std::exp (-t * 6.0f);
                float s = (float) (std::sin (v.phase + 0.8 * tine * std::sin (v.phase2)));
                s = svfLow (s, 1800.0f, 0.1f, v.ic1, v.ic2);
                return s * e * v.vel * 0.15f;
            }
            case Piano:
            {
                const float e = adsr (v, dt, 0.002f, 3.5f, 0.0f, 0.25f);
                const double w = juce::MathConstants<double>::twoPi * inc;
                v.phase += w;
                const float b = 0.0004f;   // string stiffness: slightly sharp upper partials
                float s = 0.0f;
                for (int h = 1; h <= 6; ++h)
                {
                    const float stretch = std::sqrt (1.0f + b * h * h);
                    s += std::sin ((float) (v.phase * h * stretch)) * std::exp (-t * (0.8f + 1.4f * h)) / (float) h;
                }
                const float hammer = nextNoise (v) * std::exp (-t * 120.0f) * 0.15f;
                return (s + hammer) * e * v.vel * 0.16f;
            }
            case Pad:
            case Strings:
            case SuperSaw:
            {
                const bool pad = v.patch == Pad, str = v.patch == Strings;
                const float e = pad ? adsr (v, dt, 0.45f, 2.0f, 0.85f, 0.9f)
                              : str ? adsr (v, dt, 0.12f, 1.5f, 0.8f, 0.45f)
                                    : adsr (v, dt, 0.01f, 0.8f, 0.7f, 0.25f);
                static const double det[5] = { -0.011, -0.0045, 0.0, 0.0047, 0.0102 };
                const double vib = str ? vibrato (v, 0.004f) : 1.0;
                float s = 0.0f;
                for (int k = 0; k < 5; ++k) s += saw (v.uni[(size_t) k], inc * vib * (1.0 + det[k] * (pad ? 0.8 : 1.0)));
                s *= 0.3f;
                const float cut = pad ? 900.0f + 700.0f * std::sin (t * 0.7f) * 0.5f + 600.0f * v.vel
                                : str ? 2600.0f + 1500.0f * v.vel
                                      : 3500.0f + 4000.0f * std::exp (-t * 3.0f);
                s = svfLow (s, cut, 0.2f, v.ic1, v.ic2);
                return s * e * v.vel * (pad ? 0.16f : str ? 0.15f : 0.18f);
            }
            case Pluck:
            case PluckBass:
            {
                const bool bass = v.patch == PluckBass;
                const float e = adsr (v, dt, 0.002f, bass ? 0.35f : 0.5f, 0.0f, 0.12f);
                float s = bass ? square (v.phase, inc, 0.5) * 0.6f + saw (v.phase2, inc) * 0.5f
                               : saw (v.phase, inc) * 0.6f + saw (v.phase2, inc * 1.006) * 0.5f;
                const float cut = (bass ? 180.0f : 500.0f) + (bass ? 2200.0f : 6000.0f) * std::exp (-t * (bass ? 14.0f : 11.0f));
                s = svfLow (s, cut, 0.35f, v.ic1, v.ic2);
                return s * e * v.vel * (bass ? 0.5f : 0.24f);
            }
            case Organ:
            {
                const float e = adsr (v, dt, 0.008f, 0.3f, 1.0f, 0.08f);
                const double w = juce::MathConstants<double>::twoPi * inc * (1.0 + 0.0025 * std::sin (t * 39.0));
                v.phase += w;
                const float click = std::exp (-t * 200.0f) * 0.2f;
                const float s = (float) (std::sin (v.phase * 0.5) * 0.6 + std::sin (v.phase) * 0.8 + std::sin (v.phase * 1.5) * 0.4
                                         + std::sin (v.phase * 2.0) * 0.5 + std::sin (v.phase * 4.0) * 0.25) + click * nextNoise (v);
                return s * e * v.vel * 0.07f;
            }
            case FMBell:
            {
                const float e = adsr (v, dt, 0.002f, 0.7f, 0.22f, 0.25f);
                const double w = juce::MathConstants<double>::twoPi * inc;
                v.phase2 += w * 3.5;
                const float index = 2.2f * std::exp (-t * 4.0f) + 0.25f;
                v.phase += w;
                return (float) std::sin (v.phase + index * std::sin (v.phase2)) * e * v.vel * 0.14f;
            }
            case SawLead:
            {
                const float e = adsr (v, dt, 0.005f, 0.6f, 0.75f, 0.18f);
                const double vib = vibrato (v, 0.006f);
                float s = saw (v.phase, inc * vib) * 0.55f + saw (v.phase2, inc * vib * 1.008) * 0.45f;
                s = svfLow (s, 1600.0f + 3500.0f * std::exp (-t * 5.0f) + 1500.0f * v.vel, 0.3f, v.ic1, v.ic2);
                return s * e * v.vel * 0.2f;
            }
            case SquareLead:
            {
                const float e = adsr (v, dt, 0.004f, 0.5f, 0.7f, 0.15f);
                const double vib = vibrato (v, 0.007f);
                float s = square (v.phase, inc * vib, 0.42 + 0.06 * std::sin (t * 2.0));
                s = svfLow (s, 2400.0f + 2000.0f * std::exp (-t * 6.0f), 0.25f, v.ic1, v.ic2);
                return s * e * v.vel * 0.12f;
            }
            case Flute:
            {
                const float e = adsr (v, dt, 0.06f, 0.8f, 0.8f, 0.2f);
                const double w = juce::MathConstants<double>::twoPi * inc * vibrato (v, 0.006f, 5.0f, 0.3f);
                v.phase += w;
                float breath = svfLow (nextNoise (v), 2500.0f, 0.5f, v.ic1, v.ic2) * (0.25f + 0.5f * std::exp (-t * 12.0f));
                const float s = (float) (std::sin (v.phase) + 0.18 * std::sin (v.phase * 2.0) + 0.06 * std::sin (v.phase * 3.0)) + breath * 0.5f;
                return s * e * v.vel * 0.15f;
            }
            case Bass808:
            {
                const float e = adsr (v, dt, 0.003f, 2.5f, 0.0f, 0.06f);
                v.freq += (v.targetFreq - v.freq) * (1.0 - std::exp (-dt / 0.025));   // glide / punch
                v.phase += juce::MathConstants<double>::twoPi * v.freq * dt;
                return std::tanh ((float) std::sin (v.phase) * 2.2f) * e * v.vel * 0.32f;
            }
            case SubBass:
            {
                const float e = adsr (v, dt, 0.006f, 1.0f, 0.85f, 0.08f);
                v.freq += (v.targetFreq - v.freq) * (1.0 - std::exp (-dt / 0.012));
                v.phase += juce::MathConstants<double>::twoPi * v.freq * dt;
                const float s = (float) (std::sin (v.phase) + 0.12 * std::sin (v.phase * 2.0));
                return s * e * v.vel * 0.3f;
            }
            case Reese:
            {
                const float e = adsr (v, dt, 0.01f, 1.0f, 0.9f, 0.1f);
                v.freq += (v.targetFreq - v.freq) * (1.0 - std::exp (-dt / 0.03));
                const double i2 = v.freq * dt;
                float s = saw (v.phase, i2 * 0.996) * 0.5f + saw (v.phase2, i2 * 1.004) * 0.5f + saw (v.uni[0], i2 * 0.5) * 0.35f;
                s = svfLow (s, 420.0f + 250.0f * std::sin (t * 1.3f), 0.25f, v.ic1, v.ic2);
                return std::tanh (s * 2.0f) * e * v.vel * 0.22f;
            }
            case SawBass:
            {
                const float e = adsr (v, dt, 0.003f, 0.6f, 0.6f, 0.08f);
                v.freq += (v.targetFreq - v.freq) * (1.0 - std::exp (-dt / 0.02));
                float s = saw (v.phase, v.freq * dt) * 0.7f + square (v.phase2, v.freq * dt * 0.5) * 0.4f;
                s = svfLow (s, 300.0f + 1600.0f * std::exp (-t * 9.0f), 0.3f, v.ic1, v.ic2);
                return s * e * v.vel * 0.26f;
            }
            default: return 0.0f;
        }
    }

    float drum (Voice& v, float dt)
    {
        const float t = (float) v.t;
        float s = 0.0f, decay = 0.2f;
        auto hpNoise = [&] (float cut)
        {
            const float x = nextNoise (v);
            const float a = std::exp (-juce::MathConstants<float>::twoPi * cut * dt);
            v.hp = a * (v.hp + x - v.hpIn);
            v.hpIn = x;
            return v.hp;
        };

        switch (v.drum)
        {
            case 36: case 35:   // kick
            {
                const double f = 45.0 + 110.0 * std::exp (-t * 30.0);
                v.phase += juce::MathConstants<double>::twoPi * f * dt;
                s = std::tanh ((float) std::sin (v.phase) * 1.8f) * std::exp (-t * 7.0f)
                  + 0.3f * std::exp (-t * 300.0f) * nextNoise (v);
                decay = 0.6f;
                break;
            }
            case 38: case 40:   // snare
            {
                v.phase += juce::MathConstants<double>::twoPi * (185.0 + 40.0 * std::exp (-t * 40.0)) * dt;
                s = 0.5f * (float) std::sin (v.phase) * std::exp (-t * 22.0f) + 0.7f * hpNoise (1800.0f) * std::exp (-t * 16.0f);
                decay = 0.35f;
                break;
            }
            case 39:            // clap: three quick bursts, then a tail
            {
                const float burst = (t < 0.01f || (t > 0.012f && t < 0.02f) || (t > 0.024f && t < 0.032f)) ? 1.0f : 0.0f;
                const float env = t < 0.032f ? burst : std::exp (-(t - 0.032f) * 14.0f);
                s = hpNoise (1200.0f) * env * 0.9f;
                decay = 0.4f;
                break;
            }
            case 37:            // rim
                v.phase += juce::MathConstants<double>::twoPi * 1700.0 * dt;
                s = (float) std::sin (v.phase) * std::exp (-t * 90.0f) * 0.6f + hpNoise (3000.0f) * std::exp (-t * 120.0f) * 0.4f;
                decay = 0.1f;
                break;
            case 42: case 44:   // closed hat
                s = hpNoise (7000.0f) * std::exp (-t * 55.0f) * 0.55f;
                decay = 0.12f;
                break;
            case 46:            // open hat
                s = hpNoise (6500.0f) * std::exp (-t * 7.0f) * 0.45f;
                decay = 0.8f;
                break;
            case 49: case 57:   // crash
                s = hpNoise (4500.0f) * std::exp (-t * 2.2f) * 0.4f;
                decay = 2.5f;
                break;
            case 51:            // ride
                v.phase += juce::MathConstants<double>::twoPi * 3200.0 * dt;
                s = hpNoise (6000.0f) * std::exp (-t * 4.0f) * 0.25f + (float) std::sin (v.phase) * std::exp (-t * 5.0f) * 0.08f;
                decay = 1.5f;
                break;
            case 70: case 82:   // shaker
            {
                const float env = t < 0.01f ? t / 0.01f : std::exp (-(t - 0.01f) * 40.0f);
                s = hpNoise (5000.0f) * env * 0.35f;
                decay = 0.15f;
                break;
            }
            default:            // toms (41, 43, 45, 47, 48, 50) and anything else
            {
                const double base = 70.0 + (v.drum - 41) * 16.0;
                v.phase += juce::MathConstants<double>::twoPi * (base + base * 0.6 * std::exp (-t * 25.0)) * dt;
                s = std::tanh ((float) std::sin (v.phase) * 1.5f) * std::exp (-t * 8.0f) * 0.8f;
                decay = 0.6f;
                break;
            }
        }
        if (t > decay * 4.0f) v.active = false;
        return s * (0.25f + 0.75f * v.vel) * 0.5f;
    }

    double sr = 44100.0;
    Type type = Keys;
    int patch = EPiano;
    std::array<Voice, 24> voices {};
};
} // namespace hs
