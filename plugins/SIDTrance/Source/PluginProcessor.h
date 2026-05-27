#pragma once

#include <JuceHeader.h>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

// ============================================================
//  SIDOscillator — 6581-inspired phase-accumulator oscillator
// ============================================================
struct SIDOscillator {
    // HYPERSAW adds 7 bandlimited saws with variable detune spread (Roland JP-8000 style)
    enum class Wave : int { SAW = 0, TRI, PULSE, NOISE, SAW_TRI, RING, HYPERSAW };

    Wave     wave      = Wave::SAW;
    float    phase     = 0.0f;
    float    phaseInc  = 0.0f;
    float    pw        = 0.5f;   // pulse width 0..1
    int      semi      = 0;      // -24..+24
    float    fine      = 0.0f;   // -100..+100 cents
    float    volume    = 1.0f;

    // TRANCE DRIFT
    float    drift     = 0.0f;   // amount 0..1
    float    driftPhs  = 0.0f;
    float    driftVal  = 0.0f;
    float    driftInc  = 0.0f;
    uint32_t driftLfsr = 0xACE1u; // per-osc LFSR for drift — thread-safe (no rand())

    // DIGITAL AGE (bitcrusher)
    float    age       = 0.0f;   // amount 0..1
    float    qCache    = 0.0f;   // precomputed 2^bits — updated once per note-on via updateAge()

    // Dual-engine chip blend.  0.0 = 6581 (no polyBLEP correction → aliased,
    // raw "old/dirty" character).  1.0 = 8580 (full polyBLEP correction →
    // bandlimited, "new/clean" character).  Intermediate values are linear
    // mixes of the same aliased waveform plus a scaled alias-suppression term,
    // which is the correct crossfade for two phase-coherent engines (constant-
    // power would over-boost in the middle because the signals are correlated).
    // Updated per-sample from the processor's smoothed chip_model parameter.
    float    chipBlend = 1.0f;

    // Ring mod: feed from other osc
    float    ringIn    = 0.0f;

    // OSC Sync: set true when this osc's phase wraps — used by synced osc to reset
    bool     syncTick  = false;

    // HYPERSAW: 6 additional phase accumulators for the 7-voice supersaw.
    // Voice 0 uses the primary `phase` above; voices 1-6 use hypPhase[0..5].
    // Detune is computed once in setFrequency() via hypInc[].
    float    hypPhase[6] = {};
    float    hypInc[6]   = {};

    // SUPERSAW mode — independent of the Wave selector.  When superOn is true
    // the oscillator renders (1 + sides) detuned saws regardless of the wave
    // choice, using the JP-8000 mix curve and 1/√N gain compensation so the
    // loudness stays constant as voice count or detune changes.  Up to 10 side
    // voices (so totals of 7, 9, or 11 fit), each with its own phase
    // accumulator that's randomised on note-on for a wide, lively start.
    bool     superOn       = false;
    int      superVoices   = 7;        // 7, 9, or 11
    float    superDetune   = 0.5f;     // 0..1, drives the spread curve
    float    superMix      = 0.5f;     // 0..1, centre ↔ sides balance
    float    ssPhase[10]   = {};
    float    ssInc[10]     = {};

    // LFSR noise register
    uint32_t lfsr      = 0x7FFFF8u;

    // Randomise the supersaw side-voice phases.  Called from the voice's
    // noteOn() so each new note starts wide rather than phase-aligned (which
    // would sound thin until the detune naturally drifts the voices apart).
    void randomizeSuperPhases() {
        for (int i = 0; i < 10; ++i) {
            lfsr ^= (lfsr << 13); lfsr ^= (lfsr >> 17); lfsr ^= (lfsr << 5);
            ssPhase[i] = float(lfsr & 0xFFFFu) / 65535.0f;
        }
    }

    void setFrequency(float freq, float sr) {
        phaseInc = std::max(0.0f, freq / sr);
        driftInc = 0.1f / sr;   // drift LFO ~0.1 Hz

        // Hypersaw: precompute per-voice detuned increments.
        // The 6 extra voices are spread symmetrically around the base frequency.
        // `pw` is repurposed as detune spread: 0=unison, 0.5=medium, 1.0=full JP-8000 width.
        // Max detune ≈ ±50 cents at pw=1.0 (JP-8000-style).
        if (wave == Wave::HYPERSAW && freq > 0.0f) {
            // Fixed spread ratios for 6 voices: ±12.5, ±25, ±37.5 cents relative to center
            static constexpr float kCentSpreads[6] = {
                -37.5f, -25.0f, -12.5f, +12.5f, +25.0f, +37.5f
            };
            const float spread = pw; // pw knob repurposed as detune spread
            for (int i = 0; i < 6; ++i) {
                const float cents = kCentSpreads[i] * spread;
                const float ratio = std::pow(2.0f, cents / 1200.0f);
                hypInc[i] = std::max(0.0f, freq * ratio / sr);
            }
        }

        // SUPERSAW mode — also precompute side-voice phase increments.
        // JP-8000-style: side voices are spread in symmetric pairs around the
        // centre voice with an exponential outward curve, so the outer pairs
        // are detuned much more than the inner ones (the classic "supersaw"
        // chorus shimmer).  The Detune knob (0..1) scales the maximum spread
        // up to ±50 cents; at 0 every voice tracks the centre (unison).
        if (superOn && freq > 0.0f) {
            const int sides = std::clamp(superVoices - 1, 0, 10);  // 6, 8, or 10
            const int pairs = sides / 2;                            // 3, 4, or 5
            const float maxCents = 50.0f;
            for (int p = 1; p <= pairs; ++p) {
                // Normalised position 0..1 — outer pair gets full curve.
                const float pos = float(p) / float(pairs);
                // Exponential outward: pos^1.7 concentrates spread on outer
                // voices (matches JP-8000 character).
                const float curve = std::pow(pos, 1.7f);
                const float cents = curve * maxCents * superDetune;
                const float ratioDn = std::pow(2.0f, -cents / 1200.0f);
                const float ratioUp = std::pow(2.0f, +cents / 1200.0f);
                const int idxLo = (p - 1) * 2;
                const int idxHi = idxLo + 1;
                ssInc[idxLo] = std::max(0.0f, freq * ratioDn / sr);
                ssInc[idxHi] = std::max(0.0f, freq * ratioUp / sr);
            }
        }
    }

    // Call once per note-on to precompute the bitcrusher quantisation step.
    // Avoids std::pow() on every sample in the inner loop.
    void updateAge(float a) {
        age = a;
        if (a > 0.0f) {
            const float bits = 2.0f + (1.0f - a) * 14.0f;
            qCache = std::pow(2.0f, bits);
        } else {
            qCache = 0.0f;
        }
    }

    // polyBLEP correction for seamless waveforms
    static float polyBlep(float t, float dt) {
        if (t < dt) {
            t /= dt; return t + t - t * t - 1.0f;
        } else if (t > 1.0f - dt) {
            t = (t - 1.0f) / dt; return t * t + t + t + 1.0f;
        }
        return 0.0f;
    }

