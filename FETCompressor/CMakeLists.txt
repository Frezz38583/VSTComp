cmake_minimum_required(VERSION 3.22)
project(FETCompressor VERSION 1.0.0)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(FETCompressor
    COMPANY_NAME "MyStudio"
    PLUGIN_MANUFACTURER_CODE Mstd
    PLUGIN_CODE Fet1
    FORMATS VST3 Standalone
    PRODUCT_NAME "FET Compressor"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

juce_generate_juce_header(FETCompressor)

target_sources(FETCompressor PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(FETCompressor PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_compile_features(FETCompressor PUBLIC cxx_std_17)
target_compile_options(FETCompressor PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

target_link_libraries(FETCompressor PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags)
