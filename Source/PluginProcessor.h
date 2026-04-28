#pragma once
#include <JuceHeader.h>
#include "Dsp/Compressor1176.h"

class Couch1176Processor final : public juce::AudioProcessor
{
public:
    Couch1176Processor();
    ~Couch1176Processor() override = default;

    // ── AudioProcessor overrides ──────────────────────────────────────────────
    void prepareToPlay  (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;   // expose the double-buffer variant

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor()  const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool   acceptsMidi()  const override { return false; }
    bool   producesMidi() const override { return false; }
    bool   isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int  getNumPrograms()  override { return 1; }
    int  getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    // ── Parameter state ───────────────────────────────────────────────────────
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // ── Metering ──────────────────────────────────────────────────────────────
    /** Returns current smoothed gain reduction in dB (positive = reduction). */
    float getGainReductionDb() const noexcept { return compressor.getGainReductionDb(); }
    float getOutputLevelDb()   const noexcept { return compressor.getOutputLevelDb();   }

private:
    Compressor1176 compressor;

    // Raw parameter pointers (set in ctor, safe to read on audio thread)
    std::atomic<float>* pInput   = nullptr;
    std::atomic<float>* pOutput  = nullptr;
    std::atomic<float>* pAttack  = nullptr;
    std::atomic<float>* pRelease = nullptr;
    std::atomic<float>* pRatio   = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Couch1176Processor)
};