    float process() {
        // TRANCE DRIFT: slow random pitch wobble per voice
        driftPhs += driftInc;
        if (driftPhs >= 1.0f) {
            driftPhs -= 1.0f;
            // Per-oscillator LFSR — thread-safe, no global rand()
            driftLfsr ^= (driftLfsr << 13); driftLfsr ^= (driftLfsr >> 17); driftLfsr ^= (driftLfsr << 5);
            driftVal = (float(driftLfsr & 0xFFFFu) / 32767.5f - 1.0f) * 0.003f * drift;
        }
        const float driftedInc = phaseInc * (1.0f + driftVal);

        phase += driftedInc;
        syncTick = false;
        if (phase >= 1.0f) {
            phase -= 1.0f;
            syncTick = true;
            // LFSR advance on cycle boundary (SID noise behaviour)
            if (lfsr & 1u) lfsr = (lfsr >> 1) ^ 0x400000u;
            else            lfsr >>= 1;
        }

        float out = 0.0f;
        switch (wave) {
        case Wave::SAW:
            out = phase * 2.0f - 1.0f;
            if (chipBlend > 0.0f && driftedInc > 0.0f)
                out -= chipBlend * polyBlep(phase, driftedInc);
            break;

        case Wave::TRI:
            out = (phase < 0.5f) ? (phase * 4.0f - 1.0f) : (3.0f - phase * 4.0f);
            break;

        case Wave::PULSE: {
            out = (phase < pw) ? 1.0f : -1.0f;
            if (chipBlend > 0.0f && driftedInc > 0.0f) {
                out += chipBlend * polyBlep(phase, driftedInc);
                out -= chipBlend * polyBlep(std::fmod(phase - pw + 1.0f, 1.0f), driftedInc);
            }
            break;
        }

        case Wave::NOISE: {
            // Map LFSR bits to [-1, 1]
            const float nv = float(int(lfsr & 0xFFFFu) - 32768) / 32768.0f;
            out = nv;
            break;
        }

        case Wave::SAW_TRI: {
            const float s = phase * 2.0f - 1.0f;
            const float t = (phase < 0.5f) ? (phase * 4.0f - 1.0f) : (3.0f - phase * 4.0f);
            out = (s + t) * 0.5f;
            break;
        }

        case Wave::RING:
            out = (phase * 2.0f - 1.0f) * ringIn;
            break;

        case Wave::HYPERSAW: {
            // 7-voice bandlimited supersaw (Roland JP-8000 style).
            // Voice 0 = primary phase (no detune); voices 1-6 = hypPhase[0..5].
            // PolyBLEP on all voices for alias suppression.
            const float saw0 = phase * 2.0f - 1.0f
                               - ((chipBlend > 0.0f && driftedInc > 0.0f)
                                  ? chipBlend * polyBlep(phase, driftedInc)
                                  : 0.0f);
            out = saw0;
            for (int i = 0; i < 6; ++i) {
                hypPhase[i] += hypInc[i];
                if (hypPhase[i] >= 1.0f) hypPhase[i] -= 1.0f;
                float sawi = hypPhase[i] * 2.0f - 1.0f;
                if (chipBlend > 0.0f && hypInc[i] > 0.0f)
                    sawi -= chipBlend * polyBlep(hypPhase[i], hypInc[i]);
                out += sawi;
            }
            // Normalise by voice count and apply a mild mix correction (-6 dBish)
            // so 7 voices ≈ same perceived loudness as a single saw.
            out *= 0.143f;  // ≈ 1/7
            break;
        }
        }

        // ── SUPERSAW mode (overrides the wave switch when enabled) ──────
        // Centre voice + N detuned side voices.  Each side voice advances its
        // own phase accumulator at the ratio set by setFrequency().  JP-8000
        // mix curve weighs centre vs sides; explicit 1/√N normalisation keeps
        // the perceived loudness constant as voice count or detune changes.
        if (superOn) {
            const int sides = std::clamp(superVoices - 1, 0, 10);  // 6/8/10
            // Centre voice: enveloped saw with polyBLEP (already-advanced phase).
            float centreSaw = phase * 2.0f - 1.0f;
            if (chipBlend > 0.0f && driftedInc > 0.0f)
                centreSaw -= chipBlend * polyBlep(phase, driftedInc);
            // Side voices: each accumulator advances independently.
            float sumSides = 0.0f;
            for (int i = 0; i < sides; ++i) {
                ssPhase[i] += ssInc[i];
                if (ssPhase[i] >= 1.0f) ssPhase[i] -= 1.0f;
                float sawi = ssPhase[i] * 2.0f - 1.0f;
                if (chipBlend > 0.0f && ssInc[i] > 0.0f)
                    sawi -= chipBlend * polyBlep(ssPhase[i], ssInc[i]);
                sumSides += sawi;
            }
            // JP-8000 mix curve (Adam Szabo's emulation paper):
            //   centre = -0.55366·m + 0.99785
            //   side   = -0.73764·m² + 1.2841·m + 0.044372
            // At m=0 the output is pure centre; at m=1 the sides dominate.
            const float m = std::clamp(superMix, 0.0f, 1.0f);
            const float centreLvl = -0.55366f * m + 0.99785f;
            const float sideLvl   = -0.73764f * m * m + 1.2841f * m + 0.044372f;
            // 1/√N gain compensation — total voices = 1 + sides.  Together
            // with the JP-8000 mix curve this keeps the perceived loudness
            // roughly constant across all voice-count / detune settings.
            const float norm = 1.0f / std::sqrt(float(1 + sides));
            out = (centreSaw * centreLvl + sumSides * sideLvl) * norm;
        }

        // DIGITAL AGE: reduce bit depth (qCache precomputed in updateAge() at note-on)
        if (age > 0.0f && qCache > 0.0f)
            out = std::round(out * qCache) / qCache;

        return out;   // volume applied by caller (prevents double-application at call sites)
    }
};

// ============================================================
//  NoiseGen — per-voice noise generator with 4 selectable colours.
//  Mixed in alongside the oscillators (before the filter) so it
//  inherits the existing filter + amp routing automatically.
// ============================================================
struct NoiseGen {
    enum class Type : int { White = 0, Pink, Vinyl, Sweep };
    Type     type = Type::White;

    // White-noise source (LFSR — thread-safe, no rand()).
    uint32_t lfsr = 0xA5A5A5A5u;

    // Pink-noise filter state (Paul Kellet's economy 3-pole 1/f).
    float    pinkB0 = 0.0f, pinkB1 = 0.0f, pinkB2 = 0.0f;

    // Vinyl-air state (1-pole HPF + sparse pop scheduler).
    float    vinylHpZ        = 0.0f;
    float    vinylPopCountdn = 0.0f;
    float    vinylPopLevel   = 0.0f;
    float    vinylPopDecay   = 1.0f;

    // Sweep-noise: state-variable bandpass + accumulated sweep phase.
    // The bandpass centre frequency is driven by `modulator` passed
    // in by the caller (usually the noise ADSR's current value) so
    // long noise attacks sweep up — perfect for risers/uplifters.
    float    svfLow = 0.0f;
    float    svfBand = 0.0f;

