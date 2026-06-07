cmake_minimum_required(VERSION 3.15)

# SIDTrance uses the Visage UI framework. When Visage isn't enabled the
# visage::visage target doesn't exist — skip this plugin entirely (same guard
# as gnarly3) so it doesn't break the whole-project CMake configure/generate.
if(NOT TARGET visage::visage)
    message(STATUS "Skipping SIDTrance: Visage not enabled (set APC_ENABLE_VISAGE=ON to build)")
    return()
endif()

# ── Plugin definition ──────────────────────────────────────
# NEEDS_WEBVIEW2     → Windows: statically links WebView2 SDK
# NEEDS_WEB_BROWSER  → Linux: pulls in webkit2gtk + GTK include paths
#                      (no-op on Windows/macOS).  macOS uses WKWebView via
#                      the system framework — no extra flag required.
juce_add_plugin(SIDTrance
    COMPANY_NAME            "CMOSAudio"
    BUNDLE_ID               "com.cmos-audio.sidyssey"
    PLUGIN_MANUFACTURER_CODE Cmos
    PLUGIN_CODE             Sitr
    FORMATS                 VST3 Standalone
    PRODUCT_NAME            "SIDyssey"
    PLUGIN_DESCRIPTION      "SID Hybrid Trance Synthesizer"
    IS_SYNTH                TRUE
    NEEDS_MIDI_INPUT        TRUE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    NEEDS_WEBVIEW2          TRUE
    NEEDS_WEB_BROWSER       TRUE
    VST3_CATEGORIES         Instrument Synth
)

juce_generate_juce_header(SIDTrance)

# ── Binary assets ──────────────────────────────────────────
# Embeds:
#   - web/index.html — TranceSID v3 UI (the entire visible GUI)
#   - web/js/juce/juce_index.js — JUCE WebView interop frontend library
#   - web/js/juce/check_native_interop.js — companion checker
#
# juce_add_binary_data flattens directory paths to file basenames.  The
# editor's resource provider looks up by basename via getNamedResource.
juce_add_binary_data(SIDTrance_Assets
    SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/web/index.html"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/web/js/juce/juce_index.js"
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/web/js/juce/check_native_interop.js"
    NAMESPACE BinaryData
)

# ── Source files ───────────────────────────────────────────
# VisageControls.h removed — the WebView editor no longer references it,
# and the visage::visage dependency was its only reason for being linked.
target_sources(SIDTrance
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.h
)

# ── Include paths ──────────────────────────────────────────
target_include_directories(SIDTrance
    PRIVATE
        Source
)

# ── Link libraries ─────────────────────────────────────────
target_link_libraries(SIDTrance
    PRIVATE
        SIDTrance_Assets
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
        juce::juce_gui_extra            # NEW — required for WebBrowserComponent
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ── Compile definitions ────────────────────────────────────
# JUCE_WEB_BROWSER=1                              → enables WebBrowserComponent
# JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1     → Windows: static WebView2 SDK
target_compile_definitions(SIDTrance
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
        _USE_MATH_DEFINES=1
)
