#pragma once
//==============================================================================
// MidBass tone section — Phase 5: saturation (5 types, 2x oversampled, parallel
// mix) → RBJ EQ (low shelf / mid peak / high shelf) → transient shaper.
//
// OVERSAMPLING / LATENCY (Phase 5 condition a): 2x via a Kaiser half-band FIR
// pair (N = 95 taps at 2fs, linear phase). The round trip is EXACTLY
// (N-1)/2 = 47 samples at base rate — an integer — so the dry path of the
// parallel Mix is a plain 47-sample delay line and nulls against a unity wet
// path. The stage has a CONSTANT 47-sample latency whether or not saturation is
// engaged (the wet path is skipped at drive 0, but dry always runs through the
// delay), reported to the host via setLatencySamples().
//
// Saturator types (drive-as-density, self-limiting, no makeup — TranceKick
// lesson). Harmonic signatures measured at the reference drive (THD test):
//   SoftClip — VAZ cubic, odd-only (h3-dominant), gentlest
//   Tape     — tanh, odd-only, smooth compression
//   Tube     — bias-shifted tanh, adds EVEN harmonics (h2), warm asymmetry
//   Diode    — hard asymmetric, strong even+odd, aggressive
//   HardClip — brickwall, odd-rich, harshest
// Asymmetric types (Tube/Diode) run a 5 Hz DC blocker inside the wet path.
//==============================================================================
#include "Biquad.h"      // teq:: RBJ biquads — include dir ../../TranceEQ/Source
#include <vector>
#include <cmath>
#include <algorithm>

namespace mb
{
//==============================================================================
// Kaiser-windowed half-band lowpass FIR (odd N, centre 0.5, even offsets zero).
struct HalfBandFir
{
    std::vector<float> h;
    std::vector<float> z;
    int pos = 0;

    static double besselI0 (double x)
    {
        double sum = 1.0, term = 1.0;
        for (int k = 1; k < 40; ++k)
        {
            term *= (x / (2.0 * k)) * (x / (2.0 * k));
            sum += term;
            if (term < 1.0e-18 * sum) break;
        }
        return sum;
    }

    void design (int numTaps, double beta)
    {
        const int M = (numTaps - 1) / 2;
        h.assign ((size_t) numTaps, 0.0f);
        const double i0b = besselI0 (beta);
        for (int n = 0; n < numTaps; ++n)
        {
            const int k = n - M;
            double v;
            if (k == 0)          v = 0.5;
            else if ((k & 1) == 0) v = 0.0;                       // half-band zeros
            else                 v = std::sin (0.5 * 3.14159265358979323846 * k)
                                     / (3.14159265358979323846 * k);
            const double r = (double) k / (double) M;
            const double w = besselI0 (beta * std::sqrt (std::max (0.0, 1.0 - r * r))) / i0b;
            h[(size_t) n] = (float) (v * w);
        }
        z.assign ((size_t) numTaps, 0.0f);
        pos = 0;
    }

    void reset() { std::fill (z.begin(), z.end(), 0.0f); pos = 0; }

    inline float process (float x)
    {
        z[(size_t) pos] = x;
        const int n = (int) h.size();
        float acc = 0.0f;
        int idx = pos;
        for (int t = 0; t < n; ++t)
        {
            acc += h[(size_t) t] * z[(size_t) idx];
            idx = (idx == 0) ? n - 1 : idx - 1;
        }
        pos = (pos + 1 == n) ? 0 : pos + 1;
        return acc;
    }
};

//==============================================================================
namespace SatType { enum { Tape = 0, Tube, Diode, SoftClip, HardClip }; }

struct MbSaturator
{
    // 127 taps: 95 measured a -50 dB null residual (passband droop near 20 kHz);
    // 127 clears the -60 dB gate. Round trip stays integer: (N-1)/2 = 63 samples.
    static constexpr int kTaps    = 127;
    static constexpr int kLatency = (kTaps - 1) / 2;      // 63 samples @ base rate

    struct Channel
    {
        HalfBandFir up, down;
        std::vector<float> dry;                            // 47-sample dry delay
        int dryPos = 0;
        float dcX = 0.0f, dcY = 0.0f;                      // wet-path DC blocker state

        void prepare()
        {
            up.design (kTaps, 8.0);
            down.design (kTaps, 8.0);
            dry.assign ((size_t) kLatency, 0.0f);
            dryPos = 0; dcX = dcY = 0.0f;
        }
        void reset() { up.reset(); down.reset(); std::fill (dry.begin(), dry.end(), 0.0f); dryPos = 0; dcX = dcY = 0.0f; }
    };