    void reset() {
        pinkB0 = pinkB1 = pinkB2 = 0.0f;
        vinylHpZ = vinylPopLevel = 0.0f;
        vinylPopCountdn = 0.0f;
        svfLow = svfBand = 0.0f;
    }

    // Return one sample of noise.  `sr` = sample rate; `modulator` is a
    // 0..1 value used by Sweep noise to drive the bandpass centre
    // (typically the host envelope value or a free LFO).  Other types
    // ignore it.  Output amplitude is roughly normalised so swapping
    // between types doesn't produce a level jump.
    float process(float sr, float modulator) {
        // White-noise step (fresh sample every call).
        lfsr ^= (lfsr << 13);
        lfsr ^= (lfsr >> 17);
        lfsr ^= (lfsr << 5);
        const float white = float(int(lfsr & 0xFFFFu) - 32768) / 32768.0f;

        switch (type) {
        default:
        case Type::White:
            return white;

        case Type::Pink: {
            // Paul Kellet's economy pink-noise filter (~10 dB / decade).
            pinkB0 = 0.99765f * pinkB0 + white * 0.0990460f;
            pinkB1 = 0.96300f * pinkB1 + white * 0.2965164f;
            pinkB2 = 0.57000f * pinkB2 + white * 1.0526913f;
            const float pink = pinkB0 + pinkB1 + pinkB2 + white * 0.1848f;
            return pink * 0.18f;   // gain-match to white
        }

        case Type::Vinyl: {
            // "Air" noise: 1-pole HPF (~3 kHz) shaped white + occasional
            // exponentially-decaying pops.  Sounds like a vinyl needle
            // riding the run-out groove.
            const float hpCoef = 0.985f;
            vinylHpZ = hpCoef * (vinylHpZ + white - vinylHpZ);  // simple HPF approximation
            const float hpOut = white - vinylHpZ;

            // Pop scheduler — at roughly every 50 ms, roll a die for a pop.
            vinylPopCountdn -= 1.0f / sr;
            if (vinylPopCountdn <= 0.0f) {
                vinylPopCountdn = 0.05f;
                // ~5 % chance of a pop
                lfsr ^= (lfsr << 13); lfsr ^= (lfsr >> 17); lfsr ^= (lfsr << 5);
                if ((lfsr & 0xFFu) < 13u) {
                    vinylPopLevel = (float(int(lfsr & 0xFFFFu) - 32768)
                                     / 32768.0f) * 0.7f;
                    vinylPopDecay = std::exp(-1.0f / (sr * 0.004f));   // 4 ms decay
                }
            }
            const float popOut = vinylPopLevel;
            vinylPopLevel *= vinylPopDecay;   // tail off
            return hpOut * 0.55f + popOut;
        }

        case Type::Sweep: {
            // Bandpass-filtered white noise — centre frequency driven by
            // `modulator`.  Maps modulator∈[0,1] to a logarithmic
            // 200 Hz..8 kHz sweep so a long noise ADSR attack produces
            // a natural-sounding upward sweep (riser).
            const float m = std::clamp(modulator, 0.0f, 1.0f);
            const float fHz = 200.0f * std::pow(40.0f, m);    // 200 → 8000
            const float f = 2.0f * std::sin(3.14159265f * fHz / sr);
            const float q = 0.30f;
            const float hp = white - svfLow - q * svfBand;
            svfBand += f * hp;
            svfLow  += f * svfBand;
            return svfBand * 1.4f;
        }
        }
    }
};

// ============================================================
//  ADSREnv — per-sample ADSR envelope
// ============================================================
struct ADSREnv {
    enum class Stage { Idle, Attack, Decay, Sustain, Release };
    Stage stage   = Stage::Idle;
    float value   = 0.0f;
    float attRate = 0.0f;
    float decRate = 0.0f;
    float susLvl  = 0.7f;
    float relRate = 0.0f;

    static float timeToRate(float seconds, float sr) {
        return (seconds <= 0.0001f) ? 1000.0f : (1.0f / (seconds * sr));
    }

    void setParams(float a, float d, float s, float r, float sr) {
        attRate = timeToRate(a, sr);
        decRate = timeToRate(d, sr);
        susLvl  = std::clamp(s, 0.0f, 1.0f);
        relRate = timeToRate(r, sr);
    }

    void noteOn()  { stage = Stage::Attack; }
    void noteOff() { if (stage != Stage::Idle) stage = Stage::Release; }
    bool isIdle()  const { return stage == Stage::Idle; }

    float process() {
        switch (stage) {
        case Stage::Idle: break;
        case Stage::Attack:
            // Exponential approach to 1 — fast initial transient, smooth rounding at top.
            // Matches hardware analogue (and SID) attack character better than linear.
            // Clamp to 1.0 before the threshold check: the additive step can exceed 1.0 by a
            // small amount when attRate is large relative to (1 - value), producing a one-sample
            // pop at the start of decay. std::min is branchless and eliminates the overshoot.
            value = std::min(value + attRate * (1.0f - value), 1.0f);
            if (value >= 0.9999f) { value = 1.0f; stage = Stage::Decay; }
            break;
        case Stage::Decay:
            // Exponential decay toward sustain level
            value += decRate * (susLvl - value);
            if (std::abs(value - susLvl) < 0.0001f) { value = susLvl; stage = Stage::Sustain; }
            break;
        case Stage::Sustain: break;
        case Stage::Release:
            // Exponential decay toward 0
            value *= (1.0f - relRate);
            if (value < 0.0001f) { value = 0.0f; stage = Stage::Idle; }
            break;
        }
        return value;
    }
};

// ============================================================
//  SIDFilter — Moog ladder filter tuned for SID character
// ============================================================
struct SIDFilter {
    float y1 = 0, y2 = 0, y3 = 0, y4 = 0, oldX = 0;
    float cutoff  = 1000.0f;
    float res     = 0.0f;
    float sr      = 44100.0f;
    int   type    = 0;   // 0=LP, 1=HP, 2=BP, 3=Notch
    bool  slope24 = true;

    void reset() { y1 = y2 = y3 = y4 = oldX = 0.0f; }

    float process(float x) {
        // Bilinear-transform 4-pole ladder (Huovilainen-style, stable + self-oscillating)
        const float fc = std::clamp(cutoff / sr, 0.0001f, 0.499f);
        const float g  = std::tan(3.14159265f * fc);
        const float k  = std::clamp(res * 4.0f, 0.0f, 3.96f);
        // Soft-clip input with resonance feedback
        x = std::tanh(x - k * y4);
        // One-pole stages using TPT integrator
        const float b = g / (1.0f + g);
        y1 += b * (x  - y1);
        y2 += b * (y1 - y2);
        y3 += b * (y2 - y3);
        y4 += b * (y3 - y4);
        switch (type) {
        case 0: return slope24 ? y4 : y2;                   // LP 24/12
        case 1: return slope24 ? (x - y4) : (x - y2);      // HP 24/12
        case 2: return 2.0f * (y2 - y4);                    // BP
        case 3: return x - 2.0f * (y2 - y4);               // Notch
        }
        return y4;
    }
};

