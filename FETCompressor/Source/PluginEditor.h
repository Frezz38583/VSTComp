#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
// Look des potards : corps sombre, repere clair, graduations
class KnobLook : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override
    {
        auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
        const float r = juce::jmin (b.getWidth(), b.getHeight()) * 0.5f;
        const auto  c = b.getCentre();
        const float kr = r * 0.74f;

        // graduations
        g.setColour (juce::Colour (0xffb8b2a0));
        for (int i = 0; i <= 10; ++i)
        {
            const float a = startAngle + (float) i / 10.0f * (endAngle - startAngle);
            g.drawLine (juce::Line<float> (c.getPointOnCircumference (r * 0.86f, a),
                                           c.getPointOnCircumference (r, a)), i % 5 == 0 ? 2.0f : 1.0f);
        }

        // ombre + corps
        g.setColour (juce::Colours::black.withAlpha (0.5f));
        g.fillEllipse (c.x - kr + 2, c.y - kr + 3, kr * 2, kr * 2);
        juce::ColourGradient grad (juce::Colour (0xff5a5d62), c.x - kr * 0.5f, c.y - kr * 0.7f,
                                   juce::Colour (0xff17181a), c.x + kr * 0.6f, c.y + kr * 0.8f, false);
        g.setGradientFill (grad);
        g.fillEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2);
        g.setColour (juce::Colour (0xff0a0a0b));
        g.drawEllipse (c.x - kr, c.y - kr, kr * 2, kr * 2, 1.5f);

        // repere
        const float a = startAngle + pos * (endAngle - startAngle);
        g.setColour (juce::Colour (0xfff2e6c0));
        g.drawLine (juce::Line<float> (c.getPointOnCircumference (kr * 0.30f, a),
                                       c.getPointOnCircumference (kr * 0.92f, a)), 3.0f);
    }
};

//==============================================================================
// Vu-metre de gain reduction (0 a 20 dB)
class GRMeter : public juce::Component
{
public:
    void setValue (float grDb)
    {
        target = juce::jlimit (0.0f, 24.0f, grDb);
        shown += (target - shown) * 0.35f;   // inertie du vu-metre
        repaint();
    }

    void paint (juce::Graphics& g) override
    {
        auto b = getLocalBounds().toFloat();
        g.setColour (juce::Colour (0xff0d0d0e));
        g.fillRoundedRectangle (b, 8.0f);

        auto face = b.reduced (6.0f);
        g.setGradientFill (juce::ColourGradient::vertical (juce::Colour (0xfff1e0a2), face.getY(),
                                                           juce::Colour (0xffd2b96c), face.getBottom()));
        g.fillRoundedRectangle (face, 5.0f);

        g.saveState();
        g.reduceClipRegion (face.toNearestInt());

        const juce::Point<float> pivot (face.getCentreX(), face.getBottom() + 14.0f);
        const float r = face.getHeight() * 1.0f;
        const float sweep = juce::degreesToRadians (46.0f);

        auto angleFor = [&] (float db)
        {
            return sweep - (db / 20.0f) * (2.0f * sweep);   // 0 dB a droite, 20 dB a gauche
        };

        g.setColour (juce::Colour (0xff2a2418));
        g.setFont (11.0f);
        for (int db = 0; db <= 20; db += 2)
        {
            const float a = angleFor ((float) db);
            const bool major = (db % 4 == 0);
            g.drawLine (juce::Line<float> (pivot.getPointOnCircumference (r - (major ? 12.0f : 7.0f), a),
                                           pivot.getPointOnCircumference (r, a)), major ? 1.8f : 1.0f);
            if (db == 0 || db == 4 || db == 8 || db == 12 || db == 20)
            {
                auto p = pivot.getPointOnCircumference (r - 24.0f, a);
                g.drawText (juce::String (db), juce::Rectangle<float> (24, 14).withCentre (p),
                            juce::Justification::centred);
            }
        }

        // zone rouge
        g.setColour (juce::Colour (0xffb3261e));
        juce::Path red;
        red.addCentredArc (pivot.x, pivot.y, r - 3.0f, r - 3.0f, 0.0f,
                           angleFor (20.0f), angleFor (14.0f), true);
        g.strokePath (red, juce::PathStrokeType (3.0f));

        g.setColour (juce::Colour (0xff2a2418));
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        g.drawText ("GAIN REDUCTION", face.withTrimmedTop (face.getHeight() * 0.62f).toNearestInt(),
                    juce::Justification::centredTop);

        // aiguille
        const float a = angleFor (shown);
        g.setColour (juce::Colour (0xff1a1209));
        g.drawLine (juce::Line<float> (pivot, pivot.getPointOnCircumference (r - 4.0f, a)), 2.0f);

        g.restoreState();
    }

private:
    float target = 0.0f, shown = 0.0f;
};

//==============================================================================
class FETCompAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    private juce::Timer
{
public:
    explicit FETCompAudioProcessorEditor (FETCompAudioProcessor&);
    ~FETCompAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void setupKnob (juce::Slider&, juce::Label&, const juce::String& name);

    FETCompAudioProcessor& audioProcessor;
    KnobLook look;

    juce::Slider inputS, outputS, attackS, releaseS, mixS;
    juce::Label  inputL, outputL, attackL, releaseL, mixL, ratioL;
    juce::TextButton ratioBtn[5];
    juce::TextButton bypassBtn { "BYPASS" };
    GRMeter meter;

    using SA = juce::AudioProcessorValueTreeState::SliderAttachment;
    using BA = juce::AudioProcessorValueTreeState::ButtonAttachment;
    std::unique_ptr<SA> inputA, outputA, attackA, releaseA, mixA;
    std::unique_ptr<BA> bypassA;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FETCompAudioProcessorEditor)
};
