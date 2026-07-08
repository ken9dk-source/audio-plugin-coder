// Offscreen GUI snapshot tool (Phase 8): builds the real editor, renders it to
// PNG for layout review, exercises the open/close/reopen lifecycle (condition
// g; the JUCE LeakDetector fires on exit if anything leaks), and prints the
// attachment count (condition d).
#include "PluginProcessor.h"
#include "PluginEditor.h"

int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceGui;

    MidBassAudioProcessor proc;
    proc.setPlayConfigDetails (0, 2, 44100.0, 512);
    proc.prepareToPlay (44100.0, 512);

    // lifecycle: open/close/reopen while "playing"
    juce::AudioBuffer<float> buf (2, 512);
    juce::MidiBuffer midi;
    midi.addEvent (juce::MidiMessage::noteOn (1, 45, (juce::uint8) 100), 0);
    proc.processBlock (buf, midi);
    midi.clear();

    for (int cycle = 0; cycle < 3; ++cycle)
    {
        std::unique_ptr<juce::AudioProcessorEditor> ed (proc.createEditor());
        if (ed == nullptr) { std::cout << "FAIL: no editor\n"; return 1; }
        for (int b = 0; b < 4; ++b) { proc.processBlock (buf, midi); }

        if (cycle == 2)
        {
            auto* mbe = dynamic_cast<MidBassAudioProcessorEditor*> (ed.get());
            std::cout << "attached parameters: " << (mbe != nullptr ? mbe->attachedParameterCountForTest() : -1)
                      << " / " << mb::pid::kExpectedParamCount << "\n";

            auto img = ed->createComponentSnapshot (ed->getLocalBounds(), false, 1.0f);
            const juce::File out = juce::File::getCurrentWorkingDirectory()
                                       .getChildFile (argc > 1 ? argv[1] : "midbass_gui.png");
            out.deleteFile();
            juce::FileOutputStream os (out);
            juce::PNGImageFormat png;
            if (! os.openedOk() || ! png.writeImageToStream (img, os)) { std::cout << "FAIL: png write\n"; return 1; }
            std::cout << "snapshot: " << out.getFullPathName() << " (" << img.getWidth() << "x" << img.getHeight() << ")\n";
        }
    }
    std::cout << "lifecycle: 3x open/close OK\n";
    return 0;
}
