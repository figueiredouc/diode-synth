#include "PluginProcessor.h"
#include "PluginEditor.h"

// ── Voice ────────────────────────────────────────────────────────────────────

void EPVoice::prepare(double sampleRate)
{
    juce::dsp::ProcessSpec spec { sampleRate, 1, 1 };
    filter1.prepare(spec); filter1.reset();
    filter2.prepare(spec); filter2.reset();
}

void EPVoice::noteOn(int note, float vel, double sampleRate,
                     double attack, double decay, float sustain)
{
    midiNote     = note;
    velocity     = vel;
    active       = true;
    phase1       = phase2 = 0.0;
    envLevel     = 0.0;
    attackTime   = attack;
    decayTime    = decay;
    sustainLevel = sustain;
    stage        = Stage::Attack;
    envRate      = 1.0 / (attack * sampleRate);
    filter1.reset(); filter2.reset();
}

void EPVoice::noteOff(double sampleRate)
{
    if (stage == Stage::Off) return;
    if (envLevel < 0.001) { active = false; stage = Stage::Off; return; }
    stage   = Stage::Release;
    envRate = envLevel / (0.4 * sampleRate);
}

void EPVoice::setFilter1(int type, float cutoff, float res)
{
    filter1.setType(toFilterType(type));
    filter1.setCutoffFrequency(juce::jlimit(20.0f, 20000.0f, cutoff));
    filter1.setResonance(juce::jlimit(0.1f, 8.0f, res));
}

void EPVoice::setFilter2(int type, float cutoff, float res)
{
    filter2.setType(toFilterType(type));
    filter2.setCutoffFrequency(juce::jlimit(20.0f, 20000.0f, cutoff));
    filter2.setResonance(juce::jlimit(0.1f, 8.0f, res));
}

float EPVoice::process(double sampleRate,
                       int osc1Type, int osc2Type, float osc2Detune,
                       float osc1Vol, float osc2Vol,
                       bool f1On, bool f2On)
{
    if (!active) return 0.0f;

    switch (stage)
    {
        case Stage::Attack:
            envLevel += envRate;
            if (envLevel >= 1.0) { envLevel = 1.0; stage = Stage::Decay;
                                   envRate = (1.0 - sustainLevel) / (decayTime * sampleRate); }
            break;
        case Stage::Decay:
            envLevel -= envRate;
            if (envLevel <= sustainLevel) { envLevel = sustainLevel; stage = Stage::Sustain; }
            break;
        case Stage::Sustain:
            envLevel = sustainLevel;
            break;
        case Stage::Release:
            envLevel -= envRate;
            if (envLevel <= 0.0) { envLevel = 0.0; active = false; stage = Stage::Off; return 0.0f; }
            break;
        case Stage::Off:
            return 0.0f;
    }

    double twoPi = juce::MathConstants<double>::twoPi;
    double f1    = 440.0 * std::pow(2.0, (midiNote - 69) / 12.0);
    double f2    = f1    * std::pow(2.0, osc2Detune / 1200.0);
    phase1 += twoPi * f1 / sampleRate; if (phase1 > twoPi) phase1 -= twoPi;
    phase2 += twoPi * f2 / sampleRate; if (phase2 > twoPi) phase2 -= twoPi;

    float s1 = waveSample(phase1, osc1Type);
    float s2 = waveSample(phase2, osc2Type);
    if (f1On) s1 = filter1.processSample(0, s1);
    if (f2On) s2 = filter2.processSample(0, s2);

    return (s1 * osc1Vol + s2 * osc2Vol) * 0.5f * (float)envLevel * velocity * 0.3f;
}

// ── Processor ────────────────────────────────────────────────────────────────

