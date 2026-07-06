# TranceEQ test suite (Catch2 v3). Enabled from the top level with -DAPC_BUILD_TESTS=ON.
# Compiles the processor + DSP fresh with TRANCEEQ_HEADLESS=1 (no editor / WebView / BinaryData)
# and links Catch2. Deterministic, no audio/MIDI devices.
include(FetchContent)
FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.6.0
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(Catch2)

juce_add_console_app(TranceEQ_Tests PRODUCT_NAME "TranceEQ_Tests")

target_sources(TranceEQ_Tests PRIVATE
    test_biquad.cpp
    test_processor.cpp
    test_state.cpp
    ../Source/PluginProcessor.cpp)

target_include_directories(TranceEQ_Tests PRIVATE ../Source)

target_compile_definitions(TranceEQ_Tests PRIVATE
    TRANCEEQ_HEADLESS=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    _USE_MATH_DEFINES=1
    TRANCEEQ_TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

target_link_libraries(TranceEQ_Tests PRIVATE
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

# CTest discovery (optional; the exe also gates on its own exit code).
list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(TranceEQ_Tests)
