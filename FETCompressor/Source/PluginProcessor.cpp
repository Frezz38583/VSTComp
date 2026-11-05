#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

AudioProcessorValueTreeState::ParameterLayout FETCompAudioProcessor::createLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> p;

    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "input", 1 },  "Input",
                 NormalisableRange<float> (-12.0f, 36.0f, 0.1f), 0.0f));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "output", 1 }, "Output",
                 NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));
    // Attack en ms : 20 us -> 800 us (comme un FET rapide)
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "attack", 1 }, "Attack",
                 NormalisableRange<float> (0.02f, 0.8f, 0.001f, 0.5f), 0.2f));
    // Release en ms : 50 -> 1100
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "release", 1 }, "Release",
                 NormalisableRange<float> (50.0f, 1100.0f, 1.0f, 0.5f), 300.0f));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "ratio", 1 }, "Ratio",
                 StringArray { "4", "8", "12", "20", "All" }, 0));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 },    "Mix",
                 NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "bypass", 1 },  "Bypass", false));

    return { p.begin(), p.end() };
}

FETCompAudioProcessor::FETCompAudioProcessor()
    : AudioProcessor (BusesProperties()
                        .withInput  ("Input",  AudioChannelSet::stereo(), true)
                        .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    pInput   = apvts.getRawParameterValue ("input");
    pOutput  = apvts.getRawParameterValue ("output");
    pAttack  = apvts.getRawParameterValue ("attack");
    pRelease = apvts.getRawParameterValue ("release");
    pRatio   = apvts.getRawParameterValue ("ratio");
    pMix     = apvts.getRawParameterValue ("mix");
    pBypass  = apvts.getRawParameterValue ("bypass");
}

void FETCompAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    sr = sampleRate;
    env = 0.0f;
    dry.setSize (2, jmax (samplesPerBlock, 1));

    inGain.reset  (sr, 0.02);  inGain.setCurrentAndTargetValue  (Decibels::decibelsToGain (pInput->load()));
    outGain.reset (sr, 0.02);  outGain.setCurrentAndTargetValue (Decibels::decibelsToGain (pOutput->load()));
    mixAmt.reset  (sr, 0.02);  mixAmt.setCurrentAndTargetValue  (pMix->load() * 0.01f);
}

bool FETCompAudioProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    auto out = l.getMainOutputChannelSet();
    if (out != AudioChannelSet::mono() && out != AudioChannelSet::stereo())
        return false;
    return out == l.getMainInputChannelSet();
}

void FETCompAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;

    const int numCh = jmin (buffer.getNumChannels(), 2);
    const int n     = buffer.getNumSamples();

    if (pBypass->load() > 0.5f)
    {
        grMeter.store (0.0f);
        return;
    }

    // Copie du signal sec pour le Mix (compression parallele)
    dry.setSize (2, n, false, false, true);
    for (int ch = 0; ch < numCh; ++ch)
        dry.copyFrom (ch, 0, buffer, ch, 0, n);

    // Parametres
    const int   ratioIdx = (int) pRatio->load();
    const bool  allMode  = (ratioIdx == 4);
    static const float ratios[] = { 4.0f, 8.0f, 12.0f, 20.0f, 20.0f };
    const float ratio    = ratios[jlimit (0, 4, ratioIdx)];
    const float thresh   = allMode ? -18.0f : -10.0f;   // dBFS (seuil fixe, comme un 1176)
    const float knee     = allMode ? 1.0f : 4.0f;
    const float atkMs    = allMode ? pAttack->load() * 0.5f : pAttack->load();
    const float relMs    = pRelease->load();

    const float aCoef = std::exp (-1.0f / (float) (atkMs * 0.001 * sr));
    const float rCoef = std::exp (-1.0f / (float) (relMs * 0.001 * sr));
    const float slope = 1.0f - 1.0f / ratio;

    inGain.setTargetValue  (Decibels::decibelsToGain (pInput->load()));
    outGain.setTargetValue (Decibels::decibelsToGain (pOutput->load()));
    mixAmt.setTargetValue  (pMix->load() * 0.01f);

    for (int i = 0; i < n; ++i)
    {
        const float g = inGain.getNextValue();
        const float o = outGain.getNextValue();
        const float m = mixAmt.getNextValue();

        // Detection stereo-linked (pic max des canaux)
        float peak = 0.0f;
        for (int ch = 0; ch < numCh; ++ch)
            peak = jmax (peak, std::abs (buffer.getSample (ch, i) * g));

        const float levelDb = Decibels::gainToDecibels (peak, -100.0f);
        const float over    = levelDb - thresh;

        // Calcul de la reduction de gain avec soft knee
        float gr = 0.0f;
        if (2.0f * over > knee)
            gr = over * slope;
        else if (2.0f * over > -knee)
        {
            const float x = over + knee * 0.5f;
            gr = slope * x * x / (2.0f * knee);
        }

        // Ballistique attack / release
        const float coef = (gr > env) ? aCoef : rCoef;
        env = coef * env + (1.0f - coef) * gr;

        const float compGain = Decibels::decibelsToGain (-env);

        for (int ch = 0; ch < numCh; ++ch)
        {
            const float x   = buffer.getSample (ch, i);
            const float wet = x * g * compGain * o;
            buffer.setSample (ch, i, dry.getSample (ch, i) * (1.0f - m) + wet * m);
        }
    }

    grMeter.store (env);
}

AudioProcessorEditor* FETCompAudioProcessor::createEditor()
{
    return new FETCompAudioProcessorEditor (*this);
}

void FETCompAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void FETCompAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (ValueTree::fromXml (*xml));
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new FETCompAudioProcessor();
}
