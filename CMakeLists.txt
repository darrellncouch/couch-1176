cmake_minimum_required(VERSION 3.22)
project(Couch1176 VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Fetch JUCE ────────────────────────────────────────────────────────────────
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        7.0.12
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(JUCE)

# ── Plugin target ─────────────────────────────────────────────────────────────
juce_add_plugin(Couch1176
    COMPANY_NAME                "Couch"
    PLUGIN_MANUFACTURER_CODE    Couc
    PLUGIN_CODE                 C176
    FORMATS                     VST3 Standalone
    PRODUCT_NAME                "Couch 1176"
    BUNDLE_ID                   "com.couch.couch1176"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD     FALSE
    VST3_CATEGORIES             "Fx" "Dynamics"
)

juce_generate_juce_header(Couch1176)

# ── Binary data (images, etc.) ────────────────────────────────────────────────
juce_add_binary_data(Couch1176BinaryData SOURCES
    Assets/76-vu.png
    Assets/vu_sprite.png
    Assets/76-large.png
    Assets/76-small.png
)

# ── Sources ───────────────────────────────────────────────────────────────────
target_sources(Couch1176 PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
    Source/Dsp/Compressor1176.cpp
)

target_include_directories(Couch1176 PRIVATE Source)

# ── Compile definitions ───────────────────────────────────────────────────────
target_compile_definitions(Couch1176 PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
    JUCE_REPORT_APP_USAGE=0
)

# ── Link libraries ────────────────────────────────────────────────────────────
target_link_libraries(Couch1176
    PRIVATE
        Couch1176BinaryData
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