// ============================================================
//  SIDVoice — one polyphonic voice (3 OSC + filter + envelopes)
// ============================================================
struct SIDVoice {
    SIDOscillator osc1, osc2, osc3;
    ADSREnv       env1, env2, env3, ampEnv, filterEnv;
    SIDFilter     filter;

    // Noise generator — mixed into the osc sum before the filter so it
    // shares the filter + amp signal path.  Has its own ADSR so the user
    // can shape a long sweep-noise riser independently of the amp envelope.
    NoiseGen      noiseGen;
    ADSREnv       noiseEnv;
    float         noiseLevel = 0.0f;

    int   note    = -1;
    int   age     = 0;      // voice allocation age counter
    bool  active  = false;
    float velocity = 1.0f;
    float sr      = 44100.0f;

    // Unison panning: -1 (full left) to +1 (full right), 0 = center
    float uniPan  = 0.0f;

    // Per-oscillator output snapshot for the UI oscilloscopes (no envelope
    // applied — raw post-volume oscillator signal so the display matches
    // what each waveform actually looks like).  Written each sample in
    // processMono(); read once per sample by processBlock for the first
    // active voice.
    float lastS1  = 0.0f;
    float lastS2  = 0.0f;
    float lastS3  = 0.0f;

    // Glide: pitch offset in semitones, converges to 0 each sample
    float glideOffsetSemis = 0.0f;

    // Pitch cache: recompute baseHz only when glide or pitch bend changes
    float cachedBaseHz_    = 0.0f;
    float cachedGlide_     = 1e30f;   // sentinel — forces recalc on first sample after noteOn
    float cachedPitchBend_ = 1e30f;   // sentinel — same

    // Mod matrix destination offsets (applied each block)
    float modCutoff = 0.0f;
    float modRes    = 0.0f;
    float modPW1    = 0.0f;
    float modPW2    = 0.0f;
    float modPW3    = 0.0f;   // OSC3 PW modulation (was missing — stub fixed)
    float modAmp    = 0.0f;
    float modFine1  = 0.0f;
    float modFine2  = 0.0f;

    void noteOn(int midiNote, float vel, float sampleRate) {
        note = midiNote; velocity = vel; sr = sampleRate;
        active = true;
        uniPan           = 0.0f;   // reset before handleNoteOn sets the real value;
                                   // prevents a stolen voice carrying a stale pan for 1 sample
        cachedGlide_     = 1e30f;   // invalidate pitch cache so first sample recomputes baseHz
        cachedPitchBend_ = 1e30f;
        filter.reset();
        noiseGen.reset();
        env1.noteOn(); env2.noteOn(); env3.noteOn();
        ampEnv.noteOn(); filterEnv.noteOn(); noiseEnv.noteOn();
    }

    void noteOff() {
        env1.noteOff(); env2.noteOff(); env3.noteOff();
        ampEnv.noteOff(); filterEnv.noteOff(); noiseEnv.noteOff();
    }

    // Returns mono sample — unison panning applied by caller via uniPan
    // glideRate:       fractional semitone convergence per sample (0 = no glide)
    // oscSync:         if true, OSC1/2 phase resets when OSC3 completes a cycle
    // fmAmt:           OSC3 → OSC1/2 FM depth (0..1)
    // pitchBendSemis:  pitch wheel offset in semitones (passed from processor, ±range)
    float processMono(float baseCutoff, float baseRes,
                      float keyTrack, float velSens, float filterEnvAmt,
                      float glideRate      = 0.0f,
                      bool  oscSync        = false,
                      float fmAmt          = 0.0f,
                      float pitchBendSemis = 0.0f) {
        if (!active) return 0.0f;

        const float e1 = env1.process();
        const float e2 = env2.process();
        const float e3 = env3.process();
        const float eA = ampEnv.process();
        const float fE = filterEnv.process();

        if (ampEnv.isIdle()) { active = false; return 0.0f; }

        // Glide convergence: move glideOffsetSemis toward 0 by glideRate each sample
        if (std::abs(glideOffsetSemis) > 0.001f) {
            const float step = glideRate * glideOffsetSemis;
            glideOffsetSemis -= std::clamp(step, -std::abs(glideOffsetSemis), std::abs(glideOffsetSemis));
        } else {
            glideOffsetSemis = 0.0f;
        }

        // MIDI → Hz: recompute baseHz only when pitch changes (glide every sample during
        // portamento; pitch bend on wheel move; steady state skips the pow() entirely).
        if (glideOffsetSemis != cachedGlide_ || pitchBendSemis != cachedPitchBend_) {
            cachedGlide_     = glideOffsetSemis;
            cachedPitchBend_ = pitchBendSemis;
            cachedBaseHz_    = 440.0f * std::pow(2.0f,
                                   (note - 69 + glideOffsetSemis + pitchBendSemis) / 12.0f);
        }
        auto oscHz = [&](int semi, float fine, float extraFine) {
            return cachedBaseHz_ * std::pow(2.0f, (semi + (fine + extraFine) / 100.0f) / 12.0f);
        };

        // OSC3 runs first so it can be used as sync/FM source for OSC1/2
        osc3.setFrequency(oscHz(osc3.semi, osc3.fine, 0.0f), sr);

        // Apply PW mod temporarily — save/restore so the base value isn't accumulated each sample
        const float savedPW1 = osc1.pw;
        const float savedPW2 = osc2.pw;
        const float savedPW3 = osc3.pw;
        osc1.pw = std::clamp(osc1.pw + modPW1, 0.05f, 0.95f);
        osc2.pw = std::clamp(osc2.pw + modPW2, 0.05f, 0.95f);
        osc3.pw = std::clamp(osc3.pw + modPW3, 0.05f, 0.95f);

        // OSC3 as ring-mod / FM source for osc1+osc2.
        // process() is called exactly once per sample to advance the phase.
        // s3osc : raw oscillator output (no envelope, no volume)
        // s3raw : envelope-shaped     (for FM depth and OSC3 mix contribution)
        // s3vol : envelope + volume   (what goes into the final output mixer)
        //
        // Ring mod carrier uses s3osc * volume (envelope-free) so that the RING
        // waveform is always audible regardless of OSC3's ADSR state.  Without
        // this, default OSC3 sustain=0 / decay=0.1 s kills the carrier instantly.
        const float s3osc = osc3.process();           // raw osc — advances phase once
        const float s3raw = s3osc * e3;               // envelope-shaped (mix/FM)
        const float s3vol = s3raw * osc3.volume;      // volume-scaled (final mix)

        osc1.ringIn = s3osc * osc3.volume;  // carrier: volume knob controls depth, not envelope
        osc2.ringIn = s3osc * osc3.volume;

        // OSC Sync: if OSC3 completed a cycle, reset OSC1/2 phase
        if (oscSync && osc3.syncTick) {
            osc1.phase = 0.0f;
            osc2.phase = 0.0f;
        }

        // FM: OSC3 modulates OSC1/2 pitch (linear FM for SID-style metallic tones)
        const float fmOffset = s3vol * fmAmt * 24.0f;  // up to ±24 semitones of FM

        osc1.setFrequency(oscHz(osc1.semi, osc1.fine, modFine1 + fmOffset * 100.0f), sr);
        osc2.setFrequency(oscHz(osc2.semi, osc2.fine, modFine2 + fmOffset * 100.0f * 0.7f), sr);

        const float s1 = osc1.process() * e1 * osc1.volume;
        const float s2 = osc2.process() * e2 * osc2.volume;
        const float s3 = s3vol;

        // Expose per-osc samples for the UI scopes.  Plain stores — single-
        // writer (audio thread) / single-reader (UI thread) means we don't
        // need atomics; worst case the UI sees a slightly torn float.
        lastS1 = s1;
        lastS2 = s2;
        lastS3 = s3;

        // Restore base PW so modulation doesn't accumulate across samples
        osc1.pw = savedPW1;
        osc2.pw = savedPW2;
        osc3.pw = savedPW3;

        // Noise generator — runs in parallel with the oscillators and is
        // mixed into the same pre-filter sum, so it inherits the existing
        // filter envelope and amp envelope routing.  The Sweep noise type
        // uses the noise envelope value as its sweep modulator, so a slow
        // attack naturally produces a rising-frequency riser.
        const float nEnv = noiseEnv.process();
        const float noiseRaw = (noiseLevel > 0.0001f)
                             ? noiseGen.process(sr, nEnv)
                             : 0.0f;
        const float noiseSig = noiseRaw * nEnv * noiseLevel;

        const float mixed = s1 + s2 + s3 + noiseSig;

        // Filter: cutoff from param + keytrack + filter env + mod
        const float keyOff    = (note - 60) * keyTrack * 50.0f;
        const float velOff    = (velocity - 0.5f) * velSens * 5000.0f;
        // Exponential filter envelope — fE*filterEnvAmt in octave domain gives
        // the classic trance "CUT click": with fast A, short D, low S the filter
        // spikes wide open then slams shut, producing a percussive transient.
        // Power-law shaping: sqrt(fE) for positive envAmt gives a logarithmic attack
        // curve — the filter opens VERY fast at first then plateaus, producing a
        // sharp transient "click" even with a 5-20ms attack time (like the Virus CUT).
        // For negative envAmt (filter closes on attack), sq(fE) gives the reverse.
        // Guard against sqrt(negative): fE can dip slightly below 0 in the Decay
        // stage when the exponential convergence overshoots sustain=0. Clamping to 0
        // avoids NaN propagation into envMult / filter.cutoff.
        const float fESafe    = std::max(0.0f, fE);
        const float fEShaped  = (filterEnvAmt >= 0.0f)
                                ? std::sqrt(fESafe)       // fast open: log curve
                                : fESafe * fESafe;        // slow open: exp curve
        // fE*filterEnvAmt*9 → up to 9 octaves (512× cutoff) when env is at peak.
        const float envMult   = std::pow(2.0f, fEShaped * filterEnvAmt * 9.0f);
        filter.cutoff = std::clamp((baseCutoff + keyOff + velOff + modCutoff * 12000.0f) * envMult,
                                   20.0f, 18000.0f);
        filter.res = std::clamp(baseRes + modRes, 0.0f, 0.99f);

        float filtered = filter.process(mixed);
        // NaN guard: high resonance + loud input can blow up the ladder filter.
        // Detect it and reset state so the voice recovers cleanly.
        if (!std::isfinite(filtered)) { filter.reset(); filtered = 0.0f; }

        // Amp: envelope × velocity × mod
        const float ampMod = 1.0f + std::clamp(modAmp, -0.9f, 1.0f);
        return filtered * eA * velocity * ampMod;
    }
};

