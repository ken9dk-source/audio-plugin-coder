# PeakLFO Plugin (Visage)
# Faithful clone of the classic peak controller LFO + peak follower.
# Cross-platform: Windows (VST3), macOS (VST3/AU), Linux (VST3/LV2)

# ============================================
# REQUIRE VISAGE
# ============================================
if(NOT TARGET visage::visage)
    message(STATUS "Skipping PeakLFO: Visage not enabled (set APC_ENABLE_VISAGE=ON to build)")
    return()
endif()

# ============================================
# PLATFORM-SPECIFIC CONFIGURATION
# ============================================
if(WIN32)
    set(PLUGIN_FORMATS VST3 Standalone)
elseif(APPLE)
    set(PLUGIN_FORMATS VST3 AU Standalone)
    set(AU_MAIN_TYPE kAudioUnitType_Effect)
elseif(UNIX)
    set(PLUGIN_FORMATS VST3 LV2 Standalone)
    set(LV2_URI "https://github.com/noizefield/audio-plugin-coder/PeakLFO")
else()
    message(FATAL_ERROR "Unsupported platform")
endif()

message(STATUS "PeakLFO: Building for ${PLUGIN_FORMATS}")

# ============================================
# PLUGIN DEFINITION
# ============================================
juce_add_plugin(PeakLFO
    COMPANY_NAME "APC"
    PLUGIN_MANUFACTURER_CODE Apc0
    PLUGIN_CODE PkLf
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "PeakLFO"
    VST3_CATEGORIES Fx Modulation
    AU_MAIN_TYPE ${AU_MAIN_TYPE}
    LV2URI ${LV2_URI}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)

# ============================================
# BINARY DATA (Embedded Assets)
# ============================================
juce_add_binary_data(PeakLFO_Assets
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/../../_tools/visage/visage_graphics/fonts/Lato-Regular.ttf
    NAMESPACE PeakLFO_BinaryData
)

# ============================================
# SOURCE FILES
# ============================================
target_sources(PeakLFO
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/VisageControls.h
        Source/FPCEngine.h
)

# ============================================
# INCLUDE PATHS
# ============================================
target_include_directories(PeakLFO
    PRIVATE
        Source
        ${CMAKE_CURRENT_SOURCE_DIR}/../../common
)

# ============================================
# LINK LIBRARIES
# ============================================
target_link_libraries(PeakLFO
    PRIVATE
        PeakLFO_Assets
        visage::visage
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
        juce::juce_opengl
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ============================================
# COMPILE DEFINITIONS
# ============================================
target_compile_definitions(PeakLFO
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=0
        JUCE_USE_WIN_WEBVIEW2=0
        _USE_MATH_DEFINES=1
)

if(APPLE)
    target_compile_definitions(PeakLFO PUBLIC JUCE_USE_CURL=0)
endif()

if(UNIX AND NOT APPLE)
    target_compile_definitions(PeakLFO PUBLIC JUCE_USE_CURL=0 JUCE_JACK=1)
endif()

# ============================================
# TESTS (Catch2) — enable with -DAPC_BUILD_TESTS=ON
# ============================================
option(APC_BUILD_TESTS "Build the plugin Catch2 test suites" OFF)
if(APC_BUILD_TESTS)
    add_subdirectory(Tests)
endif()
