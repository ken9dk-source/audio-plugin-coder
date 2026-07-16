# VAZDelay — standalone clone of VAZ 2010's Delay (3-mode stereo echo), WebView UI.
set(NEEDS_WEBVIEW2 FALSE)
set(NEEDS_WEB_BROWSER FALSE)

if(WIN32)
    set(PLUGIN_FORMATS VST3 Standalone)
    set(NEEDS_WEBVIEW2 TRUE)
elseif(APPLE)
    set(PLUGIN_FORMATS VST3 AU Standalone)
elseif(UNIX)
    set(PLUGIN_FORMATS VST3 Standalone)
    set(NEEDS_WEB_BROWSER TRUE)
endif()

message(STATUS "VAZDelay: Building for ${PLUGIN_FORMATS}")

juce_add_binary_data(VAZDelay_WebUI
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/index.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
)

juce_add_plugin(VAZDelay
    COMPANY_NAME "APC"
    PLUGIN_MANUFACTURER_CODE Apcl
    PLUGIN_CODE Vzdl
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "VAZDelay"
    NEEDS_WEBVIEW2 ${NEEDS_WEBVIEW2}
    NEEDS_WEB_BROWSER ${NEEDS_WEB_BROWSER}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    VST3_CATEGORIES Fx Delay
)

target_sources(VAZDelay PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_include_directories(VAZDelay PRIVATE Source)

target_link_libraries(VAZDelay
    PRIVATE
        VAZDelay_WebUI
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

target_compile_definitions(VAZDelay
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        _USE_MATH_DEFINES=1
)

if(WIN32)
    target_compile_definitions(VAZDelay PUBLIC JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1)
endif()
if(UNIX AND NOT APPLE)
    target_compile_definitions(VAZDelay PUBLIC JUCE_USE_CURL=0)
endif()

# Headless end-to-end render test. Links the REAL processor with VAZDELAY_HEADLESS=1 (no WebView editor),
# feeds an impulse and asserts an audible delay tail leaves processBlock on the shipped defaults. The oracle
# tests the ENGINE directly; this covers the processor's param->engine mapping (the "oracle green, plugin
# silent" gap). Run: VazDelayRenderTest.exe
juce_add_console_app(VazDelayRenderTest PRODUCT_NAME "VazDelayRenderTest")
target_sources(VazDelayRenderTest PRIVATE Source/vazdelay_render_test.cpp Source/PluginProcessor.cpp)
target_include_directories(VazDelayRenderTest PRIVATE Source)
target_compile_definitions(VazDelayRenderTest PRIVATE
    VAZDELAY_HEADLESS=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=0 _USE_MATH_DEFINES=1)
target_link_libraries(VazDelayRenderTest PRIVATE
    juce::juce_audio_basics juce::juce_audio_processors juce::juce_audio_utils
    juce::juce_core juce::juce_data_structures juce::juce_dsp juce::juce_events)