// ============================================================
//  LFOEngine — shapes: Sine, Tri, Saw, RevSaw, Square, S&H, Step
// ============================================================
struct LFOEngine {
    enum class Shape : int { SINE = 0, TRI, SAW, REVSAW, SQUARE, SHOLD, STEP };

    // Musical sync divisions (index into kSyncDivMult): 0=1/32 .. 7=4/1

    Shape    shape     = Shape::SINE;
    float    phase     = 0.0f;
    float    rate      = 1.0f;     // Hz in free mode
    float    sr        = 44100.0f;
    bool     enabled   = true;     // on/off gate
    bool     syncOn    = false;
    int      syncDiv   = 3;        // index into kSyncDivMult (default: 1/4 note)
    bool     retrig    = true;
    float    holdVal   = 0.0f;
    uint32_t sHoldLfsr = 0x13579BDFu; // per-LFO — thread-safe S&H

    // 16 user-drawn step values in range -1..+1 (populated from APVTS each block)
    float stepValues[16] = {};

    void retrigger() { if (retrig) phase = 0.0f; }

    float process(float hostBPM) {
        // When sync is on, derive freq from BPM and the selected musical division.
        // Multiplier = how many cycles per quarter-note.
        // Indices 0-7 are the legacy straight divisions (preset compatibility).
        // Indices 8-12 add triplets and dotted variants.
        //   0=1/32, 1=1/16, 2=1/8, 3=1/4, 4=1/2, 5=1/1, 6=2/1, 7=4/1
        //   8=1/4T (triplet quarter, 1.5× rate of 1/4)
        //   9=1/8T (triplet eighth)
        //  10=1/16T
        //  11=1/4D (dotted quarter, 2/3 rate of 1/4)
        //  12=1/8D
        static const float kDiv[13] = {
            8.f,   4.f,   2.f,   1.f,   0.5f,  0.25f, 0.125f, 0.0625f,
            1.5f,  3.0f,  6.0f,                       // triplets
            (2.f/3.f), (4.f/3.f)                       // dotted
        };
        const float freq = syncOn
            ? (hostBPM / 60.0f) * kDiv[std::clamp(syncDiv, 0, 12)]
            : rate;
        const float inc  = freq / sr;
        phase += inc;
        if (phase >= 1.0f) {
            phase -= 1.0f;
            if (shape == Shape::SHOLD) {
                sHoldLfsr ^= (sHoldLfsr << 13); sHoldLfsr ^= (sHoldLfsr >> 17); sHoldLfsr ^= (sHoldLfsr << 5);
                holdVal = float(sHoldLfsr & 0xFFFFu) / 32767.5f - 1.0f;
            }
        }

        switch (shape) {
        case Shape::SINE:   return std::sin(phase * 6.28318530f);
        case Shape::TRI:    return (phase < 0.5f) ? (phase*4.0f-1.0f) : (3.0f-phase*4.0f);
        case Shape::SAW:    return phase * 2.0f - 1.0f;
        case Shape::REVSAW: return 1.0f - phase * 2.0f;
        case Shape::SQUARE: return phase < 0.5f ? 1.0f : -1.0f;
        case Shape::SHOLD:  return holdVal;
        case Shape::STEP: {
            // 16 evenly-spaced steps; sample-and-hold per step cell
            const int idx = std::clamp(int(phase * 16.0f), 0, 15);
            return stepValues[idx];
        }
        }
        return 0.0f;
    }
};

