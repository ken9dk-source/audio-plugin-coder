# VAZClone — faithful VAZ 2010 virtual-analog clone (WebView UI)
# Modeled on the working CloudWash webview plugin in this repo.

set(NEEDS_WEBVIEW2 FALSE)
set(NEEDS_WEB_BROWSER FALSE)
set(LV2_URI "")

if(WIN32)
    set(PLUGIN_FORMATS VST3 Standalone)
    set(NEEDS_WEBVIEW2 TRUE)
elseif(APPLE)
    set(PLUGIN_FORMATS VST3 AU Standalone)
elseif(UNIX)
    set(PLUGIN_FORMATS VST3 LV2 Standalone)
    set(NEEDS_WEB_BROWSER TRUE)
    set(LV2_URI "https://github.com/noizefield/audio-plugin-coder/VAZClone")
endif()

message(STATUS "VAZClone: Building for ${PLUGIN_FORMATS}")

# ============================================
# BINARY DATA (Web UI Resources)
# ============================================
juce_add_binary_data(VAZClone_WebUI
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/index.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
)

# ============================================
# PLUGIN DEFINITION
# ============================================
juce_add_plugin(VAZClone
    COMPANY_NAME "APC"
    PLUGIN_MANUFACTURER_CODE Apcl
    PLUGIN_CODE Vazc
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "VAZClone"
    NEEDS_WEBVIEW2 ${NEEDS_WEBVIEW2}
    NEEDS_WEB_BROWSER ${NEEDS_WEB_BROWSER}
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    VST3_CATEGORIES Instrument Synth
    LV2URI ${LV2_URI}
)

# ============================================
# SOURCE FILES
# ============================================
target_sources(VAZClone
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

target_include_directories(VAZClone PRIVATE Source)

# ============================================
# LINK LIBRARIES
# ============================================
target_link_libraries(VAZClone
    PRIVATE
        VAZClone_WebUI
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
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ============================================
# COMPILE DEFINITIONS
# ============================================
target_compile_definitions(VAZClone
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_VST3_CAN_REPLACE_VST2=0
        _USE_MATH_DEFINES=1
)

if(WIN32)
    target_compile_definitions(VAZClone PUBLIC JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1)
endif()
if(UNIX AND NOT APPLE)
    target_compile_definitions(VAZClone PUBLIC JUCE_USE_CURL=0)
endif()

# ============================================
# HEADLESS RENDERER (A/B testing) — VazRender
# Compiles the processor + DSP fresh with VAZ_HEADLESS (no editor / WebView / BinaryData),
# instantiates it directly and renders a .mid -> WAV. No audio/MIDI devices, deterministic.
#   VazRender <preset.v2p|-> <midi.mid> <out.wav> [seconds] [sampleRate]
# ============================================
juce_add_console_app(VazRender PRODUCT_NAME "VazRender")
target_sources(VazRender PRIVATE Source/render_main.cpp Source/PluginProcessor.cpp)
target_include_directories(VazRender PRIVATE Source)
target_compile_definitions(VazRender PRIVATE
    VAZ_HEADLESS=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    _USE_MATH_DEFINES=1)
target_link_libraries(VazRender PRIVATE
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

# ============================================
# DSP AUDIT TEST SUITE — VazDSPAudit
# Renders the real processor headlessly and runs click/aliasing/voice-drop/DC/gain detectors.
# Returns the number of failures; several tests are EXPECTED to fail (they pin audited bugs).
# ============================================
juce_add_console_app(VazDSPAudit PRODUCT_NAME "VazDSPAudit")
target_sources(VazDSPAudit PRIVATE Source/dsp_audit_main.cpp Source/PluginProcessor.cpp)
target_include_directories(VazDSPAudit PRIVATE Source)
target_compile_definitions(VazDSPAudit PRIVATE
    VAZ_HEADLESS=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    _USE_MATH_DEFINES=1)
target_link_libraries(VazDSPAudit PRIVATE
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

# ============================================
# NO-DISCARD PARSER AUDIT — VazV2PAudit  (DEL 1)
# Mirrors parseV2P with a labeled cursor and asserts every consumed byte in every factory .v2p is
# either mapped to a param or on an explicit whitelist; drift-checked vs the real parser's end offset.
# ============================================
juce_add_console_app(VazV2PAudit PRODUCT_NAME "VazV2PAudit")
target_sources(VazV2PAudit PRIVATE Source/v2p_audit_main.cpp Source/PluginProcessor.cpp)
target_include_directories(VazV2PAudit PRIVATE Source)
target_compile_definitions(VazV2PAudit PRIVATE
    VAZ_HEADLESS=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    _USE_MATH_DEFINES=1)
target_link_libraries(VazV2PAudit PRIVATE
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

# ============================================
# .v2p ROUND-TRIP ORACLE — VazV2PRoundtrip
# load -> EDIT every param -> buildV2P (save) -> reload -> assert every param survived. Exposes
# writer gaps (buildV2P currently serialises ~26 of ~63 loaded fields).
# ============================================
juce_add_console_app(VazV2PRoundtrip PRODUCT_NAME "VazV2PRoundtrip")
target_sources(VazV2PRoundtrip PRIVATE Source/v2p_roundtrip_main.cpp Source/PluginProcessor.cpp)
target_include_directories(VazV2PRoundtrip PRIVATE Source)
target_compile_definitions(VazV2PRoundtrip PRIVATE
    VAZ_HEADLESS=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    _USE_MATH_DEFINES=1)
target_link_libraries(VazV2PRoundtrip PRIVATE
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

# ============================================
# DECOMPILED REFERENCE-ORACLE — VazOracle  (DEL 2)
# Diffs the clone's DSP primitives (VAZEnv, cutoff smoother, detune) against reference impls
# transcribed from the Ghidra decomp + raw constants (reference/vaz_constants.h). Prints a status
# table BIT-EXACT / DEVIATION(max) / NOT TESTED.
# ============================================
juce_add_console_app(VazOracle PRODUCT_NAME "VazOracle")
target_sources(VazOracle PRIVATE Source/oracle_main.cpp)
target_include_directories(VazOracle PRIVATE Source reference)
target_compile_definitions(VazOracle PRIVATE
    VAZ_HEADLESS=1
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_MODAL_LOOPS_PERMITTED=0
    _USE_MATH_DEFINES=1)
target_link_libraries(VazOracle PRIVATE
    juce::juce_audio_basics
    juce::juce_core
    juce::juce_dsp
    juce::juce_events
    juce::juce_recommended_config_flags)
