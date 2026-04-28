#include "PluginProcessor.h"
#include "PluginEditor.h"

// ── Parameter layout ──────────────────────────────────────────────────────────

juce::AudioProcessorValueTreeState::ParameterLayout
Couch1176Processor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // INPUT  0–100  (drives signal into compressor, effectively lowers threshold)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "input", 1 }, "Input",
        juce::NormalisableRange<float> (0.f, 100.f, 0.01f), 50.f));

    // OUTPUT 0–100  (makeup gain)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "output", 1 }, "Output",
        juce::NormalisableRange<float> (0.f, 100.f, 0.01f), 50.f));

    // ATTACK 0–100  (100 = fastest, 0 = slowest — matches hardware CW = faster)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "attack", 1 }, "Attack",
        juce::NormalisableRange<float> (0.f, 100.f, 0.01f), 50.f));

    // RELEASE 0–100  (100 = fastest, 0 = slowest)
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "release", 1 }, "Release",
        juce::NormalisableRange<float> (0.f, 100.f, 0.01f), 50.f));

    // RATIO  0=4:1  1=8:1  2=12:1  3=20:1  4=ALL
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID { "ratio", 1 }, "Ratio", 0, 4, 0));

    return { params.begin(), params.end() };
}

// ── Constructor / Destructor ───────────────────────────────────────────────────

Couch1176Processor::Couch1176Processor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "Couch1176State", createParameterLayout())
{
    pInput   = apvts.getRawParameterValue ("input");
    pOutput  = apvts.getRawParameterValue ("output");
    pAttack  = apvts.getRawParameterValue ("attack");
    pRelease = apvts.getRawParameterValue ("release");
    pRatio   = apvts.getRawParameterValue ("ratio");
}

// ── Playback lifecycle ────────────────────────────────────────────────────────

void Couch1176Processor::prepareToPlay (double sampleRate, int /*samplesPerBlock*/)
{
    compressor.prepare (sampleRate);
}

void Couch1176Processor::releaseResources()
{
    compressor.reset();
}

bool Couch1176Processor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // Stereo or mono in/out only
    const auto& main = layouts.getMainOutputChannelSet();
    if (main != juce::AudioChannelSet::mono()
        && main != juce::AudioChannelSet::stereo())
        return false;

    return main == layouts.getMainInputChannelSet();
}

// ── Audio processing ──────────────────────────────────────────────────────────

void Couch1176Processor::processBlock (juce::AudioBuffer<float>& buffer,
                                       juce::MidiBuffer& /*midi*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = std::min (getTotalNumOutputChannels(), 2);

    // ── Translate 0–100 knob positions to DSP values ────────────────────────

    // Input:  0→0 dB, 100→+44 dB  (linear)
    compressor.inputGainDb.store (pInput->load() * 0.44f);

    // Output: 0→–20 dB, 100→+32 dB
    compressor.outputGainDb.store (pOutput->load() * 0.52f - 20.f);

    // Attack:  0→800 µs (slowest), 100→20 µs (fastest) — log taper
    {
        const float norm = pAttack->load() * 0.01f;
        // 0.0008 * (0.025)^norm : at 0 → 800µs, at 1 → 20µs
        compressor.attackTime.store (0.0008f * std::pow (0.025f, norm));
    }

    // Release: 0→1100 ms (slowest), 100→50 ms (fastest) — log taper
    {
        const float norm = pRelease->load() * 0.01f;
        // 1.1 * (0.04545)^norm : at 0 → 1.1 s, at 1 → 50 ms
        compressor.releaseTime.store (1.1f * std::pow (0.04545f, norm));
    }

    compressor.ratioMode.store (static_cast<int> (pRatio->load()));

    // ── Process ─────────────────────────────────────────────────────────────
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = buffer.getWritePointer (ch);
        for (int s = 0; s < numSamples; ++s)
            data[s] = compressor.process (data[s], ch);
    }

    // Clear any unused output channels
    for (int ch = numChannels; ch < getTotalNumOutputChannels(); ++ch)
        buffer.clear (ch, 0, numSamples);
}

// ── Editor ────────────────────────────────────────────────────────────────────

juce::AudioProcessorEditor* Couch1176Processor::createEditor()
{
    return new Couch1176Editor (*this);
}

// ── State ─────────────────────────────────────────────────────────────────────

void Couch1176Processor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void Couch1176Processor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

// ── Plugin entry point ────────────────────────────────────────────────────────

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Couch1176Processor();
}
