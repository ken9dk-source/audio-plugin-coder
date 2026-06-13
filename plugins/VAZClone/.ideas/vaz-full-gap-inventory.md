# VAZ 2010 — complete gap inventory vs the clone (2026-06-13)

**Goal:** collect EVERYTHING still missing in VAZClone (research only, no fixes), so we have the full picture
before deciding what to build. Driven by the *authoritative* source this time — VAZ's **own manual**
(`VAZ2010.chm` → `tools/chm_out/`, every feature has a page) — then each candidate verified against the binary
(named reader `vaz_osc.c`, `Vaz2010Core.dll` strings) and the clone's code. Lesson from the Voices miss: the
patch named-reader only covers *patch properties* — performance / voice-allocation / global settings live
elsewhere, so I cross-checked the GUI/Core strings too.

---

## A. SYNTH gaps (excluding the sequencer)

### HIGH impact (audible, common)
1. **Poly Detune — a SECOND, independent detune.** Manual (New/Performance): *"separate detune parameters for
   both Poly and Unison play modes."* Poly Detune = spread between *different notes* (analogue-polysynth drift);
   Unison Detune = spread *within* one note's unison group. **Clone has ONE** param (`uni_detune`) scaled
   per-mode (Poly ×12c / Unison ×45c) — can't set them independently. The `.v2p` stores it: parseV2P reads
   `p2f4` (uni_detune) then **discards `p2f0` (+0x2f0) = the poly detune** (PluginProcessor.cpp:453). → add a
   `poly_detune` param + load p2f0 + use it for the Poly-mode spread.
2. **Oscillator 3 (LFO1 as an audio-rate, key-tracked 3rd oscillator).** Manual (Mixer/LFOs): selecting
   "Oscillator 3" as a mixer source switches **LFO 1 to audio rate, tracking the keyboard**; Rate sets pitch in
   quarter-tone steps, footage values **32'=48, 16'=96, 8'=144, 4'=192, 2'=240**. The clone HAS the mixer
   "Oscillator 3" option but **no DSP** (LFO1 stays an LFO) → mix3="Oscillator 3" is silent/wrong. Also the
   **Link** button should route Osc1's 1st mod input to Osc3 too.
3. **Oversample x2 (global).** Manual (New/Global): *"Oversample x2 option for even higher sound quality."*
   `Oversample` string present in Core.dll. The clone does NOT oversample the synth (only the filters are
   internally 2×). You asked for this earlier ("oversample men vent"). → a 2× oversample toggle around the
   whole voice render.

### MEDIUM
4. **Microtuning / Keymap.** `Tunings/` ships `cmajjust.tun / equal.tun / quart.tun / revequal.tun`; Core.dll
   has `Tuning` + `Keymap` (no "Scala"/".tun" literals → its own .tun format). The clone is hard 12-TET
   (`MidiMessage::getMidiNoteInHertz`). → a tuning-table loader + per-note frequency lookup.
5. **Full Sample Loader.** Manual (Sample Loader Dialog): multisample mapping (per-keygroup), **Drums** mapping
   (one sample/key), **loop modes Loop / One Shot / Wavetable**, wavetable-position playback, auto multisample
   from embedded tuning/loop points. The clone's Sample oscillator is **basic** (default sine, minimal). Big but
   niche (few factory patches embed samples). (Relatedly: the osc2 `One Shot`/`No Trigger` flags I noted before
   are this dialog's loop-mode bits.)
6. **Arpeggiator completeness.** Manual (Arp Mode): modes are **Up, Down, Up Down, Random 1, Random 2** (clone
   has Up/Down/Up&Down/**one** Random); **Trigger Free** (run the seq when arp is on); Range semantics
   (2 = +1 oct above, 3 = +1 above & below, 4 = +2 above & 1 below — clone likely just multiplies).

### LOW / niche
7. **Configurable MIDI CC → parameter mapping** (MIDI Controller Mappings dialog; Clear/Default, VAZ-Plus
   compatible). Clone has the *fixed default* (mod-wheel→Control A, CC11→Control B) which covers most patches.
8. **MIDI Program Change → patch switch** + the Patch List (24 F-key shortcuts, Load Sequence/Insert flags).
9. **Per-synth MIDI channel** ("Midi" box). Clone responds to all channels (DAW routes).
10. **Dynamic-mode exact behaviour:** VAZ frees a voice when **Env1 (the 1st Amp-mod source) is fully closed**;
    the clone approximates via JUCE's "voice inactive → freed". Functionally close.
11. **Note priority "Duo"** = lowest note → Osc1, highest → Osc2 (ARP Odyssey style). Clone has `note_priority`
    incl. Duo + `duoHighHz` — VERIFY it routes per-osc correctly (looks present).

---

## B. EFFECTS (the clone ships these as separate plugins)
- **Present + 2.0-complete:** Autopan, Chorus (has stereo width + L/R phase + sync), Decimator, Delay (sync),
  Flanger (phase), Phaser, Reverb. ✅
- **MISSING plugins:** **Compressor** (hard-knee peak follower: Threshold / Ratio / Attack / Release / Makeup +
  GR meter), **Equalizer** (4-band parametric, High/Low switchable to shelf/filter), **Oscilloscope** (visual).
  Plus **Sub Output** + **Plugin Chainer** (routing/host utilities — likely N/A for a single VST).

---

## C. SEQUENCER (the big deferred block — needs its own window/project)
Step sequencer w/ **Swing**, **Timebase** (per-step note value), **Tempo + Local**, **Free** (detach from
transport → run continuously as a mod source or MIDI-triggered), **Performance** + **Performance 2** ("phrase
sequencer": transpose the pattern across the keyboard), Pattern Mode, **Pattern Randomizer**, Song Editor.
Brings the **Accent / Sequencer A / Sequencer B** mod sources online (currently return 0 for ~21 patches).

---

## D. Confirmed NON-gaps (already covered — for completeness)
Oscillators (5 waves, Link, FM×2, PWM, WaveShape, Sync), Filter (all 22 modes bit-exact + the ~15 ms cutoff
smoother), Envelopes (bit-exact rate tables + Reset/Cycle/Curve/Multi + Env2 A/D/R mod), LFO 1/2/3 (8 waves,
Delay, tempo-sync, LFO2 rate-mod, LFO3 Tri/Sine), Mixer (6 sources + Post), Mod Amp 1/2 (+SQ), Amplifier (Pan
mod, Voice Number, exact overdrive cubic + DC-block), Voices (Dynamic + 1-32), UniV (1-32), Bend (1-24),
Portamento (Auto/Exp), the 22-source mod matrix.

---

## Suggested build order (when we start fixing)
1. **Poly Detune** (small, audible, data already in the .v2p) → **Osc3 audio-rate** (medium, completes the
   mixer) → **Oversample x2** (medium, quality + you wanted it).
2. Microtuning (.tun loader) · Arp Random2/Trigger-Free · the missing FX (Compressor, EQ, Oscilloscope).
3. Sample Loader (big/niche) · MIDI CC-map + Program Change.
4. Sequencer (separate window — the largest).
