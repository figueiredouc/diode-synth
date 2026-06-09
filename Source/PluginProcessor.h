#pragma once
#include <JuceHeader.h>

// ── Waveform helpers ─────────────────────────────────────────────────────────

static inline float waveSample(double phase, int type)
{
    double t = phase / juce::MathConstants<double>::twoPi;
    switch (type)
    {
        case 1: return (float)(2.0 * t - 1.0);
        case 2: return t < 0.5 ? 1.0f : -1.0f;
        case 3: return (float)(t < 0.5 ? 4.0*t - 1.0 : 3.0 - 4.0*t);
        default: return (float)std::sin(phase);
    }
}

static inline juce::dsp::StateVariableTPTFilterType toFilterType(int i)
{
    switch (i)
    {
        case 1:  return juce::dsp::StateVariableTPTFilterType::highpass;
        case 2:  return juce::dsp::StateVariableTPTFilterType::bandpass;
        default: return juce::dsp::StateVariableTPTFilterType::lowpass;
    }
}

// ── Voice ────────────────────────────────────────────────────────────────────

struct EPVoice
{
    bool  active     = false;
    int   midiNote   = 0;
    float velocity   = 0.0f;

    double phase1 = 0.0, phase2 = 0.0;

    double attackTime   = 0.005;
    double decayTime    = 1.2;
    float  sustainLevel = 0.25f;

    enum class Stage { Attack, Decay, Sustain, Release, Off };
    Stage  stage    = Stage::Off;
    double envLevel = 0.0;
    double envRate  = 0.0;

    juce::dsp::StateVariableTPTFilter<float> filter1, filter2;

    void  prepare (double sampleRate);
    void  noteOn  (int note, float vel, double sampleRate,
                   double attack, double decay, float sustain);
    void  noteOff (double sampleRate);
    void  setFilter1 (int type, float cutoff, float res);
    void  setFilter2 (int type, float cutoff, float res);
    float process    (double sampleRate,
                      int osc1Type, int osc2Type, float osc2Detune,
                      float osc1Vol, float osc2Vol,
                      bool f1On, bool f2On);
};

// ── Sequencer ────────────────────────────────────────────────────────────────

struct SeqStep
{
    std::atomic<int>   note     { 60 };
    std::atomic<float> velocity { 0.8f };

    SeqStep() = default;
    SeqStep(const SeqStep& o) { note.store(o.note.load()); velocity.store(o.velocity.load()); }
    SeqStep& operator=(const SeqStep& o) { note.store(o.note.load()); velocity.store(o.velocity.load()); return *this; }
};

// ── Processor ────────────────────────────────────────────────────────────────

class AndreProcessor : public juce::AudioProcessor
{
public:
    AndreProcessor();
    ~AndreProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Andre"; }
    bool   acceptsMidi()  const override { return true; }
    bool   producesMidi() const override { return false; }
    bool   isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 2.0; }

    int  getNumPrograms()    override { return 1; }
    int  getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return "Default"; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int)   override;

    bool isBusesLayoutSupported(const BusesLayout&) const override;

    juce::AudioProcessorValueTreeState apvts;

    // Sequencer state (accessed by editor)
    static constexpr int SEQ_STEPS = 8;
    SeqStep              seqSteps[SEQ_STEPS];
    std::atomic<int>     seqCurrentStep  { -1 };
    std::atomic<bool>    seqRecording    { false };
    std::atomic<int>     seqRecordIndex  { 0 };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    static juce::String stepEnabledId(int i) { return "s" + juce::String(i) + "en"; }

    static constexpr int MAX_VOICES = 8;
    EPVoice voices[MAX_VOICES];
    double  currentSampleRate = 44100.0;

    // Sequencer runtime (audio thread only)
    int  seqLastStep       = -1;
    int  seqActiveNote     = -1;
    int  noteOffCountdown  = 0;

    EPVoice* findFreeVoice();
    void     seqNoteOff();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AndreProcessor)
};
