#include "PluginEditor.h"

using namespace juce;

void FETCompAudioProcessorEditor::setupKnob (Slider& s, Label& l, const String& name)
{
    s.setSliderStyle (Slider::RotaryHorizontalVerticalDrag);
    s.setTextBoxStyle (Slider::TextBoxBelow, false, 80, 16);
    s.setRotaryParameters (degreesToRadians (225.0f), degreesToRadians (495.0f), true);
    s.setColour (Slider::textBoxTextColourId, Colour (0xffe8dfc4));
    s.setColour (Slider::textBoxOutlineColourId, Colours::transparentBlack);
    s.setLookAndFeel (&look);
    addAndMakeVisible (s);

    l.setText (name, dontSendNotification);
    l.setJustificationType (Justification::centred);
    l.setFont (FontOptions (14.0f, Font::bold));
    l.setColour (Label::textColourId, Colour (0xffe8dfc4));
    addAndMakeVisible (l);
}

FETCompAudioProcessorEditor::FETCompAudioProcessorEditor (FETCompAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setupKnob (inputS,   inputL,   "INPUT");
    setupKnob (outputS,  outputL,  "OUTPUT");
    setupKnob (attackS,  attackL,  "ATTACK");
    setupKnob (releaseS, releaseL, "RELEASE");
    setupKnob (mixS,     mixL,     "MIX");

    inputS.textFromValueFunction   = [] (double v) { return String (v, 1) + " dB"; };
    outputS.textFromValueFunction  = [] (double v) { return String (v, 1) + " dB"; };
    attackS.textFromValueFunction  = [] (double v) { return String (roundToInt (v * 1000.0)) + " us"; };
    releaseS.textFromValueFunction = [] (double v) { return String (roundToInt (v)) + " ms"; };
    mixS.textFromValueFunction     = [] (double v) { return String (roundToInt (v)) + " %"; };

    inputA   = std::make_unique<SA> (p.apvts, "input",   inputS);
    outputA  = std::make_unique<SA> (p.apvts, "output",  outputS);
    attackA  = std::make_unique<SA> (p.apvts, "attack",  attackS);
    releaseA = std::make_unique<SA> (p.apvts, "release", releaseS);
    mixA     = std::make_unique<SA> (p.apvts, "mix",     mixS);

    // Boutons de ratio (groupe radio)
    const char* names[] = { "4", "8", "12", "20", "ALL" };
    for (int i = 0; i < 5; ++i)
    {
        auto& b = ratioBtn[i];
        b.setButtonText (names[i]);
        b.setClickingTogglesState (true);
        b.setRadioGroupId (1001);
        b.setColour (TextButton::buttonColourId,   Colour (0xff202225));
        b.setColour (TextButton::buttonOnColourId, Colour (0xffe0a526));
        b.setColour (TextButton::textColourOffId,  Colour (0xffe8dfc4));
        b.setColour (TextButton::textColourOnId,   Colours::black);
        b.onClick = [this, i]
        {
            if (auto* prm = audioProcessor.apvts.getParameter ("ratio"))
            {
                prm->beginChangeGesture();
                prm->setValueNotifyingHost (prm->convertTo0to1 ((float) i));
                prm->endChangeGesture();
            }
        };
        addAndMakeVisible (b);
    }

    ratioL.setText ("RATIO", dontSendNotification);
    ratioL.setJustificationType (Justification::centred);
    ratioL.setFont (FontOptions (14.0f, Font::bold));
    ratioL.setColour (Label::textColourId, Colour (0xffe8dfc4));
    addAndMakeVisible (ratioL);

    bypassBtn.setClickingTogglesState (true);
    bypassBtn.setColour (TextButton::buttonColourId,   Colour (0xff202225));
    bypassBtn.setColour (TextButton::buttonOnColourId, Colour (0xffb3261e));
    bypassBtn.setColour (TextButton::textColourOffId,  Colour (0xffe8dfc4));
    bypassBtn.setColour (TextButton::textColourOnId,   Colours::white);
    addAndMakeVisible (bypassBtn);
    bypassA = std::make_unique<BA> (p.apvts, "bypass", bypassBtn);

    addAndMakeVisible (meter);

    setSize (760, 430);
    startTimerHz (30);
    timerCallback();
}

FETCompAudioProcessorEditor::~FETCompAudioProcessorEditor()
{
    for (auto* s : { &inputS, &outputS, &attackS, &releaseS, &mixS })
        s->setLookAndFeel (nullptr);
}

void FETCompAudioProcessorEditor::timerCallback()
{
    meter.setValue (audioProcessor.grMeter.load());

    const int idx = (int) audioProcessor.apvts.getRawParameterValue ("ratio")->load();
    for (int i = 0; i < 5; ++i)
        ratioBtn[i].setToggleState (i == idx, dontSendNotification);
}

void FETCompAudioProcessorEditor::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setGradientFill (ColourGradient::vertical (Colour (0xff2e3033), 0.0f, Colour (0xff141517), b.getBottom()));
    g.fillAll();

    g.setColour (Colour (0xff050506));
    g.drawRect (b, 3.0f);
    g.setColour (Colour (0xffe0a526));
    g.fillRect (16.0f, 52.0f, b.getWidth() - 32.0f, 2.0f);

    g.setColour (Colour (0xffe8dfc4));
    g.setFont (FontOptions (26.0f, Font::bold));
    g.drawText ("FET COMPRESSOR", 20, 12, 360, 34, Justification::centredLeft);
    g.setFont (FontOptions (11.0f));
    g.setColour (Colour (0xff9a9482));
    g.drawText ("Classic FET-style limiting amplifier", 20, 36, 360, 14, Justification::centredLeft);
}

void FETCompAudioProcessorEditor::resized()
{
    auto r = getLocalBounds().reduced (16);

    auto head = r.removeFromTop (44);
    bypassBtn.setBounds (head.removeFromRight (90).withSizeKeepingCentre (90, 28));
    r.removeFromTop (14);

    auto place = [] (Slider& s, Label& l, Rectangle<int> area)
    {
        l.setBounds (area.removeFromTop (20));
        s.setBounds (area);
    };

    // Rangee du haut : Input | Meter | Output
    auto top = r.removeFromTop (190);
    place (inputS,  inputL,  top.removeFromLeft (170));
    place (outputS, outputL, top.removeFromRight (170));
    meter.setBounds (top.reduced (14, 8));

    r.removeFromTop (14);

    // Rangee du bas : Attack | Ratio | Release | Mix
    auto bottom = r;
    place (attackS,  attackL,  bottom.removeFromLeft (130));
    place (mixS,     mixL,     bottom.removeFromRight (130));
    place (releaseS, releaseL, bottom.removeFromRight (130));

    auto ratioArea = bottom.reduced (10, 0);
    ratioL.setBounds (ratioArea.removeFromTop (20));
    auto row = ratioArea.withSizeKeepingCentre (ratioArea.getWidth(), 44);
    const int w = (row.getWidth() - 4 * 6) / 5;
    for (int i = 0; i < 5; ++i)
    {
        ratioBtn[i].setBounds (row.removeFromLeft (w));
        row.removeFromLeft (6);
    }
}
