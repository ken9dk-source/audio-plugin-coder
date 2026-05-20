#pragma once

#include <JuceHeader.h>
#include <array>
#include <cmath>
#include <cstdint>

// ============================================================
//  SIDOscillator — 6581-inspired phase-accumulator oscillator
// ============================================================
struct SIDOscillator {
    enum class Wave : int { SAW = 0, TRI, PULSE, NOISE, SAW_TRI, RING };

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

    // DIGITAL AGE (bitcrusher)
    float    age       = 0.0f;   // amount 0..1

    // SID mode: true = aliased (Classic), false = polyBLEP (Modern/Trance)
    bool     aliased   = false;

    // Ring mod: feed from other osc
    float    ringIn    = 0.0f;

    // LFSR noise register
    uint32_t lfsr      = 0x7FFFF8u;

    void setFrequency(float freq, float sr) {
        phaseInc = std::max(0.0f, freq / sr);
        driftInc = 0.1f / sr;   // drift LFO ~0.1 Hz
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
            driftVal = (float(rand()) / RAND_MAX * 2.0f - 1.0f) * 0.003f * drift;
        }
        const float driftedInc = phaseInc * (1.0f + driftVal);

        phase += driftedInc;
        if (phase >= 1.0f) {
            phase -= 1.0f;
            // LFSR advance on cycle boundary (SID noise behaviour)
            if (lfsr & 1u) lfsr = (lfsr >> 1) ^ 0x400000u;
            else            lfsr >>= 1;
        }

        float out = 0.0f;
        switch (wave) {
        case Wave::SAW:
            out = phase * 2.0f - 1.0f;
            if (!aliased && driftedInc > 0.0f)
                out -= polyBlep(phase, driftedInc);
            break;

        case Wave::TRI:
            out = (phase < 0.5f) ? (phase * 4.0f - 1.0f) : (3.0f - phase * 4.0f);
            break;

        case Wave::PULSE: {
            out = (phase < pw) ? 1.0f : -1.0f;
            if (!aliased && driftedInc > 0.0f) {
                out += polyBlep(phase, driftedInc);
                out -= polyBlep(std::fmod(phase - pw + 1.0f, 1.0f), driftedInc);
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
        }

        // DIGITAL AGE: reduce bit depth
        if (age > 0.0f) {
            const float bits = 2.0f + (1.0f - age) * 14.0f;
            const float q    = std::pow(2.0f, bits);
            out = std::round(out * q) / q;
        }

        return out * volume;
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
        case Stage::Idle:    break;
        case Stage::Attack:
            value += attRate;
            if (value >= 1.0f) { value = 1.0f; stage = Stage::Decay; }
            break;
        case Stage::Decay:
            value -= decRate;
            if (value <= susLvl) { value = susLvl; stage = Stage::Sustain; }
            break;
        case Stage::Sustain: break;
        case Stage::Release:
            value -= relRate;
            if (value <= 0.0f) { value = 0.0f; stage = Stage::Idle; }
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
        const float f  = std::min(0.99f, 2.0f * cutoff / sr);
        const float k  = 3.6f * f - 1.6f * f * f - 1.0f;
        const float p  = (k + 1.0f) * 0.5f;
        const float sc = std::exp((1.0f - p) * 1.386249f);
        const float r  = res * sc;

        const float xn = x - r * y4;
        y1 = xn * p + oldX * p - k * y1;
        y2 = y1 * p + y1  * p - k * y2;
        y3 = y2 * p + y2  * p - k * y3;
        y4 = y3 * p + y3  * p - k * y4;
        y4 = std::tanh(y4);
        oldX = xn;

        switch (type) {
        case 0: return slope24 ? y4 : y2;
        case 1: return x - y4;
        case 2: return 3.0f * (y3 - y4);
        case 3: return x - 3.0f * (y3 - y4);
        }
        return y4;
    }
};

// ============================================================
//  SIDVoice — one polyphonic voice (3 OSC + filter + envelopes)
// ============================================================
struct SIDVoice {
    SIDOscillator osc1, osc2, osc3;
    ADSREnv       env1, env2, env3, ampEnv;
    SIDFilter     filter;

    int   note    = -1;
    int   age     = 0;      // voice allocation age counter
    bool  active  = false;
    float velocity = 1.0f;
    float sr      = 44100.0f;

    // Mod matrix destination offsets (applied each block)
    float modCutoff = 0.0f;
    float modRes    = 0.0f;
    float modPW1    = 0.0f;
    float modPW2    = 0.0f;
    float modAmp    = 0.0f;
    float modFine1  = 0.0f;
    float modFine2  = 0.0f;

    void noteOn(int midiNote, float vel, float sampleRate) {
        note = midiNote; velocity = vel; sr = sampleRate;
        active = true;
        filter.reset();
        env1.noteOn(); env2.noteOn(); env3.noteOn(); ampEnv.noteOn();
    }

    void noteOff() {
        env1.noteOff(); env2.noteOff(); env3.noteOff(); ampEnv.noteOff();
    }