juce::AudioProcessorValueTreeState::ParameterLayout DiodeProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    juce::StringArray waves  { "Sine", "Saw", "Square", "Triangle" };
    juce::StringArray ftypes { "Off", "LP", "HP", "BP" };

    // Oscillators
    params.push_back(std::make_unique<juce::AudioParameterChoice>("osc1Type",   "Osc 1",   waves,  0));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("osc2Type",   "Osc 2",   waves,  1));
    params.push_back(std::make_unique<juce::AudioParameterFloat> ("osc2Detune", "Detune",
        juce::NormalisableRange<float>(-50.0f, 50.0f, 0.1f), 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat> ("osc1Vol",    "Vol 1",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat> ("osc2Vol",    "Vol 2",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f), 1.0f));

    // Filters
    params.push_back(std::make_unique<juce::AudioParameterChoice>("f1Type",   "Filter 1", ftypes, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat> ("f1Cutoff", "Cutoff 1",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 0.0f, 0.3f), 20000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat> ("f1Res",    "Res 1",
        juce::NormalisableRange<float>(0.1f, 8.0f, 0.01f), 0.7f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>("f2Type",   "Filter 2", ftypes, 0));
    params.push_back(std::make_unique<juce::AudioParameterFloat> ("f2Cutoff", "Cutoff 2",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 0.0f, 0.3f), 20000.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat> ("f2Res",    "Res 2",
        juce::NormalisableRange<float>(0.1f, 8.0f, 0.01f), 0.7f));

    // Envelope
    params.push_back(std::make_unique<juce::AudioParameterFloat>("attack",  "Attack",
        juce::NormalisableRange<float>(0.001f, 2.0f, 0.001f, 0.4f), 0.005f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("decay",   "Decay",
        juce::NormalisableRange<float>(0.01f,  5.0f, 0.01f,  0.4f), 1.2f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("sustain", "Sustain",
        juce::NormalisableRange<float>(0.0f, 10.0f, 0.01f), 2.5f));

    // Sequencer
    params.push_back(std::make_unique<juce::AudioParameterFloat>("gate", "Gate",
        juce::NormalisableRange<float>(0.05f, 0.95f, 0.01f), 0.5f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        "seqSpeed", "Speed",
        juce::StringArray { "1/2", "1x", "2x", "4x" }, 1));
    // index: 0=half, 1=normal(default), 2=double, 3=quad
    for (int i = 0; i < SEQ_STEPS; ++i)
        params.push_back(std::make_unique<juce::AudioParameterBool>(
            stepEnabledId(i), "Step " + juce::String(i + 1), false));

    return { params.begin(), params.end() };
}

DiodeProcessor::DiodeProcessor()
    : AudioProcessor(BusesProperties()
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{}

bool DiodeProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        || layouts.getMainOutputChannelSet() == juce::AudioChannelSet::mono();
}

void DiodeProcessor::prepareToPlay(double sampleRate, int)
{
    currentSampleRate = sampleRate;
    for (auto& v : voices) { v = EPVoice{}; v.prepare(sampleRate); }
    seqLastStep = -1; seqActiveNote = -1; noteOffCountdown = 0;
}

EPVoice* DiodeProcessor::findFreeVoice()
{
    for (auto& v : voices) if (!v.active) return &v;
    voices[0].active = false; voices[0].stage = EPVoice::Stage::Off;
    return &voices[0];
}

void DiodeProcessor::seqNoteOff()
{
    if (seqActiveNote < 0) return;
    for (auto& v : voices)
        if (v.active && v.midiNote == seqActiveNote)
            v.noteOff(currentSampleRate);
    seqActiveNote = -1;
}

void DiodeProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                   juce::MidiBuffer& midiMessages)
{
    buffer.clear();
    const int numSamples = buffer.getNumSamples();

    // Read synth params once per block
    float attack  = apvts.getRawParameterValue("attack")   ->load();
    float decay   = apvts.getRawParameterValue("decay")    ->load();
    float sustain = apvts.getRawParameterValue("sustain")  ->load() / 10.0f;
    int   osc1T   = (int)apvts.getRawParameterValue("osc1Type") ->load();
    int   osc2T   = (int)apvts.getRawParameterValue("osc2Type") ->load();
    float detune  = apvts.getRawParameterValue("osc2Detune")->load();
    float osc1Vol = apvts.getRawParameterValue("osc1Vol")  ->load();
    float osc2Vol = apvts.getRawParameterValue("osc2Vol")  ->load();
    int   f1Type  = (int)apvts.getRawParameterValue("f1Type")->load();
    float f1Cut   = apvts.getRawParameterValue("f1Cutoff") ->load();
    float f1Res   = apvts.getRawParameterValue("f1Res")    ->load();
    int   f2Type  = (int)apvts.getRawParameterValue("f2Type")->load();
    float f2Cut   = apvts.getRawParameterValue("f2Cutoff") ->load();
    float f2Res   = apvts.getRawParameterValue("f2Res")    ->load();
    float gate    = apvts.getRawParameterValue("gate")     ->load();
    int   speedIdx = (int)apvts.getRawParameterValue("seqSpeed")->load();
    static const double speedTable[] = { 0.5, 1.0, 2.0, 4.0 };
    double stepsPerBeat = speedTable[juce::jlimit(0, 3, speedIdx)];
    bool  f1On    = f1Type > 0;
    bool  f2On    = f2Type > 0;

    for (auto& v : voices)
    {
        if (f1On) v.setFilter1(f1Type - 1, f1Cut, f1Res);
        if (f2On) v.setFilter2(f2Type - 1, f2Cut, f2Res);
    }

    // ── Sequencer step advance ───────────────────────────────────────────
    if (auto* ph = getPlayHead())
    {
        if (auto pos = ph->getPosition())
        {
            bool playing = pos->getIsPlaying();
            auto ppqOpt  = pos->getPpqPosition();
            auto bpmOpt  = pos->getBpm();

            if (playing && ppqOpt.hasValue() && bpmOpt.hasValue())
            {
                double ppq      = *ppqOpt;
                double bpm      = *bpmOpt;
                double scaledPpq = ppq * stepsPerBeat;
                int    step = (((int)std::floor(scaledPpq)) % SEQ_STEPS + SEQ_STEPS) % SEQ_STEPS;

                if (step != seqLastStep)
                {
                    seqNoteOff();
                    noteOffCountdown = 0;
                    seqLastStep = step;
                    seqCurrentStep.store(step);

                    bool enabled = apvts.getRawParameterValue(stepEnabledId(step))->load() > 0.5f;
                    if (enabled)
                    {
                        int   note = seqSteps[step].note.load();
                        float vel  = seqSteps[step].velocity.load();
                        findFreeVoice()->noteOn(note, vel, currentSampleRate, attack, decay, sustain);
                        seqActiveNote    = note;
                        noteOffCountdown = (int)(gate * (60.0 / bpm) / stepsPerBeat * currentSampleRate);
                    }
                }

                // Gate countdown
                if (noteOffCountdown > 0)
                {
                    noteOffCountdown -= numSamples;
                    if (noteOffCountdown <= 0) { noteOffCountdown = 0; seqNoteOff(); }
                }
            }
            else if (!playing)
            {
                seqNoteOff();
                seqLastStep = -1;
                seqCurrentStep.store(-1);
                noteOffCountdown = 0;
            }
        }
    }

    // ── MIDI input (record or passthrough) ───────────────────────────────
    bool recording = seqRecording.load();
    int  samplePos = 0;

    for (const auto meta : midiMessages)
    {
        auto msg       = meta.getMessage();
        int  msgSample = meta.samplePosition;

        // Render audio up to this event
        for (int s = samplePos; s < msgSample; ++s)
        {
            float out = 0.0f;
            for (auto& v : voices)
                out += v.process(currentSampleRate, osc1T, osc2T, detune, osc1Vol, osc2Vol, f1On, f2On);
            for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
                buffer.addSample(ch, s, out);
        }
        samplePos = msgSample;

        if (msg.isNoteOn())
        {
            if (recording)
            {
                // Capture note into next step slot
                int idx = seqRecordIndex.load();
                if (idx < SEQ_STEPS)
                {
                    seqSteps[idx].note.store(msg.getNoteNumber());
                    seqSteps[idx].velocity.store(msg.getFloatVelocity());
                    seqRecordIndex.store(idx + 1);
                    // Auto-enable this step
                    if (auto* p = dynamic_cast<juce::AudioParameterBool*>(
                            apvts.getParameter(stepEnabledId(idx))))
                        *p = true;
                    // Stop recording when all steps filled
                    if (idx + 1 >= SEQ_STEPS)
                        seqRecording.store(false);
                }
                // Also play note through synth for audible feedback
                for (auto& v : voices)
                    if (v.active && v.midiNote == msg.getNoteNumber())
                        v.noteOff(currentSampleRate);
                findFreeVoice()->noteOn(msg.getNoteNumber(), msg.getFloatVelocity(),
                                        currentSampleRate, attack, decay, sustain);
            }
            else
            {
                for (auto& v : voices)
                    if (v.active && v.midiNote == msg.getNoteNumber())
                        v.noteOff(currentSampleRate);
                findFreeVoice()->noteOn(msg.getNoteNumber(), msg.getFloatVelocity(),
                                        currentSampleRate, attack, decay, sustain);
            }
        }
        else if (msg.isNoteOff())
        {
            for (auto& v : voices)
                if (v.active && v.midiNote == msg.getNoteNumber())
                    v.noteOff(currentSampleRate);
        }
        else if (msg.isAllNotesOff() || msg.isAllSoundOff())
        {
            for (auto& v : voices) { v.active = false; v.stage = EPVoice::Stage::Off; }
        }
    }

    // Render remaining samples
    for (int s = samplePos; s < numSamples; ++s)
    {
        float out = 0.0f;
        for (auto& v : voices)
            out += v.process(currentSampleRate, osc1T, osc2T, detune, osc1Vol, osc2Vol, f1On, f2On);
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            buffer.addSample(ch, s, out);
    }
}

void DiodeProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    auto state = apvts.copyState();
    auto seqTree = juce::ValueTree("SEQ");
    for (int i = 0; i < SEQ_STEPS; ++i)
    {
        seqTree.setProperty("note" + juce::String(i), seqSteps[i].note.load(),     nullptr);
        seqTree.setProperty("vel"  + juce::String(i), seqSteps[i].velocity.load(), nullptr);
    }
    state.addChild(seqTree, -1, nullptr);
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, dest);
}

void DiodeProcessor::setStateInformation(const void* data, int size)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, size));
    if (!xml || !xml->hasTagName(apvts.state.getType())) return;
    auto state = juce::ValueTree::fromXml(*xml);
    auto seqTree = state.getChildWithName("SEQ");
    if (seqTree.isValid())
    {
        for (int i = 0; i < SEQ_STEPS; ++i)
        {
            seqSteps[i].note    .store((int)  seqTree.getProperty("note" + juce::String(i), 60));
            seqSteps[i].velocity.store((float)seqTree.getProperty("vel"  + juce::String(i), 0.8f));
        }
        state.removeChild(seqTree, nullptr);
    }
    apvts.replaceState(state);
}

juce::AudioProcessorEditor* DiodeProcessor::createEditor()
{
    return new DiodeEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DiodeProcessor();
}
