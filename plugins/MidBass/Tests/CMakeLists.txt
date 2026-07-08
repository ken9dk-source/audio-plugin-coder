# MidBass test suite (Catch2 v3). Enabled from the top level with -DAPC_BUILD_TESTS=ON.
# Compiles the processor fresh with MIDBASS_HEADLESS=1 (no editor / GUI) and links Catch2.
include(FetchContent)
FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.6.0
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(Catch2)

juce_add_console_app(MidBass_Tests PRODUCT_NAME "MidBass_Tests")

target_sources(MidBass_Tests PRIVATE
    test_parameters.cpp
    test_state.cpp
    test_processor.cpp
    test_oscillators.cpp
    test_filter.cpp
    test_voice.cpp
    test_mod.cpp
    test_tone.cpp
    test_fx.cpp
    test_macros.cpp
    test_integration.cpp
    ../Source/PluginProcessor.cpp)

# MidBass Source FIRST so its ParameterIDs.hpp wins over TranceAcid's.
target_include_directories(MidBass_Tests PRIVATE
    ../Source
    ../../VAZClone/Source
    ../../TranceAcid/Source
    ../../TranceEQ/Source
    ../../VAZChorus/Source
    ../../VAZPhaser/Source
    ../../VAZDelay/Source
    ../../VAZReverb/Source)

target_compile_definitions(MidBass_Tests PRIVATE
    MIDBASS_HEADLESS=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    _USE_MATH_DEFINES=1)

target_link_libraries(MidBass_Tests PRIVATE
    Catch2::Catch2WithMain
    juce::juce_audio_basics
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_recommended_config_flags)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(MidBass_Tests)

# Phase 9 audition renderer: all 10 factory presets to WAV (ear sign-off).
juce_add_console_app(MidBass_Render PRODUCT_NAME "MidBass_Render")
target_sources(MidBass_Render PRIVATE render_main.cpp ../Source/PluginProcessor.cpp)
target_include_directories(MidBass_Render PRIVATE
    ../Source ../../VAZClone/Source ../../TranceAcid/Source ../../TranceEQ/Source
    ../../VAZChorus/Source ../../VAZPhaser/Source ../../VAZDelay/Source ../../VAZReverb/Source)
target_compile_definitions(MidBass_Render PRIVATE
    MIDBASS_HEADLESS=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=0 _USE_MATH_DEFINES=1)
target_link_libraries(MidBass_Render PRIVATE
    juce::juce_audio_basics juce::juce_audio_formats juce::juce_audio_processors
    juce::juce_core juce::juce_data_structures juce::juce_dsp juce::juce_events
    juce::juce_graphics juce::juce_gui_basics juce::juce_recommended_config_flags)

# Offscreen GUI snapshot / lifecycle tool (Phase 8) — compiles the REAL editor
# (no MIDBASS_HEADLESS), renders the panel to PNG for layout review.
juce_add_console_app(MidBass_Snapshot PRODUCT_NAME "MidBass_Snapshot")
target_sources(MidBass_Snapshot PRIVATE
    snapshot_main.cpp
    ../Source/PluginProcessor.cpp
    ../Source/PluginEditor.cpp)
target_include_directories(MidBass_Snapshot PRIVATE
    ../Source
    ../../VAZClone/Source
    ../../TranceAcid/Source
    ../../TranceEQ/Source
    ../../VAZChorus/Source
    ../../VAZPhaser/Source
    ../../VAZDelay/Source
    ../../VAZReverb/Source)
target_compile_definitions(MidBass_Snapshot PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    _USE_MATH_DEFINES=1)
target_link_libraries(MidBass_Snapshot PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_data_structures
    juce::juce_dsp
    juce::juce_events
    juce::juce_graphics
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_recommended_config_flags)
