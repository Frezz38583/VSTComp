#pragma once
#include <JuceHeader.h>

class FETCompAudioProcessor : public juce::AudioProcessor
{
public:
    FETCompAudioProcessor();
    ~FETCompAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "FET Compressor"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<float> grMeter { 0.0f };   // gain reduction en dB (positif)

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    double sr = 44100.0;
    float env = 0.0f;                       // enveloppe de gain reduction (dB)
    juce::AudioBuffer<float> dry;
    juce::SmoothedValue<float> inGain, outGain, mixAmt;

    std::atomic<float>* pInput   = nullptr;
    std::atomic<float>* pOutput  = nullptr;
    std::atomic<float>* pAttack  = nullptr;
    std::atomic<float>* pRelease = nullptr;
    std::atomic<float>* pRatio   = nullptr;
    std::atomic<float>* pMix     = nullptr;
    std::atomic<float>* pBypass  = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FETCompAudioProcessor)
};
