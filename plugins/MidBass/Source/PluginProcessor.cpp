#include "PluginProcessor.h"
#if ! MIDBASS_HEADLESS
 #include "PluginEditor.h"
#endif

MidBassAudioProcessor::MidBassAudioProcessor()
    : AudioProcessor (BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "MidBass", mb::createParameterLayout())
{
}

void MidBassAudioProcessor::prepareToPlay (double, int)
{
    // Phase 1+: voice engine prepare goes here.
}

bool MidBassAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void MidBassAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    keyboardState.processNextMidiBuffer (midi, 0, buffer.getNumSamples(), true);
    buffer.clear();   // Phase 0: no voice engine yet — output silence
}

bool MidBassAudioProcessor::hasEditor() const
{
   #if MIDBASS_HEADLESS
    return false;
   #else
    return true;
   #endif
}

juce::AudioProcessorEditor* MidBassAudioProcessor::createEditor()
{
   #if MIDBASS_HEADLESS
    return nullptr;
   #else
    return new MidBassAudioProcessorEditor (*this);
   #endif
}

void MidBassAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void MidBassAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MidBassAudioProcessor();
}
