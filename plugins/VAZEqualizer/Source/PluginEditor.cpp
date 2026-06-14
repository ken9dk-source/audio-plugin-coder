#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"

VAZEqualizerAudioProcessorEditor::VAZEqualizerAudioProcessorEditor (VAZEqualizerAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    using Options = juce::WebBrowserComponent::Options;

    webView = std::make_unique<juce::WebBrowserComponent>(
        Options{}
           #if JUCE_WINDOWS
            .withBackend (Options::Backend::webview2)
            .withWinWebView2Options (Options::WinWebView2{}
                .withUserDataFolder (juce::File::getSpecialLocation (juce::File::SpecialLocationType::tempDirectory)))
           #endif
            .withNativeIntegrationEnabled()
            .withResourceProvider ([this] (const auto& url) { return getResource (url); })
            .withOptionsFrom (lowGainRelay)
            .withOptionsFrom (lowFreqRelay)
            .withOptionsFrom (lowQRelay)
            .withOptionsFrom (loMidGainRelay)
            .withOptionsFrom (loMidFreqRelay)
            .withOptionsFrom (loMidQRelay)
            .withOptionsFrom (hiMidGainRelay)
            .withOptionsFrom (hiMidFreqRelay)
            .withOptionsFrom (hiMidQRelay)
            .withOptionsFrom (highGainRelay)
            .withOptionsFrom (highFreqRelay)
            .withOptionsFrom (highQRelay));

    addAndMakeVisible (*webView);

    auto attach = [this] (const char* id, juce::WebSliderRelay& relay)
    {
        return std::make_unique<juce::WebSliderParameterAttachment>(
            *audioProcessor.apvts.getParameter (id), relay, nullptr);
    };
    lowGainAtt   = attach (ParameterIDs::low_gain,   lowGainRelay);
    lowFreqAtt   = attach (ParameterIDs::low_freq,   lowFreqRelay);
    lowQAtt      = attach (ParameterIDs::low_q,      lowQRelay);
    loMidGainAtt = attach (ParameterIDs::lomid_gain, loMidGainRelay);
    loMidFreqAtt = attach (ParameterIDs::lomid_freq, loMidFreqRelay);
    loMidQAtt    = attach (ParameterIDs::lomid_q,    loMidQRelay);
    hiMidGainAtt = attach (ParameterIDs::himid_gain, hiMidGainRelay);
    hiMidFreqAtt = attach (ParameterIDs::himid_freq, hiMidFreqRelay);
    hiMidQAtt    = attach (ParameterIDs::himid_q,    hiMidQRelay);
    highGainAtt  = attach (ParameterIDs::high_gain,  highGainRelay);
    highFreqAtt  = attach (ParameterIDs::high_freq,  highFreqRelay);
    highQAtt     = attach (ParameterIDs::high_q,     highQRelay);

    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());
    setSize (440, 205);
}

VAZEqualizerAudioProcessorEditor::~VAZEqualizerAudioProcessorEditor() {}

void VAZEqualizerAudioProcessorEditor::paint (juce::Graphics& g) { g.fillAll (juce::Colour (0xff0d1a2a)); }

void VAZEqualizerAudioProcessorEditor::resized()
{
    if (webView != nullptr) webView->setBounds (getLocalBounds());
}

const char* VAZEqualizerAudioProcessorEditor::getMimeForExtension (const juce::String& extension)
{
    static const std::unordered_map<juce::String, const char*> mimeMap =
    {
        { "html", "text/html" }, { "htm", "text/html" }, { "js", "text/javascript" },
        { "css", "text/css" }, { "json", "application/json" }, { "svg", "image/svg+xml" },
        { "png", "image/png" }, { "jpg", "image/jpeg" }, { "woff2", "font/woff2" }
    };
    if (auto it = mimeMap.find (extension.toLowerCase()); it != mimeMap.end())
        return it->second;
    return "text/plain";
}

std::optional<juce::WebBrowserComponent::Resource>
VAZEqualizerAudioProcessorEditor::getResource (const juce::String& url)
{
    auto path = url.fromFirstOccurrenceOf (
        juce::WebBrowserComponent::getResourceProviderRoot(), false, false);
    if (path.startsWithChar ('/')) path = path.substring (1);
    if (path.isEmpty()) path = "index.html";

    const char* data = nullptr;
    int size = 0;
    juce::String mime;

    if (path == "index.html")
    {
        data = BinaryData::index_html; size = BinaryData::index_htmlSize; mime = "text/html";
    }
    else if (path == "js/index.js")
    {
        data = BinaryData::index_js; size = BinaryData::index_jsSize; mime = "text/javascript";
    }
    else if (path == "js/juce/index.js")
    {
        data = BinaryData::index_js2; size = BinaryData::index_js2Size; mime = "text/javascript";
    }
    else if (path == "js/juce/check_native_interop.js")
    {
        data = BinaryData::check_native_interop_js; size = BinaryData::check_native_interop_jsSize; mime = "text/javascript";
    }

    if (data != nullptr && size > 0)
    {
        std::vector<std::byte> bytes ((size_t) size);
        std::memcpy (bytes.data(), data, (size_t) size);
        return juce::WebBrowserComponent::Resource { std::move (bytes), mime };
    }
    return std::nullopt;
}