// ============================================================
//  Freeverb Schroeder reverb (8 comb + 4 allpass, stereo)
// ============================================================
struct SIDReverb {
    // Reference delay lengths (samples at 44100 Hz — Freeverb defaults)
    static constexpr int kCombRefL[]    = {1116,1188,1277,1356,1422,1491,1557,1617};
    static constexpr int kCombRefR[]    = {1139,1211,1300,1379,1445,1514,1580,1640};
    static constexpr int kAllpassRefL[] = {556, 441, 341, 225};
    static constexpr int kAllpassRefR[] = {579, 464, 364, 248};

    // Buffers sized for up to 192 kHz (≈4.35× the 44100 Hz lengths)
    static constexpr int kMaxComb    = 7200;
    static constexpr int kMaxAllpass = 2600;

    // Scaled lengths (recalculated in prepare() per actual sample rate)
    int scaledCombL[8]    = {1116,1188,1277,1356,1422,1491,1557,1617};
    int scaledCombR[8]    = {1139,1211,1300,1379,1445,1514,1580,1640};
    int scaledAllpassL[4] = {556, 441, 341, 225};
    int scaledAllpassR[4] = {579, 464, 364, 248};

    float combL[8][kMaxComb]    = {};
    float combR[8][kMaxComb]    = {};
    float allL[4][kMaxAllpass]  = {};
    float allR[4][kMaxAllpass]  = {};
    int   combPL[8] = {}, combPR[8] = {};
    int   allPL[4]  = {}, allPR[4]  = {};
    float combFiltL[8] = {}, combFiltR[8] = {};

    float roomSize = 0.5f;  // 0..1 → feedback
    float damp     = 0.5f;
    float wet      = 0.3f;

    void prepare(double sr) {
        const double ratio = sr / 44100.0;
        for (int i = 0; i < 8; ++i) {
            scaledCombL[i] = std::clamp(int(kCombRefL[i] * ratio + 0.5), 1, kMaxComb - 1);
            scaledCombR[i] = std::clamp(int(kCombRefR[i] * ratio + 0.5), 1, kMaxComb - 1);
        }
        for (int i = 0; i < 4; ++i) {
            scaledAllpassL[i] = std::clamp(int(kAllpassRefL[i] * ratio + 0.5), 1, kMaxAllpass - 1);
            scaledAllpassR[i] = std::clamp(int(kAllpassRefR[i] * ratio + 0.5), 1, kMaxAllpass - 1);
        }
        reset();
    }

    void reset() {
        for (auto& b : combL) for (auto& s : b) s = 0.0f;
        for (auto& b : combR) for (auto& s : b) s = 0.0f;
        for (auto& b : allL)  for (auto& s : b) s = 0.0f;
        for (auto& b : allR)  for (auto& s : b) s = 0.0f;
        for (auto& p : combPL)  p = 0;
        for (auto& p : combPR)  p = 0;
        for (auto& p : allPL)   p = 0;
        for (auto& p : allPR)   p = 0;
        for (auto& f : combFiltL) f = 0.0f;
        for (auto& f : combFiltR) f = 0.0f;
    }

    void process(float& L, float& R) {
        const float feedback = roomSize * 0.28f + 0.7f;
        const float d = damp;

        float outL = 0.0f, outR = 0.0f;

        // 8 comb filters
        for (int i = 0; i < 8; ++i) {
            const int lenL = scaledCombL[i], lenR = scaledCombR[i];
            float& fl = combFiltL[i]; float& fr = combFiltR[i];
            int&   pL = combPL[i];    int&   pR = combPR[i];

            const float dl = combL[i][pL];
            fl = dl * (1.0f - d) + fl * d;
            combL[i][pL] = L + feedback * fl;
            pL = (pL + 1) % lenL;
            outL += dl;

            const float dr = combR[i][pR];
            fr = dr * (1.0f - d) + fr * d;
            combR[i][pR] = R + feedback * fr;
            pR = (pR + 1) % lenR;
            outR += dr;
        }
        outL *= 0.015f; outR *= 0.015f;

        // 4 allpass filters
        for (int i = 0; i < 4; ++i) {
            const int lenL = scaledAllpassL[i], lenR = scaledAllpassR[i];
            int& pL = allPL[i]; int& pR = allPR[i];

            const float aL = allL[i][pL];
            allL[i][pL] = outL + aL * 0.5f;
            outL = aL - outL;
            pL = (pL + 1) % lenL;

            const float aR = allR[i][pR];
            allR[i][pR] = outR + aR * 0.5f;
            outR = aR - outR;
            pR = (pR + 1) % lenR;
        }

        const float dry = 1.0f - wet;
        L = L * dry + outL * wet;
        R = R * dry + outR * wet;
    }
};

// ============================================================
//  SIDDelay — ping-pong delay with tempo sync
// ============================================================
struct SIDDelay {
    static constexpr int kMaxDelay = 192001; // ~2s @ 96kHz
    float bufL[kMaxDelay] = {}, bufR[kMaxDelay] = {};
    int   writePos = 0;
    int   delayLen = 22050;
    float feedback = 0.3f;
    float mix      = 0.25f;
    bool  enabled  = true;

    void setDelayMs(float ms, float sr) {
        delayLen = std::clamp(int(ms * 0.001f * sr), 1, kMaxDelay - 1);
    }

    void setTempoDivision(int div, float bpm, float sr) {
        // div: 0=1/16, 1=1/8, 2=1/4, 3=3/8, 4=1/2, 5=3/4, 6=1/1
        static const float divs[] = {0.25f, 0.5f, 1.0f, 1.5f, 2.0f, 3.0f, 4.0f};
        const float beats = divs[std::clamp(div, 0, 6)];
        const float ms    = beats * 60000.0f / bpm;
        setDelayMs(ms, sr);
    }

    void process(float& L, float& R) {
        if (!enabled) return;

        // Read from delay (ping-pong: L reads from R tap, R reads from L tap)
        const int tapL = (writePos - delayLen + kMaxDelay) % kMaxDelay;
        const int tapR = (writePos - delayLen / 2 + kMaxDelay) % kMaxDelay;
        const float dL = bufL[tapL];
        const float dR = bufR[tapR];

        // Write with feedback
        bufL[writePos] = L + dR * feedback;
        bufR[writePos] = R + dL * feedback;
        writePos = (writePos + 1) % kMaxDelay;

        L = L * (1.0f - mix) + dL * mix;
        R = R * (1.0f - mix) + dR * mix;
    }

    void reset() {
        for (auto& s : bufL) s = 0.0f;
        for (auto& s : bufR) s = 0.0f;
        writePos = 0;
    }
};

// ============================================================
//  SIDChorus — 2-tap BBD-style stereo chorus
// ============================================================
struct SIDChorus {
    static constexpr int kMaxBuf = 4096;
    float bufL[kMaxBuf] = {}, bufR[kMaxBuf] = {};
    int   writePos = 0;
    float lfoPhase = 0.0f;
    float rate     = 1.2f;
    float depth    = 0.4f;
    float mix      = 0.3f;
    float sr       = 44100.0f;
    bool  enabled  = true;

