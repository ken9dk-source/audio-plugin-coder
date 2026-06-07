#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"

VAZDelayAudioProcessorEditor::VAZDelayAudioProcessorEditor (VAZDelayAudioProcessor& p)
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
            .withNativeFunction (juce::Identifier ("resetParam"),
                [this] (const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion complete)
                {
                    if (args.size() > 0)
                        if (auto* p = audioProcessor.apvts.getParameter (args[0].toString()))
                            { p->beginChangeGesture(); p->setValueNotifyingHost (p->getDefaultValue()); p->endChangeGesture(); }
                    complete (juce::var());
                })
            .withOptionsFrom (modeRelay)
            .withOptionsFrom (noteLRelay)
            .withOptionsFrom (noteRRelay)
            .withOptionsFrom (linkRelay)
            .withOptionsFrom (syncRelay)
            .withOptionsFrom (dlLRelay).withOptionsFrom (fbLRelay).withOptionsFrom (tnLRelay)
            .withOptionsFrom (wLRelay).withOptionsFrom (dryLRelay)
            .withOptionsFrom (dlRRelay).withOptionsFrom (fbRRelay).withOptionsFrom (tnRRelay)
            .withOptionsFrom (wRRelay).withOptionsFrom (dryRRelay)
            .withOptionsFrom (controlParamReceiver));

    addAndMakeVisible (*webView);

    auto sl = [this] (const char* id, juce::WebSliderRelay& r)
    { return std::make_unique<juce::WebSliderParameterAttachment>(*audioProcessor.apvts.getParameter (id), r, nullptr); };

    modeAtt = std::make_unique<juce::WebComboBoxParameterAttachment>(*audioProcessor.apvts.getParameter (ParameterIDs::mode), modeRelay, nullptr);
    noteLAtt = std::make_unique<juce::WebComboBoxParameterAttachment>(*audioProcessor.apvts.getParameter (ParameterIDs::note_l), noteLRelay, nullptr);
    noteRAtt = std::make_unique<juce::WebComboBoxParameterAttachment>(*audioProcessor.apvts.getParameter (ParameterIDs::note_r), noteRRelay, nullptr);
    linkAtt = std::make_unique<juce::WebToggleButtonParameterAttachment>(*audioProcessor.apvts.getParameter (ParameterIDs::link), linkRelay, nullptr);
    syncAtt = std::make_unique<juce::WebToggleButtonParameterAttachment>(*audioProcessor.apvts.getParameter (ParameterIDs::sync), syncRelay, nullptr);
    dlLAtt = sl (ParameterIDs::delay_l, dlLRelay); fbLAtt = sl (ParameterIDs::fb_l, fbLRelay);
    tnLAtt = sl (ParameterIDs::tone_l, tnLRelay);  wLAtt  = sl (ParameterIDs::wet_l, wLRelay);  dryLAtt = sl (ParameterIDs::dry_l, dryLRelay);
    dlRAtt = sl (ParameterIDs::delay_r, dlRRelay); fbRAtt = sl (ParameterIDs::fb_r, fbRRelay);
    tnRAtt = sl (ParameterIDs::tone_r, tnRRelay);  wRAtt  = sl (ParameterIDs::wet_r, wRRelay);  dryRAtt = sl (ParameterIDs::dry_r, dryRRelay);

    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());
    setSize (380, 320);
}

VAZDelayAudioProcessorEditor::~VAZDelayAudioProcessorEditor() {}

void VAZDelayAudioProcessorEditor::paint (juce::Graphics& g) { g.fillAll (juce::Colour (0xff2a0d0d)); }

void VAZDelayAudioProcessorEditor::resized()
{
    if (webView != nullptr) webView->setBounds (getLocalBounds());
}

int VAZDelayAudioProcessorEditor::getControlParameterIndex (juce::Component&)
{
    return controlParamReceiver.getControlParameterIndex();
}

const char* VAZDelayAudioProcessorEditor::getMimeForExtension (const juce::String& extension)
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
VAZDelayAudioProcessorEditor::getResource (const juce::String& url)
{
    auto path = url.fromFirstOccurrenceOf (
        juce::WebBrowserComponent::getResourceProviderRoot(), false, false);
    if (path.startsWithChar ('/')) path = path.substring (1);
    if (path.isEmpty()) path = "index.html";

    const char* data = nullptr;
    int size = 0;
    juce::String mime;

    if (path == "index.html")                       { data = BinaryData::index_html; size = BinaryData::index_htmlSize; mime = "text/html"; }
    else if (path == "js/index.js")                 { data = BinaryData::index_js; size = BinaryData::index_jsSize; mime = "text/javascript"; }
    else if (path == "js/juce/index.js")            { data = BinaryData::index_js2; size = BinaryData::index_js2Size; mime = "text/javascript"; }
    else if (path == "js/juce/check_native_interop.js") { data = BinaryData::check_native_interop_js; size = BinaryData::check_native_interop_jsSize; mime = "text/javascript"; }

    if (data != nullptr && size > 0)
    {
        std::vector<std::byte> bytes ((size_t) size);
        std::memcpy (bytes.data(), data, (size_t) size);
        return juce::WebBrowserComponent::Resource { std::move (bytes), mime };
    }
    return std::nullopt;
}
