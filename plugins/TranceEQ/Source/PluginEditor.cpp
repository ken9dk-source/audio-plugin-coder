#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "BinaryData.h"
#include <cstring>   // std::memcpy
#include <cstddef>   // std::byte

//==============================================================================
TranceEQAudioProcessorEditor::TranceEQAudioProcessorEditor (TranceEQAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    using Options = juce::WebBrowserComponent::Options;

    // ---- 1. one relay per parameter, chosen by parameter type ----
    struct SBind { juce::RangedAudioParameter* param; juce::WebSliderRelay*       relay; };
    struct CBind { juce::RangedAudioParameter* param; juce::WebComboBoxRelay*     relay; };
    struct TBind { juce::RangedAudioParameter* param; juce::WebToggleButtonRelay* relay; };
    std::vector<SBind> sBinds; std::vector<CBind> cBinds; std::vector<TBind> tBinds;

    for (auto* base : audioProcessor.getParameters())
    {
        auto* rp = dynamic_cast<juce::RangedAudioParameter*> (base);
        if (rp == nullptr) continue;
        const auto id = rp->getParameterID();

        if (dynamic_cast<juce::AudioParameterChoice*> (base) != nullptr)
            cBinds.push_back ({ rp, comboRelays.add (new juce::WebComboBoxRelay (id)) });
        else if (dynamic_cast<juce::AudioParameterBool*> (base) != nullptr)
            tBinds.push_back ({ rp, toggleRelays.add (new juce::WebToggleButtonRelay (id)) });
        else
            sBinds.push_back ({ rp, sliderRelays.add (new juce::WebSliderRelay (id)) });
    }

    // ---- 2. build the WebView options (native integration + functions + every relay) ----
    auto options = Options{}
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
                    if (auto* prm = audioProcessor.apvts.getParameter (args[0].toString()))
                        { prm->beginChangeGesture(); prm->setValueNotifyingHost (prm->getDefaultValue()); prm->endChangeGesture(); }
                complete (juce::var());
            })
        .withNativeFunction (juce::Identifier ("loadMode"),
            [this] (const juce::Array<juce::var>& args, juce::WebBrowserComponent::NativeFunctionCompletion complete)
            {
                if (args.size() > 0)
                    audioProcessor.applyModePreset ((int) args[0]);
                complete (juce::var());
            })
        .withOptionsFrom (controlParamReceiver);

    for (auto& b : sBinds) options = options.withOptionsFrom (*b.relay);
    for (auto& b : cBinds) options = options.withOptionsFrom (*b.relay);
    for (auto& b : tBinds) options = options.withOptionsFrom (*b.relay);

    webView = std::make_unique<juce::WebBrowserComponent> (options);
    addAndMakeVisible (*webView);

    // ---- 3. attach each relay to its parameter ----
    for (auto& b : sBinds) sliderAtts.add (new juce::WebSliderParameterAttachment       (*b.param, *b.relay, nullptr));
    for (auto& b : cBinds) comboAtts .add (new juce::WebComboBoxParameterAttachment     (*b.param, *b.relay, nullptr));
    for (auto& b : tBinds) toggleAtts.add (new juce::WebToggleButtonParameterAttachment (*b.param, *b.relay, nullptr));

    webView->goToURL (juce::WebBrowserComponent::getResourceProviderRoot());
    setSize (920, 560);
    startTimerHz (30);
}

TranceEQAudioProcessorEditor::~TranceEQAudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================
void TranceEQAudioProcessorEditor::paint (juce::Graphics& g) { g.fillAll (juce::Colour (0xff10141c)); }

void TranceEQAudioProcessorEditor::resized()
{
    if (webView != nullptr) webView->setBounds (getLocalBounds());
}

int TranceEQAudioProcessorEditor::getControlParameterIndex (juce::Component&)
{
    return controlParamReceiver.getControlParameterIndex();
}

//==============================================================================
void TranceEQAudioProcessorEditor::timerCallback()
{
    if (webView == nullptr || ! webView->isVisible()) return;

    juce::String arr;
    arr.preallocateBytes (1024);
    arr << '[';
    for (int v = 0; v < TranceEQAudioProcessor::kNumViz; ++v)
    {
        if (v != 0) arr << ',';
        arr << juce::String (audioProcessor.vizBins[v].load(), 1);
    }
    arr << ']';

    const float  f0 = audioProcessor.currentF0.load();
    const double sr = audioProcessor.getSampleRate() > 0.0 ? audioProcessor.getSampleRate() : 44100.0;
    juce::String js;
    js << "if(window.teqUpdate){window.teqUpdate(" << arr << ',' << juce::String (f0, 2)
       << ',' << juce::String (sr, 1) << ");}";

    try { webView->evaluateJavascript (js); } catch (...) {}
}

//==============================================================================
std::optional<juce::WebBrowserComponent::Resource>
TranceEQAudioProcessorEditor::getResource (const juce::String& url)
{
    auto path = url.fromFirstOccurrenceOf (
        juce::WebBrowserComponent::getResourceProviderRoot(), false, false);
    if (path.startsWithChar ('/')) path = path.substring (1);
    if (path.isEmpty()) path = "index.html";

    const char* data = nullptr; int size = 0; juce::String mime;

    if (path == "index.html")
        { data = BinaryData::index_html; size = BinaryData::index_htmlSize; mime = "text/html"; }
    else if (path == "js/index.js")
        { data = BinaryData::index_js; size = BinaryData::index_jsSize; mime = "text/javascript"; }
    else if (path == "js/juce/index.js")
        { data = BinaryData::index_js2; size = BinaryData::index_js2Size; mime = "text/javascript"; }
    else if (path == "js/juce/check_native_interop.js")
        { data = BinaryData::check_native_interop_js; size = BinaryData::check_native_interop_jsSize; mime = "text/javascript"; }

    if (data != nullptr && size > 0)
    {
        std::vector<std::byte> bytes ((size_t) size);
        std::memcpy (bytes.data(), data, (size_t) size);
        return juce::WebBrowserComponent::Resource { std::move (bytes), mime };
    }
    return std::nullopt;
}
