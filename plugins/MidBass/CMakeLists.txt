# MidBass — dedicated trance mid-bass synthesizer, native JUCE GUI (no WebView).
if(WIN32)
    set(PLUGIN_FORMATS VST3 Standalone)
elseif(APPLE)
    set(PLUGIN_FORMATS VST3 AU Standalone)
else()
    set(PLUGIN_FORMATS VST3 Standalone)
endif()

message(STATUS "MidBass: Building for ${PLUGIN_FORMATS}")

juce_add_plugin(MidBass
    COMPANY_NAME "APC"
    PLUGIN_MANUFACTURER_CODE Apcl
    PLUGIN_CODE Mdbs
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "MidBass"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    VST3_CATEGORIES Instrument Synth
)

target_sources(MidBass PRIVATE Source/PluginProcessor.cpp)

# Source first so MidBass headers win; VAZClone/Source supplies the reused VAZ
# oscillator + filter engines from Phase 1 on (header-only, no VAZClone .cpp).
target_include_directories(MidBass PRIVATE
    Source
    ${CMAKE_CURRENT_SOURCE_DIR}/../VAZClone/Source
    ${CMAKE_CURRENT_SOURCE_DIR}/../TranceAcid/Source
    ${CMAKE_CURRENT_SOURCE_DIR}/../TranceEQ/Source)

target_link_libraries(MidBass
    PRIVATE
        juce::juce_audio_basics juce::juce_audio_devices juce::juce_audio_formats
        juce::juce_audio_plugin_client juce::juce_audio_processors juce::juce_audio_utils
        juce::juce_core juce::juce_data_structures juce::juce_dsp juce::juce_events
        juce::juce_graphics juce::juce_gui_basics juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags
)

target_compile_definitions(MidBass
    PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 _USE_MATH_DEFINES=1)

option(APC_BUILD_TESTS "Build the plugin Catch2 test suites" OFF)
if(APC_BUILD_TESTS)
    add_subdirectory(Tests)
endif()
