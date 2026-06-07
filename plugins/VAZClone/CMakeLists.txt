# VAZClone — faithful VAZ 2010 virtual-analog clone (WebView UI)
# Modeled on the working CloudWash webview plugin in this repo.

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

message(STATUS "VAZClone: Building for ${PLUGIN_FORMATS}")

# ============================================
# BINARY DATA (Web UI Resources)
# ============================================
juce_add_binary_data(VAZClone_WebUI
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/index.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
)

# ============================================
# PLUGIN DEFINITION
# ============================================
juce_add_plugin(VAZClone
    COMPANY_NAME "APC"
    PLUGIN_MANUFACTURER_CODE Apcl
    PLUGIN_CODE Vazc
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "VAZClone"
    NEEDS_WEBVIEW2 ${NEEDS_WEBVIEW2}
    NEEDS_WEB_BROWSER ${NEEDS_WEB_BROWSER}
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    VST3_CATEGORIES Instrument Synth
)

# ============================================
# SOURCE FILES
# ============================================
target_sources(VAZClone
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_include_directories(VAZClone PRIVATE Source)

# ============================================
# LINK LIBRARIES
# ============================================
target_link_libraries(VAZClone
    PRIVATE
        VAZClone_WebUI
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

# ============================================
# COMPILE DEFINITIONS
# ============================================
target_compile_definitions(VAZClone
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        _USE_MATH_DEFINES=1
)

if(WIN32)
    target_compile_definitions(VAZClone PUBLIC JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1)
endif()
if(UNIX AND NOT APPLE)
    target_compile_definitions(VAZClone PUBLIC JUCE_USE_CURL=0)
endif()
