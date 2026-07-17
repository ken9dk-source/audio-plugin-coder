// VazV2PAudit — no-discard audit of the .v2p preset parser.
// ---------------------------------------------------------------------------------------------
// Mirrors parseV2P (PluginProcessor.cpp) with a LABELED cursor: EVERY consumed byte is tagged
// either PARAM(field) or WHITELIST(reason). The mirror is validated against the REAL parser by
// asserting it consumes the EXACT same byte range (proc.debugV2PConsumedEnd) on every factory
// .v2p file. So if a read is ever added to / removed from parseV2P without a matching (labeled)
// read here, the end-offsets diverge and the test fails — making the "field read and silently
// discarded" bug class (voice_count, p2f0 were both silently dropped once) impossible to
// reintroduce unnoticed. There is NO plain unlabeled read: AuditCursor's read methods REQUIRE a
// (kind,label) argument, so a discard cannot be written without a whitelist reason.
//   usage:  VazV2PAudit [vaz-2010-folder]   (default = the standard Steinberg install path)
#include "PluginProcessor.h"
#include "ParameterIDs.hpp"     // mixer-source encoding lock (mix1/2/3_src)
#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <map>

enum Kind { PARAM, WHITELIST };
struct Read { int off, size; Kind kind; std::string label; };

// Cursor that mirrors V2PCursor's stream primitives but records + labels every read.
struct AuditCursor
{
    const juce::uint8* d; int n; int pos;
    std::vector<Read> reads;

    int u32 (Kind k, const char* label)
    { int o = pos; int v = (pos + 4 <= n) ? (d[pos] | (d[pos+1]<<8) | (d[pos+2]<<16) | (d[pos+3]<<24)) : 0;
      pos += 4; reads.push_back ({ o, 4, k, label }); return v; }
    int byte (Kind k, const char* label)
    { int o = pos; int v = (pos >= 0 && pos < n) ? (int) d[pos] : 0; pos += 1; reads.push_back ({ o, 1, k, label }); return v; }
    int modsrc (int ver, Kind k, const char* label) { int v = u32 (k, label); if (ver < 200 && v > 6) v += 1; return v; }
    void strsample (const char* label)   // v<0x69/0x6a name path: 2 bytes + u32 length + skip
    { byte (WHITELIST, label); byte (WHITELIST, label); int o = pos; int ln = u32 (WHITELIST, label);
      pos += ln; if (ln > 0) reads.push_back ({ o + 4, ln, WHITELIST, label }); }
    void skipMsmp (const char* label)    // "MSmp" multisample block: 8-byte header + payload
    { if (pos + 8 <= n && d[pos]=='M' && d[pos+1]=='S' && d[pos+2]=='m' && d[pos+3]=='p')
      { int o = pos; int sz = 8 + (d[pos+4] | (d[pos+5]<<8) | (d[pos+6]<<16) | (d[pos+7]<<24));
        pos += sz; reads.push_back ({ o, sz, WHITELIST, label }); } }
};

static int findTagLocal (const juce::uint8* d, int n, const char* t)
{
    for (int i = 0; i + 4 <= n; ++i)
        if (d[i]==t[0] && d[i+1]==t[1] && d[i+2]==t[2] && d[i+3]==t[3]) return i;
    return -1;
}

