cmake_minimum_required(VERSION 3.22)

project(MidiGen VERSION 0.1.1)

# ---------------------------------------------------------------------------
# JUCE: use a local copy if you point JUCE_DIR at one, otherwise download it.
#   cmake -B build -DJUCE_DIR=/path/to/JUCE
# ---------------------------------------------------------------------------
set(JUCE_DIR "" CACHE PATH "Path to a local JUCE checkout (optional)")

if(JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.9
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

# Copy the plugin into the system plug-in folders after building (turned off on the build servers)
option(MG_COPY_AFTER_BUILD "Install the plugin after building" ON)

# Windows: link the C++ runtime statically, so friends don't need to install anything extra
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# On a Mac this builds AU + VST3 + a standalone app.
set(HS_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND HS_FORMATS AU)
endif()

juce_add_plugin(MidiGen
    COMPANY_NAME                "MB Audio"
    BUNDLE_ID                   com.mbaudio.midigen
    PLUGIN_MANUFACTURER_CODE    Mbau
    PLUGIN_CODE                 Mgen
    FORMATS                     ${HS_FORMATS}
    PRODUCT_NAME                "MIDI GEN"
    IS_SYNTH                    TRUE
    NEEDS_MIDI_INPUT            TRUE
    NEEDS_MIDI_OUTPUT           TRUE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     ${MG_COPY_AFTER_BUILD}
    AU_MAIN_TYPE                kAudioUnitType_MusicDevice
    VST3_CATEGORIES             Instrument Generator)

# Fonts (SIL Open Font License, see Resources/Fonts/OFL-*.txt)
juce_add_binary_data(MidiGenFonts SOURCES
    Resources/Fonts/Michroma-Regular.ttf
    Resources/Fonts/Rajdhani-SemiBold.ttf
    Resources/Fonts/Rajdhani-Bold.ttf
    Resources/Fonts/ShareTechMono-Regular.ttf)

target_sources(MidiGen PRIVATE
    Source/Music.cpp
    Source/Genres.cpp
    Source/Reharm.cpp
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/UIKit.cpp)

target_compile_features(MidiGen PRIVATE cxx_std_17)

target_compile_definitions(MidiGen PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_AU=1)

target_link_libraries(MidiGen
    PRIVATE
        MidiGenFonts
        juce::juce_audio_utils
        juce::juce_audio_processors
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# Reharmonizer tests (no JUCE): cmake --build build --target MidiGenTests
add_executable(MidiGenTests EXCLUDE_FROM_ALL Tests/GenTests.cpp Source/Reharm.cpp Source/Music.cpp Source/Genres.cpp)
target_compile_features(MidiGenTests PRIVATE cxx_std_17)

juce_add_console_app(MidiGenPluginTests PRODUCT_NAME "MidiGenPluginTests")
set_target_properties(MidiGenPluginTests PROPERTIES EXCLUDE_FROM_ALL TRUE)
target_sources(MidiGenPluginTests PRIVATE Tests/PluginTests.cpp)
target_compile_features(MidiGenPluginTests PRIVATE cxx_std_17)
target_compile_definitions(MidiGenPluginTests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(MidiGenPluginTests PRIVATE MidiGen juce::juce_audio_utils juce::juce_recommended_config_flags)

# Promo video renderer (real screen + real sound): cmake --build build --target MidiGenVideoKit
juce_add_console_app(MidiGenVideoKit PRODUCT_NAME "MidiGenVideoKit")
set_target_properties(MidiGenVideoKit PROPERTIES EXCLUDE_FROM_ALL TRUE)
target_sources(MidiGenVideoKit PRIVATE Tools/VideoKit.cpp)
target_compile_features(MidiGenVideoKit PRIVATE cxx_std_17)
target_compile_definitions(MidiGenVideoKit PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_PLUGINHOST_VST3=1)
target_link_libraries(MidiGenVideoKit PRIVATE MidiGen juce::juce_audio_utils juce::juce_audio_formats juce::juce_recommended_config_flags)
