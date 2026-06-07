# VAZDecimator — standalone clone of VAZ 2010's Decimator (sample-rate + bit-depth reduction), WebView UI.
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

message(STATUS "VAZDecimator: Building for ${PLUGIN_FORMATS}")

juce_add_binary_data(VAZDecimator_WebUI
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/index.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
)

juce_add_plugin(VAZDecimator
    COMPANY_NAME "APC"
    PLUGIN_MANUFACTURER_CODE Apcl
    PLUGIN_CODE Vzdc
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "VAZDecimator"
    NEEDS_WEBVIEW2 ${NEEDS_WEBVIEW2}
    NEEDS_WEB_BROWSER ${NEEDS_WEB_BROWSER}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    VST3_CATEGORIES Fx Distortion
)

target_sources(VAZDecimator
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_include_directories(VAZDecimator PRIVATE Source)

target_link_libraries(VAZDecimator
    PRIVATE
        VAZDecimator_WebUI
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

target_compile_definitions(VAZDecimator
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        _USE_MATH_DEFINES=1
)

if(WIN32)
    target_compile_definitions(VAZDecimator PUBLIC JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1)
endif()
if(UNIX AND NOT APPLE)
    target_compile_definitions(VAZDecimator PUBLIC JUCE_USE_CURL=0)
endif()
