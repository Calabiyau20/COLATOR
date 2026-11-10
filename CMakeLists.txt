cmake_minimum_required(VERSION 3.22)

# These must be set BEFORE project(), which seeds them into the cache as empty.
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum macOS version")
set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Universal binary")

project(Percoolator VERSION 0.2.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Xcode 15.0 exactly ships a linker with known issues, and JUCE refuses to build
# on it. The documented workaround is the classic linker. Scoped to that one
# release so newer toolchains (CI runners, updated machines) link normally —
# -ld_classic is deprecated from Xcode 16 on.
if (APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang"
          AND CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 15.0
          AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 15.1)
    message(STATUS "Xcode 15.0 detected — using the classic linker")
    add_compile_definitions(JUCE_SILENCE_XCODE_15_LINKER_WARNING=1)
    add_link_options("-Wl,-ld_classic")
endif()

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.12
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Percoolator
    COMPANY_NAME "Calabi Yau"
    BUNDLE_ID "live.calabiyau.percoolator"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Clby
    PLUGIN_CODE Pcol
    FORMATS AU VST3 Standalone
    VST3_CATEGORIES "Instrument" "Drum"
    AU_MAIN_TYPE "kAudioUnitType_MusicDevice"
    PRODUCT_NAME "Percoolator")

juce_generate_juce_header(Percoolator)

target_sources(Percoolator PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/NeonVoid.cpp)

target_compile_definitions(Percoolator PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Percoolator
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