    void process(float& L, float& R) {
        if (!enabled) return;

        bufL[writePos] = L;
        bufR[writePos] = R;

        lfoPhase += rate / sr;
        if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;

        const float lfo1 = std::sin(lfoPhase * 6.28318f);
        const float lfo2 = -lfo1;   // sin(x + π) == -sin(x) — saves a redundant transcendental

        const float delayMs1 = 5.0f + depth * 15.0f * (lfo1 + 1.0f) * 0.5f;
        const float delayMs2 = 5.0f + depth * 15.0f * (lfo2 + 1.0f) * 0.5f;

        auto readBuf = [&](float* buf, float delayMs) {
            const float delaySamples = delayMs * sr * 0.001f;
            const int   d0 = int(delaySamples);
            const float frac = delaySamples - d0;
            const int   p0 = (writePos - d0 + kMaxBuf)     % kMaxBuf;
            const int   p1 = (writePos - d0 - 1 + kMaxBuf) % kMaxBuf;
            return buf[p0] * (1.0f - frac) + buf[p1] * frac;
        };

        const float wetL = readBuf(bufL, delayMs1);
        const float wetR = readBuf(bufR, delayMs2);

        writePos = (writePos + 1) % kMaxBuf;
        L = L * (1.0f - mix) + wetL * mix;
        R = R * (1.0f - mix) + wetR * mix;
    }

    void reset() { for (auto& s : bufL) s=0; for (auto& s : bufR) s=0; }
};

// ============================================================
//  ArpSequencer — 16-step pattern with DAW sync
// ============================================================
struct ArpSequencer {
    enum class Mode : int { UP = 0, DOWN, UPDOWN, RANDOM, ORDER };

    bool  steps[16]    = {};
    Mode  mode         = Mode::UP;
    int   octave       = 1;
    float gate         = 0.75f;
    float swing        = 0.0f;
    float tempo        = 140.0f;
    bool  syncToDaw    = true;
    bool  enabled      = false;

    // Internal state
    int   stepIdx      = 0;
    int   direction    = 1;
    float beatPhase    = 0.0f;
    float gateSamples  = 0.0f;
    float gateCount    = 0.0f;
    bool  gateOpen     = false;

    // Fixed-size held-note array: no heap allocation on the audio thread.
    static constexpr int kMaxHeld = 32;
    int  heldNotes[kMaxHeld] = {};
    int  heldCount            = 0;
    int      currentNote  = -1;
    int      octOffset    = 0;
    uint32_t randLfsr     = 0xFACEB00Cu; // thread-safe random for arp RANDOM mode

    void addNote(int note) {
        for (int i = 0; i < heldCount; ++i)
            if (heldNotes[i] == note) return;  // already held
        if (heldCount < kMaxHeld)
            heldNotes[heldCount++] = note;
    }
    void removeNote(int note) {
        for (int i = 0; i < heldCount; ++i) {
            if (heldNotes[i] == note) {
                heldNotes[i] = heldNotes[--heldCount];  // swap with last, shrink count
                return;
            }
        }
    }
};

// ============================================================
//  TranceGate — 16-step volume gate with swing
// ============================================================
struct TranceGate {
    bool  enabled    = false;
    float levels[16] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};  // 0..1
    float swing      = 0.0f;

    int   stepIdx    = 0;
    float beatPhase  = 0.0f;
    float currentLevel = 1.0f;   // smoothed gate level

    float process(float samp, float stepLen, float smoothCoef) {
        // Advance to next step if needed (called once per sample)
        const bool isOdd = (stepIdx % 2 == 1);
        const float swingOff = stepLen * swing * 0.5f;
        const float curLen   = stepLen + (isOdd ? swingOff : -swingOff);
        beatPhase += 1.0f;
        if (beatPhase >= curLen) {
            beatPhase -= curLen;
            stepIdx = (stepIdx + 1) % 16;
        }
        // Smooth toward current step level
        const float target = levels[stepIdx];
        currentLevel += (target - currentLevel) * smoothCoef;
        return samp * currentLevel;
    }

    // Process L and R in one call so beatPhase advances exactly once per sample.
    // Calling process() twice (once per channel) would run the gate at 2× speed.
    void processStereo(float& L, float& R, float stepLen, float smoothCoef) {
        const bool isOdd = (stepIdx % 2 == 1);
        const float swingOff = stepLen * swing * 0.5f;
        const float curLen   = stepLen + (isOdd ? swingOff : -swingOff);
        beatPhase += 1.0f;
        if (beatPhase >= curLen) {
            beatPhase -= curLen;
            stepIdx = (stepIdx + 1) % 16;
        }
        const float target = levels[stepIdx];
        currentLevel += (target - currentLevel) * smoothCoef;
        L *= currentLevel;
        R *= currentLevel;
    }

    void reset() { stepIdx = 0; beatPhase = 0.0f; currentLevel = 1.0f; }
};