// EXACT mirror of parseV2P (PluginProcessor.cpp:386-467). Keep line-aligned with it. Every read is
// labeled PARAM(target) or WHITELIST(why it is deliberately not mapped to a clone parameter).
static AuditCursor auditParse (const juce::uint8* d, int n, int prst)
{
    const int v = d[prst+8] | (d[prst+9]<<8) | (d[prst+10]<<16) | (d[prst+11]<<24);
    AuditCursor c { d, n, prst + 12, {} };

    if (v >= 0x67) c.byte (PARAM, "mono/preset_enable -> voice_mode");
    if (v >= 0x6d) c.u32  (WHITELIST, "voice_count: VAZ per-preset voice cap; clone uses a global Voices param, not per-preset [FLAGGED in matrix]");
    if (v >= 0xc9) c.byte (WHITELIST, "mono dup byte (v2.0): superseded by the preset_enable slot at +12");
    if (v >= 0xc9) c.u32  (WHITELIST, "+0x94 LFO1 v2.0 field: purpose unconfirmed, not modeled [FLAGGED]");
    c.u32 (PARAM, "lfo1rate -> lfo_rate");
    c.u32 (PARAM, "lfo1wave -> lfo_wave");
    c.u32 (PARAM, "lfo1shape -> lfo_shape"); c.byte (PARAM, "lfo1trig -> lfo_trig");
    if (v >= 0xc9) c.byte (WHITELIST, "+0xded84 LFO1 v2.0 flag: not modeled [FLAGGED]");
    if (v >= 0xc9) c.u32  (WHITELIST, "+0xe0 LFO1 v2.0 field: not modeled [FLAGGED]");
    c.u32 (PARAM, "lfo2rate -> lfo2_rate");
    if (v >= 200) { c.modsrc (v, WHITELIST, "LFO2 trig-mod src: clone does not model LFO2 trig modulation [FLAGGED]");
                    c.u32 (WHITELIST, "LFO2 trig-mod depth [FLAGGED]"); }
    c.byte (PARAM, "lfo2trig -> lfo2_trig");
    if (v >= 200) c.u32 (PARAM, "lfo2mode -> lfo2_wave");
    else          c.byte (PARAM, "lfo2mode (v1xx S&H bool) -> lfo2_wave");
    c.u32 (PARAM, "lfo2delay -> lfo2_delay"); c.u32 (PARAM, "lfo3sel -> lfo3_rate"); c.byte (PARAM, "lfo3wav -> lfo3_wave");
    // env1
    if (v < 0x6b) { c.u32 (PARAM,"e1a"); c.u32 (PARAM,"e1d"); c.u32 (PARAM,"e1s"); c.u32 (PARAM,"e1r");
                    c.byte (WHITELIST,"env1 pre-0x6b trailing flag byte");
                    c.byte (PARAM,"env1 linear/exp flag -> rate-table offset adjust (affects e1a/d/r)"); }
    else          { c.u32 (PARAM,"e1a"); c.u32 (PARAM,"e1d"); c.u32 (PARAM,"e1s"); c.u32 (PARAM,"e1r");
                    c.byte (WHITELIST,"env1 (>=0x6b) trailing flag byte"); }
    c.byte (PARAM, "e1mode -> e1_reset/e1_cycle/e1_curve");
    if (v >= 0x6b) c.byte (WHITELIST, "env1 (>=0x6b) extra flag byte");
    if (v >= 0xca) c.byte (WHITELIST, "env1 (>=0xca) extra flag byte");
    // env2
    if (v < 0x6c) { c.u32 (PARAM,"e2a"); c.u32 (PARAM,"e2d"); c.u32 (PARAM,"e2s"); c.u32 (PARAM,"e2r");
                    c.byte (WHITELIST,"env2 pre-0x6c trailing flag byte");
                    c.byte (PARAM,"env2 linear/exp flag -> rate-table offset adjust"); }
    else          { c.u32 (PARAM,"e2a"); c.u32 (PARAM,"e2d"); c.u32 (PARAM,"e2s"); c.u32 (PARAM,"e2r");
                    c.byte (WHITELIST,"env2 (>=0x6c) trailing flag byte"); }
    c.byte (WHITELIST, "env2 df140 flag byte");
    if (v >= 0x6c) c.byte (PARAM, "e2mode -> e2_reset/e2_cycle/e2_curve");
    if (v >= 0xca) c.byte (WHITELIST, "env2 (>=0xca) extra flag byte");
    if (v >= 200) { c.modsrc (v, PARAM, "e2modsrc -> e2_mod_src"); c.u32 (PARAM, "e2modamt -> e2_mod_amt"); c.u32 (PARAM, "e2moddest -> e2_dest"); }
    c.modsrc (v, PARAM, "ma1in -> ma1_in_src");
    if (v >= 200) c.byte (PARAM, "ma1sq -> ma1_sq");
    c.modsrc (v, PARAM, "ma1amsrc -> ma1_am_src"); c.u32 (PARAM, "ma1amamt -> ma1_am_amt");
    if (v >= 200) c.modsrc (v, PARAM, "ma2in -> ma2_in_src");
    if (v >= 200) c.modsrc (v, PARAM, "ma2amsrc -> ma2_am_src");
    // osc1
    c.u32 (PARAM, "o1tune -> o1_oct/o1_coarse/o1_fine"); c.u32 (PARAM, "o1wave -> o1_wave"); c.u32 (PARAM, "o1shape -> o1_shape");
    if (v >= 200) c.byte (WHITELIST, "osc1 df430 v2.0 flag byte");
    c.modsrc (v, PARAM, "o1fm1s -> o1_fm_src"); c.u32 (PARAM, "o1fm1d -> o1_fm_amt");
    c.modsrc (v, PARAM, "o1fm2s -> o1_fm2_src"); c.u32 (PARAM, "o1fm2d -> o1_fm2_amt");
    c.modsrc (v, PARAM, "o1pwms -> o1_ws_src"); c.u32 (PARAM, "o1pwmd -> o1_ws_amt");
    if (v < 0x69) c.strsample ("osc1 sample name/data (MSmp1): sample-osc payload, clone loads samples separately");
    else { c.skipMsmp ("osc1 sample block (MSmp1): sample-osc payload"); c.byte (WHITELIST, "osc1 sample trailing flag byte (One-Shot/No-Trigger): clone sample osc does not model these"); }
    // osc2
    c.u32 (PARAM, "o2tune -> o2_oct/o2_coarse/o2_fine"); c.u32 (PARAM, "o2wave -> o2_wave"); c.byte (PARAM, "o1sync -> osc2_sync"); c.u32 (PARAM, "o2shape -> o2_shape");
    c.modsrc (v, PARAM, "o2fm1s -> o2_fm_src"); c.u32 (PARAM, "o2fm1d -> o2_fm_amt");
    c.modsrc (v, PARAM, "o2fm2s -> o2_fm2_src"); c.u32 (PARAM, "o2fm2d -> o2_fm2_amt");
    c.modsrc (v, PARAM, "o2pwms -> o2_ws_src"); c.u32 (PARAM, "o2pwmd -> o2_ws_amt");
    if (v < 0x6a) c.strsample ("osc2 sample name/data (MSmp2): sample-osc payload");
    else { c.skipMsmp ("osc2 sample block (MSmp2): sample-osc payload"); c.byte (WHITELIST, "osc2 sample trailing flag byte (One-Shot/No-Trigger)"); }
    // filter / mixer / output
    if (v >= 200) c.u32 (PARAM, "mix1src -> mix1_src");
    c.u32 (PARAM, "o1level -> mix1_level"); c.byte (PARAM, "mix1post -> mix1_post");
    if (v >= 200) c.u32 (PARAM, "mix2src -> mix2_src");
    c.u32 (PARAM, "o2level -> mix2_level"); c.byte (PARAM, "mix2post -> mix2_post");
    c.u32 (PARAM, "mix3src -> mix3_src"); c.u32 (PARAM, "noise -> noise_level"); c.byte (PARAM, "mix3post -> mix3_post");
    c.u32 (PARAM, "filterMode -> filter_mode"); c.byte (WHITELIST, "filter trailing flag byte (after filterMode)");
    c.u32 (PARAM, "cutoff -> cutoff"); c.u32 (PARAM, "reso -> resonance"); c.u32 (PARAM, "bandwidth -> flt_aux");
    if (v >= 200) c.u32 (PARAM, "hpCut -> hp_cutoff");
    c.modsrc (v, PARAM, "fcut1s -> cut_mod1_src"); c.u32 (PARAM, "fcut1d -> filt_env_amt");
    c.modsrc (v, PARAM, "fcut2s -> cut_mod2_src"); c.u32 (PARAM, "fcut2d -> cut_mod2_amt");
    c.modsrc (v, PARAM, "fcut3s -> cut_mod3_src"); c.u32 (PARAM, "fcut3d -> cut_mod3_amt");
    c.modsrc (v, PARAM, "fresS -> res_mod_src"); c.u32 (PARAM, "fresD -> res_mod_amt");
    c.modsrc (v, PARAM, "am1s -> amp_mod_src"); c.u32 (PARAM, "am1d -> amp_mod_amt");
    c.modsrc (v, PARAM, "am2s -> amp_mod2_src"); c.u32 (PARAM, "am2d -> amp_mod2_amt");
    if (v >= 200) { c.modsrc (v, PARAM, "am3s -> pan_mod_src"); c.u32 (PARAM, "am3d -> pan_mod_amt"); }
    c.u32 (PARAM, "overdrive -> overdrive");
    if (v >= 0x65) c.modsrc (v, WHITELIST, "e04f4 extra mod-src slot: not modeled [FLAGGED]");
    if (v >= 0x65) c.u32  (WHITELIST, "e0504 depth for the e04f4 slot [FLAGGED]");
    c.u32 (PARAM, "voiceMode -> voice_mode"); c.u32 (WHITELIST, "e05f0 glide/legato flag: not modeled [FLAGGED]"); c.byte (WHITELIST, "e0600 flag: not modeled [FLAGGED]");
    c.u32 (PARAM, "bendRange -> bend_range");
    if (v >= 200) c.u32 (PARAM, "uniVoices -> uni_voices");
    c.u32 (PARAM, "uniDetune -> uni_detune");
    if (v >= 200) c.u32 (PARAM, "polyDetune -> poly_detune");
    c.u32 (PARAM, "portamento -> portamento");
    c.byte (PARAM, "portaAuto -> porta_auto");   // byte @portamento+4 — decoded by VAZ-preset byte-diff
    c.byte (PARAM, "portaExp  -> porta_exp");    // byte @portamento+5
    return c;
}

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI init;
    const juce::String dir = argc > 1 ? juce::String (argv[1])
        : juce::String ("C:/Program Files (x86)/Steinberg/Vstplugins/VAZ Synths/VAZ 2010");
    auto files = juce::File (dir).findChildFiles (juce::File::findFiles, true, "*.v2p");
    std::cout << "=== VazV2PAudit: no-discard parser audit ===\n" << "folder: " << dir << "\nfound " << files.size() << " .v2p files\n\n";
    if (files.isEmpty()) { std::cout << "NO FILES — pass the VAZ 2010 folder as argv[1].\n"; return 2; }

    VAZCloneAudioProcessor proc;
    int nOK = 0, nFail = 0; long totalParam = 0, totalWl = 0;
    std::set<std::string> whitelist;
    std::map<int,int> byVersion;
    std::vector<std::string> failures;

    for (auto& f : files)
    {
        juce::MemoryBlock mb;
        if (! f.loadFileAsData (mb)) { failures.push_back (f.getFileName().toStdString() + ": read error"); ++nFail; continue; }
        const auto* d = (const juce::uint8*) mb.getData(); const int n = (int) mb.getSize();
        const int prst = findTagLocal (d, n, "PRST");
        if (prst < 0 || prst + 12 > n) { failures.push_back (f.getFileName().toStdString() + ": no PRST tag"); ++nFail; continue; }
        const int ver = d[prst+8] | (d[prst+9]<<8) | (d[prst+10]<<16) | (d[prst+11]<<24);

        auto ac = auditParse (d, n, prst);
        const int realEnd = proc.debugV2PConsumedEnd (mb);
        ++byVersion[ver];

        // DRIFT CHECK: the labeled mirror must consume the exact same range as the real parseV2P.
        if (ac.pos != realEnd)
        {
            failures.push_back (f.getFileName().toStdString() + ": MIRROR DRIFT (ver " + std::to_string (ver)
                + ") audit consumed to " + std::to_string (ac.pos) + " but parseV2P to " + std::to_string (realEnd)
                + " — a read was added/removed in parseV2P without a labeled read in v2p_audit_main.cpp");
            ++nFail; continue;
        }
        for (auto& r : ac.reads) { if (r.kind == PARAM) ++totalParam; else { ++totalWl; whitelist.insert (r.label); } }
        ++nOK;
    }

    std::cout << "files audited OK (mirror == parseV2P): " << nOK << " / " << files.size() << "\n";
    std::cout << "reads: " << totalParam << " -> param,  " << totalWl << " -> whitelist\n";
    std::cout << "versions seen: "; for (auto& kv : byVersion) std::cout << "v" << kv.first << "x" << kv.second << " "; std::cout << "\n\n";

    std::cout << "--- WHITELIST (bytes deliberately consumed but NOT mapped to a clone param) ---\n";
    for (auto& w : whitelist) std::cout << "  * " << w << "\n";
    std::cout << "  (" << whitelist.size() << " distinct reasons; entries tagged [FLAGGED] are candidates for the parity matrix)\n\n";

    // ── OSC WAVEFORM ENCODING LOCK ───────────────────────────────────────────────────────────────
    // VAZ's .v2p wave BYTE has a HOLE at 2: it encodes {0,1,3,4,5} while the GUI/engine use {0,1,2,3,4}
    //     byte 0→Saw, 1→Pulse, 3→Multi-Saw, 4→Wavetable("Sample"), 5→Ext     (byte 2 unreachable/legacy)
    // Ground truth (three independent sources, all agreeing):
    //   · REAL VAZ-saved presets, byte @0xD3 — checked in as the fixtures used below
    //   · VAZ's own FL hint bar: "Oscillator 1:Waveform: 0..4" with the shape label changing
    //     Waveshape / Pulsewidth / Detune / Wavetable Position / Position
    //   · the render dispatch on [+0x1ac]: <2 saw/pulse (0x4DCDDA, ==1 → pulse), ==2 multi-saw 4-phase
    //     detune loop (0x4DCAA5), >2 wavetable 4-pt Catmull-Rom (0x4DCC75), ==4 ext (vaz_big.c:574)
    // The loader previously mapped the byte STRAIGHT to the GUI index, so real presets mis-loaded:
    // Multi-Saw(3) → "Sample" → sine fallback (its detune could never sound), Wavetable(4) → "Ext";
    // Ext(5) was correct only by luck of the jlimit clamp. This lock exists so that cannot return.
    std::cout << "--- OSC WAVEFORM ENCODING LOCK (.v2p byte {0,1,3,4,5} -> index {0,1,2,3,4}) ---\n";
    {
        for (auto* id : { ParameterIDs::o1_wave, ParameterIDs::o2_wave })
        {
            auto* pc = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (id));
            if (pc == nullptr) { failures.push_back (std::string (id) + ": not an AudioParameterChoice"); ++nFail; continue; }
            const bool ok = pc->choices.size() == 5
                         && pc->choices[0] == "Sawtooth" && pc->choices[1] == "Pulse" && pc->choices[2] == "Multi-Saw"
                         && pc->choices[3] == "Sample"
                         && (pc->choices[4] == "Ext" || pc->choices[4] == "Sync");   // OSC2 has Sync where OSC1 has Ext
            std::cout << (ok ? "  [PASS] " : "  [FAIL] ") << id << " = " << pc->choices.joinIntoString ("/") << "\n";
            if (! ok) { failures.push_back (std::string (id) + ": waveform list != VAZ order"); ++nFail; }
        }
        // End-to-end: each REAL VAZ preset must land on the waveform VAZ actually shows — not sine/Ext.
        struct Fx { const char* file; int wantIdx; const char* wantName; int byte; };
        const Fx fx[] = { { "vaz_osc1_saw.v2p",       0, "Sawtooth",  0 },
                          { "vaz_osc1_pulse.v2p",     1, "Pulse",     1 },
                          { "vaz_osc1_multisaw.v2p",  2, "Multi-Saw", 3 },   // WAS loading as "Sample" -> sine
                          { "vaz_osc1_wavetable.v2p", 3, "Sample",    4 },   // WAS loading as "Ext"
                          { "vaz_osc1_ext.v2p",       4, "Ext",       5 } };
        auto wDir = juce::File::getCurrentWorkingDirectory().getChildFile ("plugins/VAZClone/tests/fixtures");
        if (! wDir.isDirectory()) wDir = juce::File (__FILE__).getParentDirectory().getSiblingFile ("tests").getChildFile ("fixtures");
        for (auto& f : fx)
        {
            auto file = wDir.getChildFile (f.file);
            juce::MemoryBlock mb;
            if (! file.existsAsFile() || ! file.loadFileAsData (mb))
            { std::cout << "  [FAIL] fixture missing: " << file.getFullPathName() << "\n";
              failures.push_back (std::string (f.file) + ": fixture missing"); ++nFail; continue; }
            // the fixture must really carry the byte we claim (guards a silently re-saved/wrong preset)
            const bool byteOk = mb.getSize() > 0xD3 && (int) ((const uint8_t*) mb.getData())[0xD3] == f.byte;
            if (! byteOk)
            { std::cout << "  [FAIL] " << f.file << ": byte@0xD3 = "
                        << (mb.getSize() > 0xD3 ? (int) ((const uint8_t*) mb.getData())[0xD3] : -1)
                        << ", expected " << f.byte << "\n";
              failures.push_back (std::string (f.file) + ": wave byte changed"); ++nFail; continue; }
            if (! proc.loadV2P (mb))
            { std::cout << "  [FAIL] loadV2P failed: " << f.file << "\n"; failures.push_back (std::string (f.file) + ": loadV2P failed"); ++nFail; continue; }
            auto* pc = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (ParameterIDs::o1_wave));
            const int got = pc != nullptr ? pc->getIndex() : -1;
            const bool ok = got == f.wantIdx;
            std::cout << (ok ? "  [PASS] " : "  [FAIL] ") << f.file << " (byte " << f.byte << ") -> o1_wave=" << got
                      << " (" << (pc != nullptr ? pc->getCurrentValueAsText() : juce::String ("?")) << "), want "
                      << f.wantIdx << " (" << f.wantName << ")\n";
            if (! ok) { failures.push_back (std::string (f.file) + ": wrong waveform after load"); ++nFail; }
        }
    }

    // ── MIXER-SOURCE ENCODING LOCK ───────────────────────────────────────────────────────────────
    // VAZ uses ONE mixer-source popup whose item 0 is relabelled per channel (Osc1/Osc2/Osc3), so all
    // three channels share the SAME encoding:
    //     0 = own oscillator, 1 = Ring Modulator, 2 = Noise, 3 = External Input, 4/5 = Mod Amplifier 1/2
    // Ground truth: Core.dll popup captions @0x183812 (Oscillator 3 / Ring Modulator / Noise / External
    // Input / Mod Amplifier 1 / Mod Amplifier 2), corroborated by two REAL VAZ-saved presets whose byte
    // diff is exactly mix3src 0 (Osc3) vs 1 (RingMod) — checked in as fixtures below.
    // ch3 previously listed Noise first (0=Noise, 1=Osc3): a real-VAZ Osc3 patch then loaded as "Noise"
    // and Osc3 never sounded. This lock exists so that off-by-one cannot silently return.
    std::cout << "--- MIXER-SOURCE ENCODING LOCK (VAZ: 0=own osc, 1=RingMod, 2=Noise, 3=Ext, 4/5=ModAmp) ---\n";
    {
        struct Expect { const char* id; const char* first; };
        const Expect exp[] = { { ParameterIDs::mix1_src, "Oscillator 1" },
                               { ParameterIDs::mix2_src, "Oscillator 2" },
                               { ParameterIDs::mix3_src, "Oscillator 3" } };
        const char* tail[] = { "Ring Modulator", "Noise", "External Input", "Mod Amplifier 1", "Mod Amplifier 2" };
        for (auto& e : exp)
        {
            auto* pc = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (e.id));
            if (pc == nullptr) { failures.push_back (std::string (e.id) + ": not an AudioParameterChoice"); ++nFail; continue; }
            const auto& ch = pc->choices;
            bool ok = ch.size() == 6 && ch[0] == juce::String (e.first);
            for (int i = 0; ok && i < 5; ++i) ok = ch[i + 1] == juce::String (tail[i]);
            std::cout << (ok ? "  [PASS] " : "  [FAIL] ") << e.id << " = " << ch.joinIntoString ("/") << "\n";
            if (! ok) { failures.push_back (std::string (e.id) + ": choice order != VAZ popup order"); ++nFail; }
        }
    }
    // ── PORTAMENTO AUTO/EXP PRESET LOCK ──────────────────────────────────────────────────────────
    // "Portamento Auto"/"Portamento Exp" (Performance section of the property descriptor table) are the
    // two bytes right after Portamento Time. Decoded by byte-diffing VAZ-saved presets differing by ONE
    // button each: porta_auto_on flips exactly portamento+4, porta_exp_on flips exactly portamento+5.
    // (They are NOT the whitelisted e05f0/e0600 — that earlier guess was wrong.) The clone already had
    // param+GUI+DSP for both but never read them from a preset; this locks the parsing.
    std::cout << "--- PORTAMENTO AUTO/EXP PRESET LOCK (bytes @portamento+4 / +5) ---\n";
    {
        struct PFx { const char* file; bool wantAuto, wantExp; };
        const PFx pfx[] = { { "vaz_porta_base.v2p",    false, false },
                            { "vaz_porta_auto_on.v2p", true,  false },
                            { "vaz_porta_exp_on.v2p",  false, true  } };
        auto fdir = juce::File::getCurrentWorkingDirectory().getChildFile ("plugins/VAZClone/tests/fixtures");
        if (! fdir.isDirectory()) fdir = juce::File (__FILE__).getParentDirectory().getSiblingFile ("tests").getChildFile ("fixtures");
        for (auto& f : pfx)
        {
            juce::MemoryBlock mb;
            auto file = fdir.getChildFile (f.file);
            if (! file.existsAsFile() || ! file.loadFileAsData (mb) || ! proc.loadV2P (mb))
            { std::cout << "  [FAIL] " << f.file << ": missing/unloadable\n"; failures.push_back (std::string (f.file) + ": missing/unloadable"); ++nFail; continue; }
            auto* pa = dynamic_cast<juce::AudioParameterBool*> (proc.apvts.getParameter (ParameterIDs::porta_auto));
            auto* pe = dynamic_cast<juce::AudioParameterBool*> (proc.apvts.getParameter (ParameterIDs::porta_exp));
            const bool gotA = pa != nullptr && pa->get(), gotE = pe != nullptr && pe->get();
            const bool ok = gotA == f.wantAuto && gotE == f.wantExp;
            std::cout << (ok ? "  [PASS] " : "  [FAIL] ") << f.file << " -> auto=" << (gotA ? "on" : "off")
                      << " exp=" << (gotE ? "on" : "off") << ", want auto=" << (f.wantAuto ? "on" : "off")
                      << " exp=" << (f.wantExp ? "on" : "off") << "\n";
            if (! ok) { failures.push_back (std::string (f.file) + ": porta auto/exp not parsed from the preset"); ++nFail; }
        }
    }

    // End-to-end: the two REAL VAZ presets must land on the right source (not silence/Noise).
    {
        struct Fx { const char* file; int wantIdx; const char* wantName; };
        const Fx fx[] = { { "vaz_mix3_osc3.v2p",    0, "Oscillator 3"   },
                          { "vaz_mix3_ringmod.v2p", 1, "Ring Modulator" } };
        auto fixDir = juce::File::getCurrentWorkingDirectory().getChildFile ("plugins/VAZClone/tests/fixtures");
        if (! fixDir.isDirectory()) fixDir = juce::File (__FILE__).getParentDirectory().getSiblingFile ("tests").getChildFile ("fixtures");
        for (auto& f : fx)
        {
            auto file = fixDir.getChildFile (f.file);
            juce::MemoryBlock mb;
            if (! file.existsAsFile() || ! file.loadFileAsData (mb))
            { std::cout << "  [FAIL] fixture missing: " << file.getFullPathName() << "\n";
              failures.push_back (std::string (f.file) + ": fixture missing"); ++nFail; continue; }
            if (! proc.loadV2P (mb))
            { std::cout << "  [FAIL] loadV2P failed: " << f.file << "\n"; failures.push_back (std::string (f.file) + ": loadV2P failed"); ++nFail; continue; }
            auto* pc = dynamic_cast<juce::AudioParameterChoice*> (proc.apvts.getParameter (ParameterIDs::mix3_src));
            const int got = pc != nullptr ? pc->getIndex() : -1;
            const bool ok = got == f.wantIdx;
            std::cout << (ok ? "  [PASS] " : "  [FAIL] ") << f.file << " -> mix3_src=" << got
                      << " (" << (pc && got >= 0 ? pc->choices[got] : juce::String ("?")) << "), want " << f.wantIdx << " (" << f.wantName << ")\n";
            if (! ok) { failures.push_back (std::string (f.file) + ": real VAZ preset maps to the wrong mixer source"); ++nFail; }
        }
    }
    std::cout << "\n";

    if (! failures.empty())
    {
        std::cout << "!!! " << failures.size() << " FAILURES:\n";
        for (auto& s : failures) std::cout << "  [FAIL] " << s << "\n";
    }
    std::cout << "\n=== " << nOK << " ok, " << nFail << " failed ===\n";
    return nFail == 0 ? 0 : 1;
}
