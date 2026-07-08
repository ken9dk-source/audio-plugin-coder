// Phase 9 audition renderer: writes every factory preset to a WAV playing the
// canonical use case — off-beat eighth mid-bass at 138 BPM (A2), 2 bars + tail.
// Final instrument sign-off is BY EAR on these files.
#include "PluginProcessor.h"
#include <juce_audio_formats/juce_audio_formats.h>

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceGui;
    const juce::File outDir = juce::File::getCurrentWorkingDirectory()
                                  .getChildFile (argc > 1 ? argv[1] : "Renders");
    outDir.createDirectory();

    constexpr double sr = 44100.0, bpm = 138.0;
    constexpr int note = 45;                                   // A2
    const double beat = 60.0 / bpm;
    const int totalSamples = (int) (sr * (beat * 8.0 + 1.5));  // 2 bars + tail

    for (int pr = 0; pr < mb::kNumFactoryPresets; ++pr)
    {
        MidBassAudioProcessor proc;
        proc.setPlayConfigDetails (0, 2, sr, 512);
        proc.prepareToPlay (sr, 512);
        proc.setCurrentProgram (pr);

        juce::AudioBuffer<float> out (2, totalSamples);
        out.clear();
        juce::AudioBuffer<float> blockBuf (2, 512);
        juce::MidiBuffer midi;

        // off-beat eighths: note on at beat k + 0.5, gate 55 % of an eighth
        for (int pos = 0; pos < totalSamples; pos += 512)
        {
            const int blockLen = juce::jmin (512, totalSamples - pos);
            for (int k = 0; k < 8; ++k)                              // 8 off-beats in 2 bars
            {
                const int on  = (int) ((k + 0.5) * beat * sr);       // the "and" of each beat
                const int off = on + (int) (0.55 * 0.5 * beat * sr); // gate: 55 % of an eighth
                if (on >= pos && on < pos + blockLen)
                    midi.addEvent (juce::MidiMessage::noteOn (1, note, (juce::uint8) 110), on - pos);
                if (off >= pos && off < pos + blockLen)
                    midi.addEvent (juce::MidiMessage::noteOff (1, note), off - pos);
            }
            blockBuf.clear();
            blockBuf.setSize (2, blockLen, false, false, true);
            proc.processBlock (blockBuf, midi);
            midi.clear();
            for (int ch = 0; ch < 2; ++ch)
                out.copyFrom (ch, pos, blockBuf, ch, 0, blockLen);
            blockBuf.setSize (2, 512, false, false, true);
        }

        const juce::String name = juce::String (pr) + "_" + juce::String (mb::kFactoryPresets[pr].name)
                                      .replaceCharacter (' ', '_').replaceCharacter ('&', 'n');
        const juce::File f = outDir.getChildFile (name + ".wav");
        f.deleteFile();
        juce::WavAudioFormat wav;
        std::unique_ptr<juce::AudioFormatWriter> writer (
            wav.createWriterFor (new juce::FileOutputStream (f), sr, 2, 24, {}, 0));
        if (writer == nullptr) { std::cout << "FAIL: writer " << name << "\n"; return 1; }
        writer->writeFromAudioSampleBuffer (out, 0, totalSamples);
        writer.reset();
        std::cout << "rendered: " << f.getFullPathName()
                  << "  peak " << out.getMagnitude (0, totalSamples) << "\n";
    }
    return 0;
}