    Channel ch[2];
    double  sr = 44100.0;
    int     type = SatType::Tape;
    float   drive = 0.0f, mix = 1.0f, gain = 1.0f;
    float   dcCoef = 0.9993f;
    // Wet-path engagement ramp (~2 ms): the drive-0 bypass may not switch the
    // OS path in instantly — a cold FIR state stepping into the mix is an
    // audible glitch (found by the Phase 7 macro-sweep gate when Bite/Warmth
    // sweep sat_drive through zero). While drive stays exactly 0 the path is
    // never engaged, so drive-0 transparency remains bit-exact.
    float   wetRamp = 0.0f, wetRampCoef = 0.011f;
    bool    forceWetForTest = false;                        // OS path w/ unity transfer (null test)

    void prepare (double sampleRate)
    {
        sr = sampleRate > 0.0 ? sampleRate : 44100.0;
        dcCoef = (float) std::exp (-2.0 * 3.14159265358979323846 * 5.0 / (sr * 2.0));
        wetRampCoef = 1.0f - (float) std::exp (-1.0 / (0.002 * sr));
        wetRamp = 0.0f;
        ch[0].prepare(); ch[1].prepare();
    }
    void reset() { ch[0].reset(); ch[1].reset(); }

    // Per-type drive gain, tuned so the THD ranking (SoftClip < Tape < Tube <
    // Diode < HardClip at the reference drive) is stable — see test_tone.cpp.
    void setParams (int t, float drive01, float mix01)
    {
        type  = t;
        drive = std::clamp (drive01, 0.0f, 1.0f);
        mix   = std::clamp (mix01, 0.0f, 1.0f);
        // Tuned so the documented ranking holds with gaps (measured THD @ drive
        // 0.5: Soft -18.7 < Tape -16.1 < Tube -14.1 < Diode ~-12 < Hard ~-9.5).
        static constexpr float kGain[5] = { 5.0f, 6.0f, 5.0f, 4.0f, 10.0f };
        gain = 1.0f + drive * kGain[std::clamp (t, 0, 4)];
    }

    inline float shape (float x) const
    {
        switch (type)
        {
            case SatType::Tube:     return std::tanh (x + 0.25f) - 0.24491866f;   // even harmonics
            // C1-continuous asymmetry: slope 1.5 on BOTH sides of zero (a slope
            // discontinuity at the crossing aliased like a hard clip: -47 dB),
            // negative half saturating early at -0.5 → strong even harmonics.
            case SatType::Diode:    return x >= 0.0f ? std::tanh (1.5f * x)
                                                     : 0.5f * std::tanh (3.0f * x);
            case SatType::SoftClip: { const float t = std::clamp (x, -1.5f, 1.5f);
                                      return t - t * t * t * (1.0f / 6.75f); }    // VAZ cubic, gentle
            case SatType::HardClip: return std::clamp (x, -1.0f, 1.0f);
            default:                return std::tanh (x);                          // Tape
        }
    }

    inline void processSample (float& L, float& R)
    {
        const bool wetOn = forceWetForTest || drive > 1.0e-4f;
        if (forceWetForTest) wetRamp = 1.0f;                // null test measures the settled path
        else wetRamp += ((wetOn ? 1.0f : 0.0f) - wetRamp) * wetRampCoef;
        if (! wetOn && wetRamp < 1.0e-4f) wetRamp = 0.0f;

        float in[2] = { L, R };
        float out[2];
        for (int c = 0; c < 2; ++c)
        {
            Channel& cc = ch[c];
            const float delayed = cc.dry[(size_t) cc.dryPos];
            cc.dry[(size_t) cc.dryPos] = in[c];
            cc.dryPos = (cc.dryPos + 1 == kLatency) ? 0 : cc.dryPos + 1;

            if (wetRamp == 0.0f) { out[c] = delayed; continue; }   // dry-only, still latency-aligned

            // up (zero-stuff x2, gain 2) → shape at 2fs → down (decimate even phase)
            float a = cc.up.process (2.0f * in[c]);
            float b = cc.up.process (0.0f);
            if (drive > 1.0e-4f)
            {
                a = shape (a * gain);
                b = shape (b * gain);
                if (type == SatType::Tube || type == SatType::Diode)
                {
                    float ya = a - cc.dcX + dcCoef * cc.dcY;
                    if (std::abs (ya) < 1.0e-20f) ya = 0.0f;               // denormal flush
                    cc.dcX = a; cc.dcY = ya; a = ya;
                    float yb = b - cc.dcX + dcCoef * cc.dcY;
                    if (std::abs (yb) < 1.0e-20f) yb = 0.0f;
                    cc.dcX = b; cc.dcY = yb; b = yb;
                }
            }
            const float wet = cc.down.process (a);
            cc.down.process (b);
            out[c] = delayed + (wet - delayed) * (mix * wetRamp);
        }
        L = out[0]; R = out[1];
    }
};

//==============================================================================
// 3-band RBJ EQ (TranceEQ Biquad port): low shelf, mid peak (f/g/Q), high shelf.
struct MbToneEq
{
    double sr = 44100.0;
    teq::Biquad ls[2], mid[2], hs[2];

