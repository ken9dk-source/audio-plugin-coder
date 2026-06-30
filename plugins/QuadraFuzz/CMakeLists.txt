# QuadraFuzz — 4-band LR4 distortion (RE from original QuadraFuzz.dll), native JUCE UI.
if(WIN32)
    set(PLUGIN_FORMATS VST3 Standalone)
elseif(APPLE)
    set(PLUGIN_FORMATS VST3 AU Standalone)
else()
    set(PLUGIN_FORMATS VST3 Standalone)
endif()

message(STATUS "QuadraFuzz: Building for ${PLUGIN_FORMATS}")

# Image assets used by the native JUCE editor.
juce_add_binary_data(QuadraFuzz_WebUI
    SOURCES
        Design/extracted/png/1000.png
        Design/extracted/png/1001.png
        Design/extracted/png/1002.png
        Design/extracted/png/1003.png
        Design/extracted/png/1004.png
        Design/extracted/png/1005.png
        Design/extracted/png/1006.png
        Design/extracted/png/1007.png
        Design/extracted/png/1008.png
        Design/extracted/png/1009.png
        Design/extracted/png/1010.png
        Design/extracted/png/1011.png
        Design/extracted/png/1012.png
)

juce_add_plugin(QuadraFuzz
    COMPANY_NAME "APC"
    PLUGIN_MANUFACTURER_CODE Apcl
    PLUGIN_CODE QdFz
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "QuadraFuzz"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    VST3_CATEGORIES Fx Distortion
)

target_sources(QuadraFuzz PRIVATE Source/PluginProcessor.cpp Source/PluginEditor.cpp)
target_include_directories(QuadraFuzz PRIVATE Source)

target_link_libraries(QuadraFuzz
    PRIVATE
        QuadraFuzz_WebUI
        juce::juce_audio_basics juce::juce_audio_devices juce::juce_audio_formats
        juce::juce_audio_plugin_client juce::juce_audio_processors juce::juce_audio_utils
        juce::juce_core juce::juce_data_structures juce::juce_dsp juce::juce_events
        juce::juce_graphics juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags
)

target_compile_definitions(QuadraFuzz
    PUBLIC
        JUCE_VST3_CAN_REPLACE_VST2=0
        _USE_MATH_DEFINES=1
        JUCE_WEB_BROWSER=0
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=0)

if(UNIX AND NOT APPLE)
    target_compile_definitions(QuadraFuzz PUBLIC JUCE_USE_CURL=0)
endif()