    // Returns stereo pair {L, R} — identical pre-unison (unison spread done in PolyEngine)
    float processMono(float baseCutoff, float baseRes,
                      float keyTrack, float velSens) {
        if (!active) return 0.0f;

        const float e1 = env1.process();
        const float e2 = env2.process();
        const float e3 = env3.process();
        const float eA = ampEnv.process();

        if (ampEnv.isIdle()) { active = false; return 0.0f; }

        // MIDI → Hz for each osc (with semi + fine + mod)
        const float baseHz = 440.0f * std::pow(2.0f, (note - 69) / 12.0f);
        auto oscHz = [&](int semi, float fine, float extraFine) {
            return baseHz * std::pow(2.0f, (semi + (fine + extraFine) / 100.0f) / 12.0f);
        };

        osc1.setFrequency(oscHz(osc1.semi, osc1.fine, modFine1), sr);
        osc2.setFrequency(oscHz(osc2.semi, osc2.fine, modFine2), sr);
        osc3.setFrequency(oscHz(osc3.semi, osc3.fine, 0.0f),     sr);

        osc1.pw = std::clamp(osc1.pw + modPW1, 0.05f, 0.95f);
        osc2.pw = std::clamp(osc2.pw + modPW2, 0.05f, 0.95f);

        // OSC3 as ring-mod source for osc1+osc2 if in RING mode
        const float s3 = osc3.process() * e3 * osc3.volume;
        osc1.ringIn = s3;
        osc2.ringIn = s3;

        const float s1 = osc1.process() * e1 * osc1.volume;
        const float s2 = osc2.process() * e2 * osc2.volume;
        const float mixed = s1 + s2 + s3;

        // Filter: cutoff from param + keytrack + mod
        const float keyOff = (note - 60) * keyTrack * 50.0f;
        const float velOff = (velocity - 0.5f) * velSens * 5000.0f;
        filter.cutoff = std::clamp(baseCutoff + keyOff + velOff + modCutoff * 12000.0f,
                                   20.0f, 18000.0f);
        filter.res = std::clamp(baseRes + modRes, 0.0f, 0.99f);

        const float filtered = filter.process(mixed);

        // Amp: envelope × velocity × mod
        const float ampMod = 1.0f + std::clamp(modAmp, -0.9f, 1.0f);
        return filtered * eA * velocity * ampMod;
    }
};

// ============================================================
//  LFOEngine — shapes: Sine, Tri, Saw, RevSaw, Square, S&H
// ============================================================
struct LFOEngine {
    enum class Shape : int { SINE = 0, TRI, SAW, REVSAW, SQUARE, SHOLD };

    Shape shape  = Shape::SINE;
    float phase  = 0.0f;
    float rate   = 1.0f;     // Hz (free) or beat-division (sync)
    float sr     = 44100.0f;
    bool  syncOn = false;
    bool  retrig = true;
    float holdVal = 0.0f;

    void retrigger() { if (retrig) phase = 0.0f; }

    float process(float hostBPM) {
        const float freq = syncOn ? (hostBPM * rate / 60.0f) : rate;
        const float inc  = freq / sr;
        phase += inc;
        if (phase >= 1.0f) {
            phase -= 1.0f;
            if (shape == Shape::SHOLD)
                holdVal = (float(rand()) / RAND_MAX) * 2.0f - 1.0f;
        }

        switch (shape) {
        case Shape::SINE:   return std::sin(phase * 6.28318530f);
        case Shape::TRI:    return (phase < 0.5f) ? (phase*4.0f-1.0f) : (3.0f-phase*4.0f);
        case Shape::SAW:    return phase * 2.0f - 1.0f;
        case Shape::REVSAW: return 1.0f - phase * 2.0f;
        case Shape::SQUARE: return phase < 0.5f ? 1.0f : -1.0f;
        case Shape::SHOLD:  return holdVal;
        }
        return 0.0f;
    }
};

// ============================================================
//  Freeverb Schroeder reverb (8 comb + 4 allpass, stereo)
// ============================================================
struct SIDReverb {
    static constexpr int kCombL[]    = {1116,1188,1277,1356,1422,1491,1557,1617};
    static constexpr int kCombR[]    = {1139,1211,1300,1379,1445,1514,1580,1640};
    static constexpr int kAllpassL[] = {556, 441, 341, 225};
    static constexpr int kAllpassR[] = {579, 464, 364, 248};

    static constexpr int kMaxComb    = 1700;
    static constexpr int kMaxAllpass = 600;

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

    void reset() {
        for (auto& b : combL) for (auto& s : b) s = 0.0f;
        for (auto& b : combR) for (auto& s : b) s = 0.0f;
        for (auto& b : allL)  for (auto& s : b) s = 0.0f;
        for (auto& b : allR)  for (auto& s : b) s = 0.0f;
    }

    void process(float& L, float& R) {
        const float feedback = roomSize * 0.28f + 0.7f;
        const float d = damp;

        float outL = 0.0f, outR = 0.0f;

        // 8 comb filters
        for (int i = 0; i < 8; ++i) {
            const int lenL = kCombL[i], lenR = kCombR[i];
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
            const int lenL = kAllpassL[i], lenR = kAllpassR[i];
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
        const float lfo2 = std::sin(lfoPhase * 6.28318f + 3.14159f);

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

    std::vector<int> held;    // held MIDI notes
    int   currentNote  = -1;
    int   octOffset    = 0;

    void addNote(int note) {
        if (std::find(held.begin(), held.end(), note) == held.end())
            held.push_back(note);
    }
    void removeNote(int note) {
        held.erase(std::remove(held.begin(), held.end(), note), held.end());
    }
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

    const juce::String getName() const override { return "SID Trance Machine"; }
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

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Voice management
    int allocateVoice(int midiNote);
    void handleNoteOn(int ch, int note, float vel);
    void handleNoteOff(int ch, int note);
    void applyModMatrix(SIDVoice& v, float lfo1v, float lfo2v, float vel, float mw);
    void processArpEvents(juce::MidiBuffer& out, int numSamples, float bpm);
    float analogGlow(float x, float amount);

    juce::AudioProcessorValueTreeState apvts;

    // DSP instances
    static constexpr int kMaxVoices  = 16;
    static constexpr int kMaxUnison  = 7;

    std::array<SIDVoice, kMaxVoices> voices;
    int voiceCounter = 0;  // age counter for voice steal

    LFOEngine   lfo1, lfo2;
    ArpSequencer arp;
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

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SIDTranceAudioProcessor)
};
