// VazRender — headless offline renderer for VAZClone (A/B testing).
// Instantiates the processor directly, loads a .v2p, renders a .mid -> WAV.
// No audio/MIDI devices, no GUI, deterministic.  Pairs with tools/abtest/.
//
//   VazRender <preset.v2p|-> <midi.mid> <out.wav> [seconds=6] [sampleRate=48000]
#include "PluginProcessor.h"
#include <juce_audio_formats/juce_audio_formats.h>
#include <iostream>

int main (int argc, char* argv[])
{
    if (argc < 4)
    {
        std::cout << "usage: VazRender <preset.v2p|-> <midi.mid> <out.wav> [seconds=6] [sampleRate=48000]\n";
        return 1;
    }
    juce::ScopedJuceInitialiser_GUI juceInit;   // JUCE singletons / message manager

    const juce::String preset = argv[1];
    const juce::File   midiF { juce::String (argv[2]) };
    const juce::File   outF  { juce::String (argv[3]) };
    const double seconds = argc > 4 ? juce::String (argv[4]).getDoubleValue() : 6.0;
    const double sr      = argc > 5 ? juce::String (argv[5]).getDoubleValue() : 48000.0;
    const int    block   = 512;

    VAZCloneAudioProcessor proc;

    if (preset != "-")
    {
        juce::MemoryBlock mb;
        if (juce::File (preset).loadFileAsData (mb) && proc.loadV2P (mb))
            std::cout << "loaded patch: " << preset << "\n";
        else
            std::cout << "WARN: could not load preset " << preset << "\n";
    }

    proc.setPlayConfigDetails (0, 2, sr, block);
    proc.prepareToPlay (sr, block);

    // ---- parse MIDI into a seconds-timed sequence ----
    juce::MidiFile mf;
    if (auto in = midiF.createInputStream()) mf.readFrom (*in);
    mf.convertTimestampTicksToSeconds();
    juce::MidiMessageSequence seq;
    for (int t = 0; t < mf.getNumTracks(); ++t)
        seq.addSequence (*mf.getTrack (t), 0.0);
    seq.updateMatchedPairs();

    // ---- render block by block ----
    const int total = (int) (seconds * sr);
    juce::AudioBuffer<float> out (2, total); out.clear();
    juce::AudioBuffer<float> buf (2, block);
    juce::MidiBuffer midi;
    int evt = 0;

    for (int pos = 0; pos < total; pos += block)
    {
        const int n = juce::jmin (block, total - pos);
        buf.setSize (2, n, false, false, true); buf.clear();
        midi.clear();
        const double t0 = pos / sr, t1 = (pos + n) / sr;
        while (evt < seq.getNumEvents() && seq.getEventTime (evt) < t1)
        {
            const double ts = seq.getEventTime (evt);
            if (ts >= t0)
                midi.addEvent (seq.getEventPointer (evt)->message, (int) ((ts - t0) * sr));
            ++evt;
        }
        proc.processBlock (buf, midi);
        for (int ch = 0; ch < 2; ++ch) out.copyFrom (ch, pos, buf, ch, 0, n);
    }

    // peak-normalize to -1 dBFS (the real-VAZ loopback captures are normalized too; A/B compares shape).
    const float natPeak = out.getMagnitude (0, total);
    if (natPeak > 1.0e-6f) out.applyGain (0.9f / natPeak);

    // ---- write 24-bit WAV ----
    outF.deleteFile();
    juce::WavAudioFormat wav;
    if (auto os = std::unique_ptr<juce::FileOutputStream> (outF.createOutputStream()))
    {
        if (auto* w = wav.createWriterFor (os.get(), sr, 2, 24, {}, 0))
        {
            os.release();
            std::unique_ptr<juce::AudioFormatWriter> writer (w);
            writer->writeFromAudioSampleBuffer (out, 0, total);
            std::cout << "wrote " << outF.getFullPathName() << "  (" << seconds
                      << "s, natural peak " << juce::Decibels::gainToDecibels (natPeak) << " dB -> normalized)\n";
            return 0;
        }
    }
    std::cout << "ERROR: could not open output WAV\n";
    return 2;
}
