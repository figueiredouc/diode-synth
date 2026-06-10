#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// ── Step button ───────────────────────────────────────────────────────────────

class StepButton : public juce::Component
{
public:
    int   index     = 0;
    bool  isCurrent = false;

    std::function<void(int)> onToggle;

    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

    void update (bool enabled_, int note_, float vel_, bool current_)
    {
        if (enabled_ != enabled || note_ != note || vel_ != vel || current_ != isCurrent)
        {
            enabled   = enabled_;
            note      = note_;
            vel       = vel_;
            isCurrent = current_;
            repaint();
        }
    }

    static juce::String noteName(int n)
    {
        static const char* names[] = {"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};
        return juce::String(names[n % 12]) + juce::String(n / 12 - 1);
    }

private:
    bool  enabled = false;
    int   note    = 60;
    float vel     = 0.8f;
};

// ── Editor ────────────────────────────────────────────────────────────────────

class DiodeEditor : public juce::AudioProcessorEditor,
                    public juce::Timer
{
public:
    explicit DiodeEditor(DiodeProcessor&);
    ~DiodeEditor() override;

    void paint   (juce::Graphics&) override;
    void resized () override;
    void timerCallback() override;

private:
    DiodeProcessor& processor;

    // Oscillator section
    juce::ComboBox osc1Box,       osc2Box;
    juce::Label    osc1Label,     osc2Label;
    juce::Slider   osc1VolSlider, osc2VolSlider, detuneSlider;
    juce::Label    osc1VolLabel,  osc2VolLabel,  detuneLabel;

    // Filter section
    juce::ComboBox f1TypeBox,    f2TypeBox;
    juce::Label    f1TypeLabel,  f2TypeLabel;
    juce::Slider   f1CutSlider,  f1ResSlider;
    juce::Label    f1CutLabel,   f1ResLabel;
    juce::Slider   f2CutSlider,  f2ResSlider;
    juce::Label    f2CutLabel,   f2ResLabel;

    // Envelope section
    juce::Slider attackSlider, decaySlider, sustainSlider;
    juce::Label  attackLabel,  decayLabel,  sustainLabel;

    // Sequencer section
    StepButton   stepButtons[DiodeProcessor::SEQ_STEPS];
    juce::TextButton recButton  { "REC" };
    juce::Slider     gateSlider;
    juce::Label      gateLabel;
    juce::ComboBox   speedBox;
    juce::Label      speedLabel;

    using SliderAtt = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAtt  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    ComboAtt  osc1Attach, osc2Attach;
    SliderAtt osc1VolAttach, osc2VolAttach, detuneAttach;
    ComboAtt  f1TypeAttach, f2TypeAttach;
    SliderAtt f1CutAttach, f1ResAttach, f2CutAttach, f2ResAttach;
    SliderAtt attackAttach, decayAttach, sustainAttach;
    SliderAtt gateAttach;
    ComboAtt  speedAttach;

    void setupSlider (juce::Slider&, juce::Label&, const juce::String& name);
    void setupCombo  (juce::ComboBox&, juce::Label&, const juce::String& name);
    void updateRecButton();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DiodeEditor)
};