// ============================================================
//  SIDTranceAudioProcessor
// ============================================================
class SIDTranceAudioProcessor : public juce::AudioProcessor
{
public:
    SIDTranceAudioProcessor();
    ~SIDTranceAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "SIDyssey"; }
    bool   acceptsMidi()     const override { return true; }
    bool   producesMidi()    const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int  getNumPrograms()               override { return 1; }
    int  getCurrentProgram()            override { return 0; }
    void setCurrentProgram(int)         override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() { return apvts; }

    // Scope ring buffer for oscilloscope display (UI reads this)
    juce::AbstractFifo         scopeFifo  { 2048 };
    juce::AudioBuffer<float>   scopeBuf   { 2, 2048 };

    // Per-oscillator scope ring buffers (mono, fed from the first active voice
    // in processBlock).  Lock-free: audio thread only writes, UI only reads.
    // Wrapped in a struct because juce::AbstractFifo has explicit ctor + non-
    // copyable atomic members, so an array of them can't be brace-initialised.
    struct OscScope {
        juce::AbstractFifo       fifo { 2048 };
        juce::AudioBuffer<float> buf  { 1, 2048 };
    };
    OscScope oscScope[3];

    // Set to true in setStateInformation so the editor reloads all widget values
    // on the next render tick (Visage controls are not APVTS listeners).
    std::atomic<bool> stateJustLoaded { false };

    // Output peak levels for the top-header VU meter.  Updated at the end
    // of every processBlock with a peak-hold + slow exponential decay so
    // the UI sees natural-looking ballistics.  Atomic for lock-free read
    // from the editor's render thread.
    std::atomic<float> outPeakL { 0.0f };
    std::atomic<float> outPeakR { 0.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Voice management
    int allocateVoice(int midiNote, const int* excluded = nullptr, int nExcluded = 0);
    void handleNoteOn(int ch, int note, float vel);
    void handleNoteOff(int ch, int note);
    // Per-block cached mod values (avoids APVTS map lookup inside the sample loop)
    struct CachedMod { int src = 0, dst = 0; float amt = 0.0f; };
    void applyModMatrix(SIDVoice& v, float lfo1v, float lfo2v, float vel, float mw,
                        const CachedMod* mods);
    void processArpEvents(juce::MidiBuffer& out, int numSamples, float bpm);
    float analogGlow(float x, float amount, int type = 0, float precomputedBitcrushQ = 0.0f);

    juce::AudioProcessorValueTreeState apvts;

    // DSP instances
    static constexpr int kMaxVoices  = 16;
    static constexpr int kMaxUnison  = 16;

    std::array<SIDVoice, kMaxVoices> voices;
    int voiceCounter = 0;  // age counter for voice steal

    LFOEngine   lfo1, lfo2;
    ArpSequencer arp;
    TranceGate  gate;
    SIDChorus   chorus;
    SIDDelay    delay;
    SIDReverb   reverb;

    double      sr        = 44100.0;
    float       modWheelVal = 0.0f;
    float       lastVelocity = 1.0f;

    juce::SmoothedValue<float, juce::ValueSmoothingTypes::Multiplicative> smCutoff;
    juce::SmoothedValue<float> smRes, smMasterVol;

    // Lookahead limiter state
    float limEnvL = 0.0f, limEnvR = 0.0f;

    // Cached raw-parameter pointers — filled in prepareToPlay, used on audio thread
    // to avoid juce::String construction and HashMap lookups inside the hot path.
    std::atomic<float>* seqStepParams_[16]  = {};   // seq_step_01..16
    std::atomic<float>* modSrcParams_[4]    = {};   // mod1_src .. mod4_src
    std::atomic<float>* modAmtParams_[4]    = {};   // mod1_amt .. mod4_amt
    std::atomic<float>* modDstParams_[4]    = {};   // mod1_dst .. mod4_dst
    std::atomic<float>* gateStepParams_[16]  = {};   // gate_step_01..16
    std::atomic<float>* lfo1StepParams_[16] = {};   // lfo1_step_01..16 (step-curve LFO)
    std::atomic<float>* lfo2StepParams_[16] = {};   // lfo2_step_01..16 (step-curve LFO)
    std::atomic<float>* masterVoicesParam_  = {};   // master_voices (used in allocateVoice)
    std::atomic<float>* masterPolyParam_    = {};   // master_poly   (used in allocateVoice)

    // Note-on / arp params — cached to avoid juce::String heap alloc on the audio thread.
    // getRawParameterValue(const char*) constructs a String + does a HashMap lookup every call.
    std::atomic<float>* digitalAgeParam_   = {};   // digital_age
    std::atomic<float>* tranceDriftParam_  = {};   // trance_drift
    std::atomic<float>* chipModelParam_    = {};   // chip_model   (0=6581, 1=8580)
    std::atomic<float>* uniVoicesParam_    = {};   // unison_voices
    std::atomic<float>* uniDetuneParam_    = {};   // unison_detune
    std::atomic<float>* uniSpreadParam_    = {};   // unison_spread
    std::atomic<float>* glideModeParam_    = {};   // glide_mode
    std::atomic<float>* ampAtkParam_       = {};   // amp_attack
    std::atomic<float>* ampDecParam_       = {};   // amp_decay
    std::atomic<float>* ampSusParam_       = {};   // amp_sustain
    std::atomic<float>* ampRelParam_       = {};   // amp_release
    std::atomic<float>* fenvAtkParam_      = {};   // filter_env_attack
    std::atomic<float>* fenvDecParam_      = {};   // filter_env_decay
    std::atomic<float>* fenvSusParam_      = {};   // filter_env_sustain
    std::atomic<float>* fenvRelParam_      = {};   // filter_env_release
    std::atomic<float>* arpTempoParam_     = {};   // arp_tempo
    std::atomic<float>* arpSyncParam_      = {};   // arp_sync
    std::atomic<float>* arpGateParam_      = {};   // arp_gate
    std::atomic<float>* arpSwingParam_     = {};   // arp_swing
    std::atomic<float>* arpOctaveParam_    = {};   // arp_octave
    std::atomic<float>* arpModeParam_      = {};   // arp_mode
    // Noise generator
    std::atomic<float>* noiseTypeParam_    = {};
    std::atomic<float>* noiseAtkParam_     = {};
    std::atomic<float>* noiseDecParam_     = {};
    std::atomic<float>* noiseSusParam_     = {};
    std::atomic<float>* noiseRelParam_     = {};
    std::atomic<float>* noiseLevelParam_   = {};

    // Per-oscillator cached pointers (index 0=osc1, 1=osc2, 2=osc3).
    // Eliminates juce::String heap allocation + HashMap lookup per note-on.
    std::atomic<float>* oscWaveParam_[3] = {};
    std::atomic<float>* oscSemiParam_[3] = {};
    std::atomic<float>* oscFineParam_[3] = {};
    std::atomic<float>* oscPwParam_[3]   = {};
    std::atomic<float>* oscVolParam_[3]  = {};
    std::atomic<float>* oscAtkParam_[3]  = {};
    std::atomic<float>* oscDecParam_[3]  = {};
    std::atomic<float>* oscSusParam_[3]  = {};
    std::atomic<float>* oscRelParam_[3]  = {};
    // SUPERSAW mode params (cached so handleNoteOn doesn't touch APVTS strings).
    std::atomic<float>* oscSuperOnParam_[3]     = {};
    std::atomic<float>* oscSuperVoicesParam_[3] = {};
    std::atomic<float>* oscSuperDetuneParam_[3] = {};
    std::atomic<float>* oscSuperMixParam_[3]    = {};

    // Pitch bend
    float pitchBendSemis_ = 0.0f;    // current wheel offset in semitones (updated from MIDI)
    static constexpr float kPitchBendRange = 2.0f;  // ±2 semitones (standard GM)

    // Sustain pedal (CC64) — deferred note-off logic
    bool  sustainHeld_   = false;
    int   sustainedNotes_[128] = {};  // notes waiting to be released when pedal lifts
    int   sustainedCount_ = 0;

    // Glide tracking: last played note for glide interval calculation
    int   lastPlayedNote_  = -1;
    float glideOffsetAccum_ = 0.0f;  // current glide offset in semitones (per-voice set at noteOn)

    // Per-block scratch buffer for the per-osc scope feed.  Sized to the host's
    // max block size in prepareToPlay(); avoids any audio-thread allocation.
    std::vector<float> oscScopeScratch_[3];

    // Dual-engine chip selector: smoothed crossfade between 6581 and 8580
    // character.  0.0 = pure 6581 (raw aliased), 1.0 = pure 8580 (full
    // polyBLEP correction).  Smoothing time set in prepareToPlay(); the audio
    // thread reads chipModelSmoothed_.getNextValue() once per sample and
    // pushes that into every active voice's three oscillators.
    juce::SmoothedValue<float> chipModelSmoothed_;

public:
    // Audio-thread-safe (target update is atomic via SmoothedValue).
    // Called from processBlock() each block after reading chipModelParam_.
    // Tests / advanced hosts may also call this from the message thread —
    // the smoothing ramp absorbs any timing jitter so transitions stay
    // click-free regardless of when the target moves.
    void setModelTarget(int chipIndex) noexcept {
        chipModelSmoothed_.setTargetValue(juce::jlimit(0.0f, 1.0f, float(chipIndex)));
    }

private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SIDTranceAudioProcessor)
};
