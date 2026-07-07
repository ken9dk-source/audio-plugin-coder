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
    ../Source/PluginProcessor.cpp)

target_include_directories(MidBass_Tests PRIVATE
    ../Source
    ../../VAZClone/Source)

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