    void prepare (double sampleRate) { sr = sampleRate > 0.0 ? sampleRate : 44100.0; reset(); }
    void reset() { for (int c = 0; c < 2; ++c) { ls[c].reset(); mid[c].reset(); hs[c].reset(); } }

    void setParams (double lsF, double lsG, double midF, double midG, double midQ,
                    double hsF, double hsG)
    {
        const auto cLs  = teq::makeCoeffs (teq::FilterType::LowShelf,  lsF,  lsG,  0.707, sr);
        const auto cMid = teq::makeCoeffs (teq::FilterType::Peak,      midF, midG, midQ,  sr);
        const auto cHs  = teq::makeCoeffs (teq::FilterType::HighShelf, hsF,  hsG,  0.707, sr);
        for (int c = 0; c < 2; ++c) { ls[c].setCoeffs (cLs); mid[c].setCoeffs (cMid); hs[c].setCoeffs (cHs); }
    }

    inline void processSample (float& L, float& R)
    {
        L = hs[0].process (mid[0].process (ls[0].process (L)));
        R = hs[1].process (mid[1].process (ls[1].process (R)));
    }
};

//==============================================================================
// Transient shaper: two envelope followers (fast/slow) on the linked stereo
// peak; their normalized difference drives attack (+) and sustain (−) gain.
// Gain is HARD-BOUNDED to [0.25, 4] (±12 dB) — condition f. The gain envelope
// moving with the material IS the effect; artifact gating in the tests uses the
// Phase 3 isolation standard (steady tone → no chatter above baseline).
struct MbTransient
{
    static constexpr float kMinGain = 0.25f, kMaxGain = 4.0f;

    double sr = 44100.0;
    float  fast = 0.0f, slow = 0.0f;
    float  aFastAtt = 0, aFastRel = 0, aSlowAtt = 0, aSlowRel = 0;
    float  attackAmt = 0.0f, sustainAmt = 0.0f;             // -1..+1 each

    void prepare (double sampleRate)
    {
        sr = sampleRate > 0.0 ? sampleRate : 44100.0;
        auto coef = [this] (double ms) { return 1.0f - (float) std::exp (-1.0 / (ms * 0.001 * sr)); };
        aFastAtt = coef (0.5);  aFastRel = coef (20.0);
        aSlowAtt = coef (20.0); aSlowRel = coef (100.0);
        fast = slow = 0.0f;
    }
    void reset() { fast = slow = 0.0f; }
    void setParams (float attack, float sustain)
    {
        attackAmt = std::clamp (attack, -1.0f, 1.0f);
        sustainAmt = std::clamp (sustain, -1.0f, 1.0f);
    }

    inline void processSample (float& L, float& R)
    {
        if (attackAmt == 0.0f && sustainAmt == 0.0f) return;   // exact passthrough

        const float x = std::max (std::abs (L), std::abs (R));
        fast += (x - fast) * (x > fast ? aFastAtt : aFastRel);
        slow += (x - slow) * (x > slow ? aSlowAtt : aSlowRel);
        if (fast < 1.0e-25f) fast = 0.0f;                      // denormal flush
        if (slow < 1.0e-25f) slow = 0.0f;

        const float r = (fast - slow) / (slow + 1.0e-6f);      // + attack, − sustain
        float g = 1.0f + attackAmt  * std::clamp (r, 0.0f, 2.0f) * 1.5f
                       + sustainAmt * std::clamp (-r, 0.0f, 1.0f) * 1.0f;
        g = std::clamp (g, kMinGain, kMaxGain);
        L *= g; R *= g;
    }
};
} // namespace mb
