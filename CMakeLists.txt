cmake_minimum_required(VERSION 3.22)
project(Bakterizid VERSION 1.0.2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE: liegt ein Ordner "JUCE" neben dieser Datei, wird er benutzt (schnell, offline).
# Sonst lädt CMake JUCE selbst von GitHub (so läuft es auch auf GitHub Actions).
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 9.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

# Lokal das fertige Plugin gleich in den Plugin-Ordner kopieren, in CI nicht
option(BAKTERIZID_COPY_AFTER_BUILD "Plugin nach dem Bauen in den Plugin-Ordner kopieren" ON)

juce_add_plugin(Bakterizid
    COMPANY_NAME "Sarin"
    COMPANY_WEBSITE "https://github.com"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    PLUGIN_MANUFACTURER_CODE Sarn
    PLUGIN_CODE Bakz
    FORMATS VST3 AU Standalone
    COPY_PLUGIN_AFTER_BUILD ${BAKTERIZID_COPY_AFTER_BUILD}
    NEEDS_WEB_BROWSER TRUE
    NEEDS_WEBVIEW2 TRUE
    VERSION ${PROJECT_VERSION}
    PLUGIN_NAME "Bakterizid"
    PRODUCT_NAME "Bakterizid"
    DESCRIPTION "Xeno-FM-Synthesizer"
    BUNDLE_ID com.sarin.bakterizid
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    VST3_CATEGORIES Instrument Synth)

# Die Oberfläche (HTML + Schriften) wird ins Plugin eingebettet
juce_add_binary_data(BakterizidUI
    HEADER_NAME BakterizidUI.h
    NAMESPACE BakterizidUI
    SOURCES
        UI/index.html
        UI/fonts/MajorMonoDisplay-Regular.ttf
        UI/fonts/IBMPlexSansCondensed-Regular.ttf
        UI/fonts/IBMPlexSansCondensed-SemiBold.ttf
        UI/fonts/IBMPlexSansCondensed-Italic.ttf
        UI/fonts/IBMPlexMono-Regular.ttf
        UI/fonts/IBMPlexMono-Medium.ttf)

target_sources(Bakterizid PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginProcessor.h
    Source/PluginEditor.cpp
    Source/PluginEditor.h
    Source/Engine.h
    Source/Presets.h)

target_compile_definitions(Bakterizid PUBLIC
    JUCE_WEB_BROWSER=1
    JUCE_USE_CURL=0
    JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(Bakterizid
    PRIVATE
        BakterizidUI
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
