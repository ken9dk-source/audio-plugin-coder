# PeakLFO test suite (Catch2 v3). Enabled from the top level with -DAPC_BUILD_TESTS=ON.
# FPCEngine.h is header-only (no JUCE), so the test target is dependency-light.
include(FetchContent)
FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.6.0
    GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(Catch2)

add_executable(PeakLFO_Tests test_phase1.cpp test_phase2.cpp test_phase3.cpp)
target_include_directories(PeakLFO_Tests PRIVATE ../Source)
target_compile_definitions(PeakLFO_Tests PRIVATE _USE_MATH_DEFINES=1)
target_link_libraries(PeakLFO_Tests PRIVATE Catch2::Catch2WithMain)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(PeakLFO_Tests)
