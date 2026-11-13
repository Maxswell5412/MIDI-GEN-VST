#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <array>
#include <cmath>

// -----------------------------------------------------------------------------
// Built-in "sketch" sounds so HOOKSMITH makes noise straight away, before you
// load your own instruments: electric piano (chords), FM bell (melody),
// 808-style bass and a synthesized drum kit on the General MIDI / Drum Rack notes.
// Allocation-free; everything runs on the audio thread.
// -----------------------------------------------------------------------------
namespace hs
{
class SketchSynth
{
public:
    enum Type { Keys = 0, Lead, Bass, Drums };

    void prepare (double sampleRate, Type t)
    {
        sr = sampleRate;
        type = t;
        for (auto& v : voices) v = Voice {};
    }

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
        int note = 0;
        float vel = 0, env = 0, pan = 0.5f;
        double phase = 0, phase2 = 0, freq = 440, targetFreq = 440, t = 0;
        float lp = 0, hp = 0, hpIn = 0, bp1 = 0, bp2 = 0;
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

    void noteOn (int note, float vel)
    {
        if (type == Bass)
        {
            // Mono with glide, like an 808
            for (auto& v : voices)
                if (v.active && ! v.released)
                {
                    v.note = note; v.vel = vel; v.targetFreq = midiHz (note); v.t = 0.0;
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
        v.vel = vel;
        v.freq = v.targetFreq = midiHz (note);
        v.pan = type == Keys ? 0.35f + 0.3f * (float) ((note * 7) % 12) / 11.0f : 0.5f;
        v.noise = (juce::uint32) note * 2654435761u + 12345u;
        v.env = type == Drums ? 1.0f : 0.0f;
        if (type == Bass) v.freq = midiHz (note + 12);   // punchy pitch drop
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
                float s = 0.0f;
                switch (type)
                {
                    case Keys:  s = keys (v, dt); break;
                    case Lead:  s = lead (v, dt); break;
                    case Bass:  s = bass (v, dt); break;
                    case Drums: s = drum (v, dt); break;
                }
                v.t += dt;
                s *= gain;
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

    float keys (Voice& v, float dt)
    {
        const float e = adsr (v, dt, 0.004f, 1.6f, 0.35f, 0.35f);
        const double w = juce::MathConstants<double>::twoPi * v.freq * dt;
        v.phase += w; v.phase2 += w * 1.0035;
        const float bright = std::exp (-(float) v.t * 2.5f);
        const float s = (float) (std::sin (v.phase) + 0.5 * std::sin (v.phase2 * 2.0) * bright
                                 + 0.15 * std::sin (v.phase * 3.0) * bright * bright);
        return s * e * v.vel * 0.12f;
    }

    float lead (Voice& v, float dt)
    {
        const float e = adsr (v, dt, 0.002f, 0.7f, 0.22f, 0.25f);
        const double w = juce::MathConstants<double>::twoPi * v.freq * dt;
        v.phase2 += w * 3.5;
        const float index = 2.2f * std::exp (-(float) v.t * 4.0f) + 0.25f;
        v.phase += w;
        const float s = (float) std::sin (v.phase + index * std::sin (v.phase2));
        return s * e * v.vel * 0.14f;
    }

    float bass (Voice& v, float dt)
    {
        const float e = adsr (v, dt, 0.003f, 2.5f, 0.0f, 0.06f);
        v.freq += (v.targetFreq - v.freq) * (1.0 - std::exp (-dt / 0.025));   // glide / punch
        v.phase += juce::MathConstants<double>::twoPi * v.freq * dt;
        const float s = std::tanh ((float) std::sin (v.phase) * 2.2f);
        return s * e * v.vel * 0.32f;
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
    std::array<Voice, 24> voices {};
};
} // namespace hs
