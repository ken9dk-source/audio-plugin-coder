cmake_minimum_required(VERSION 3.15)

# ── Plugin definition ──────────────────────────────────────
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
    VST3_CATEGORIES         Instrument Synth
)

juce_generate_juce_header(SIDTrance)

# ── Binary assets (Lato font + GUI background for Visage) ──
juce_add_binary_data(SIDTrance_Assets
    SOURCES
        ${CMAKE_CURRENT_SOURCE_DIR}/../../_tools/visage/visage_graphics/fonts/Lato-Regular.ttf
        "${CMAKE_CURRENT_SOURCE_DIR}/Resources/TranceSID Gui Design.png"
    NAMESPACE BinaryData
)

# ── Source files ───────────────────────────────────────────
target_sources(SIDTrance
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/VisageControls.h
        Source/PluginProcessor.h
        Source/PluginEditor.h
)

# ── Include paths ──────────────────────────────────────────
target_include_directories(SIDTrance
    PRIVATE
        Source
        ${CMAKE_CURRENT_SOURCE_DIR}/../../common
)

# ── Link libraries ─────────────────────────────────────────
target_link_libraries(SIDTrance
    PRIVATE
        SIDTrance_Assets
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
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ── Compile definitions ────────────────────────────────────
target_compile_definitions(SIDTrance
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_WEB_BROWSER=0
        _USE_MATH_DEFINES=1
)
