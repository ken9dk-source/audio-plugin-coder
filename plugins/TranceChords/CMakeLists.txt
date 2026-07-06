# TranceChords — vocal-trance chord-progression generator (instrument).
# Native JUCE UI + built-in preview pad + MIDI output + OS drag-and-drop of .mid.
if(WIN32)
    set(PLUGIN_FORMATS VST3 Standalone)
elseif(APPLE)
    set(PLUGIN_FORMATS VST3 AU Standalone)
elseif(UNIX)
    set(PLUGIN_FORMATS VST3 Standalone)
endif()

message(STATUS "TranceChords: Building for ${PLUGIN_FORMATS}")

juce_add_plugin(TranceChords
    COMPANY_NAME "APC"
    PLUGIN_MANUFACTURER_CODE Apcl
    PLUGIN_CODE Tchd
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "TranceChords"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    VST3_CATEGORIES Instrument Synth
)

target_sources(TranceChords PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(TranceChords PRIVATE Source)

target_link_libraries(TranceChords
    PRIVATE
        juce::juce_audio_basics juce::juce_audio_devices juce::juce_audio_formats
        juce::juce_audio_plugin_client juce::juce_audio_processors juce::juce_audio_utils
        juce::juce_core juce::juce_data_structures juce::juce_dsp juce::juce_events
        juce::juce_graphics juce::juce_gui_basics juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

target_compile_definitions(TranceChords PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 _USE_MATH_DEFINES=1)

# ---- offline render harness (validates engine + MIDI export + pad sound) ----
juce_add_console_app(TranceChords_Render PRODUCT_NAME "TranceChords_Render")
target_sources(TranceChords_Render PRIVATE Source/render_main.cpp)
target_include_directories(TranceChords_Render PRIVATE Source)
target_compile_definitions(TranceChords_Render PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 _USE_MATH_DEFINES=1 JUCE_STANDALONE_APPLICATION=1)
target_link_libraries(TranceChords_Render
    PRIVATE
        juce::juce_core juce::juce_audio_basics juce::juce_audio_formats juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags)
