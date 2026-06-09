cmake_minimum_required(VERSION 3.22)
project(Andre VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(../JUCE JUCE)

juce_add_plugin(Andre
    COMPANY_NAME "MyPlugins"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    PLUGIN_MANUFACTURER_CODE Mplg
    PLUGIN_CODE Andr
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Andre"
)

target_sources(Andre PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_definitions(Andre PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

juce_generate_juce_header(Andre)

target_link_libraries(Andre PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
