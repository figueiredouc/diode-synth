#include "PluginEditor.h"

static const juce::Colour BG        { 0xff1a1a2e };
static const juce::Colour PANEL     { 0xff0d0d1a };
static const juce::Colour GOLD      { 0xffe0a000 };
static const juce::Colour BLUE      { 0xff4a9eff };
static const juce::Colour BLUE_RIM  { 0xff2a5a8a };
static const juce::Colour GREEN     { 0xff44cc88 };
static const juce::Colour GREEN_RIM { 0xff226644 };
static const juce::Colour RED       { 0xffdd4444 };
static const juce::Colour DIM       { 0xffaaaaaa };
static const juce::Colour WHITE     { 0xffffffff };

// ── StepButton ───────────────────────────────────────────────────────────────

void StepButton::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced(2.0f);

    juce::Colour bg = isCurrent ? GOLD
                     : enabled  ? BLUE.withAlpha(0.7f)
                                : juce::Colour(0xff252535);
    g.setColour(bg);
    g.fillRoundedRectangle(bounds, 6.0f);

    g.setColour(isCurrent ? juce::Colour(0xffaa7700) : BLUE_RIM);
    g.drawRoundedRectangle(bounds, 6.0f, 1.5f);

    // Note name
    g.setColour(enabled || isCurrent ? WHITE : DIM);
    g.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    g.drawFittedText(noteName(note),
                     getLocalBounds().withTrimmedBottom(18).withTrimmedTop(4),
                     juce::Justification::centred, 1);

    // Velocity bar
    float barW = (getWidth() - 8.0f) * vel;
    g.setColour((isCurrent ? GOLD : BLUE).withAlpha(0.5f));
    g.fillRoundedRectangle(4.0f, (float)getHeight() - 12.0f, barW, 6.0f, 2.0f);

    // Step number
    g.setColour(DIM.withAlpha(0.6f));
    g.setFont(juce::FontOptions(9.0f));
    g.drawText(juce::String(index + 1),
               getLocalBounds().removeFromBottom(14),
               juce::Justification::centred);
}

void StepButton::mouseDown(const juce::MouseEvent&)
{
    enabled = !enabled;
    repaint();
    if (onToggle) onToggle(index);
}

// ── DiodeEditor ──────────────────────────────────────────────────────────────

DiodeEditor::DiodeEditor(DiodeProcessor& p)
    : AudioProcessorEditor(&p), processor(p),
      osc1Attach    (p.apvts, "osc1Type",   osc1Box),
      osc2Attach    (p.apvts, "osc2Type",   osc2Box),
      osc1VolAttach (p.apvts, "osc1Vol",    osc1VolSlider),
      osc2VolAttach (p.apvts, "osc2Vol",    osc2VolSlider),
      detuneAttach  (p.apvts, "osc2Detune", detuneSlider),
      f1TypeAttach  (p.apvts, "f1Type",     f1TypeBox),
      f2TypeAttach  (p.apvts, "f2Type",     f2TypeBox),
      f1CutAttach   (p.apvts, "f1Cutoff",   f1CutSlider),
      f1ResAttach   (p.apvts, "f1Res",      f1ResSlider),
      f2CutAttach   (p.apvts, "f2Cutoff",   f2CutSlider),
      f2ResAttach   (p.apvts, "f2Res",      f2ResSlider),
      attackAttach  (p.apvts, "attack",     attackSlider),
      decayAttach   (p.apvts, "decay",      decaySlider),
      sustainAttach (p.apvts, "sustain",    sustainSlider),
      gateAttach    (p.apvts, "gate",       gateSlider),
      speedAttach   (p.apvts, "seqSpeed",  speedBox)
{
    // Osc
    setupCombo(osc1Box, osc1Label, "Osc 1"); osc1Box.addItemList({"Sine","Saw","Square","Triangle"}, 1);
    setupCombo(osc2Box, osc2Label, "Osc 2"); osc2Box.addItemList({"Sine","Saw","Square","Triangle"}, 1);
    setupSlider(osc1VolSlider, osc1VolLabel, "Vol 1");
    setupSlider(osc2VolSlider, osc2VolLabel, "Vol 2");
    setupSlider(detuneSlider,  detuneLabel,  "Detune");

    // Filters
    setupCombo(f1TypeBox, f1TypeLabel, "Filter 1"); f1TypeBox.addItemList({"Off","LP","HP","BP"}, 1);
    setupCombo(f2TypeBox, f2TypeLabel, "Filter 2"); f2TypeBox.addItemList({"Off","LP","HP","BP"}, 1);
    setupSlider(f1CutSlider, f1CutLabel, "Cutoff"); setupSlider(f1ResSlider, f1ResLabel, "Res");
    setupSlider(f2CutSlider, f2CutLabel, "Cutoff"); setupSlider(f2ResSlider, f2ResLabel, "Res");
    for (auto* s : { &f1CutSlider, &f1ResSlider, &f2CutSlider, &f2ResSlider })
    {
        s->setColour(juce::Slider::rotarySliderFillColourId,    GREEN);
        s->setColour(juce::Slider::rotarySliderOutlineColourId, GREEN_RIM);
    }

    // Envelope
    setupSlider(attackSlider,  attackLabel,  "Attack");
    setupSlider(decaySlider,   decayLabel,   "Decay");
    setupSlider(sustainSlider, sustainLabel, "Sustain");

    // Sequencer - step buttons
    for (int i = 0; i < DiodeProcessor::SEQ_STEPS; ++i)
    {
        stepButtons[i].index = i;
        stepButtons[i].onToggle = [this](int idx)
        {
            if (auto* param = dynamic_cast<juce::AudioParameterBool*>(
                    processor.apvts.getParameter("s" + juce::String(idx) + "en")))
                *param = !(*param);
        };
        addAndMakeVisible(stepButtons[i]);
    }

    // REC button
    recButton.setColour(juce::TextButton::buttonColourId,   juce::Colour(0xff333355));
    recButton.setColour(juce::TextButton::buttonOnColourId, RED);
    recButton.setColour(juce::TextButton::textColourOnId,   WHITE);
    recButton.setColour(juce::TextButton::textColourOffId,  WHITE);
    recButton.onClick = [this]
    {
        bool nowRec = !processor.seqRecording.load();
        if (nowRec) processor.seqRecordIndex.store(0);
        processor.seqRecording.store(nowRec);
        updateRecButton();
    };
    addAndMakeVisible(recButton);

    // Gate + Speed
    setupSlider(gateSlider, gateLabel, "Gate");
    setupCombo(speedBox, speedLabel, "Speed");
    speedBox.addItemList({ "1/2", "1x", "2x", "4x" }, 1);

    setResizable(true, true);
    setResizeLimits(520, 560, 1200, 900);
    setSize(600, 620);
    startTimerHz(30);
}

DiodeEditor::~DiodeEditor() { stopTimer(); }

void DiodeEditor::setupSlider(juce::Slider& s, juce::Label& l, const juce::String& name)
{
    s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 72, 18);
    s.setColour(juce::Slider::rotarySliderFillColourId,    BLUE);
    s.setColour(juce::Slider::rotarySliderOutlineColourId, BLUE_RIM);
    s.setColour(juce::Slider::thumbColourId,               WHITE);
    s.setColour(juce::Slider::textBoxTextColourId,         WHITE);
    s.setColour(juce::Slider::textBoxBackgroundColourId,   PANEL);
    s.setColour(juce::Slider::textBoxOutlineColourId,      juce::Colours::transparentBlack);
    addAndMakeVisible(s);

    l.setText(name, juce::dontSendNotification);
    l.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    l.setColour(juce::Label::textColourId, DIM);
    l.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(l);
}

void DiodeEditor::setupCombo(juce::ComboBox& c, juce::Label& l, const juce::String& name)
{
    c.setColour(juce::ComboBox::backgroundColourId, PANEL);
    c.setColour(juce::ComboBox::textColourId,       WHITE);
    c.setColour(juce::ComboBox::outlineColourId,    BLUE_RIM);
    c.setColour(juce::ComboBox::arrowColourId,      BLUE);
    addAndMakeVisible(c);

    l.setText(name, juce::dontSendNotification);
    l.setFont(juce::FontOptions(11.0f).withStyle("Bold"));
    l.setColour(juce::Label::textColourId, DIM);
    l.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(l);
}

void DiodeEditor::updateRecButton()
{
    bool rec = processor.seqRecording.load();
    recButton.setToggleState(rec, juce::dontSendNotification);
    recButton.setButtonText(rec ? "REC ●" : "REC");
    recButton.setColour(juce::TextButton::buttonColourId, rec ? RED : juce::Colour(0xff333355));
}

void DiodeEditor::timerCallback()
{
    // Update step buttons
    int  curStep = processor.seqCurrentStep.load();
    for (int i = 0; i < DiodeProcessor::SEQ_STEPS; ++i)
    {
        bool enabled = processor.apvts.getRawParameterValue("s" + juce::String(i) + "en")->load() > 0.5f;
        int  note    = processor.seqSteps[i].note.load();
        float vel    = processor.seqSteps[i].velocity.load();
        stepButtons[i].update(enabled, note, vel, i == curStep);
    }
    updateRecButton();
}

void DiodeEditor::paint(juce::Graphics& g)
{
    g.fillAll(BG);

    // Header
    g.setColour(PANEL);
    g.fillRect(0, 0, getWidth(), 50);
    g.setColour(GOLD);
    g.setFont(juce::FontOptions(22.0f).withStyle("Bold"));
    g.drawFittedText("DIODE", {0, 6, getWidth(), 26}, juce::Justification::centred, 1);
    g.setColour(DIM);
    g.setFont(juce::FontOptions(11.0f));
    g.drawFittedText("Dual Osc  |  Filters  |  Sequencer  |  8 voices",
                     {0, 30, getWidth(), 16}, juce::Justification::centred, 1);

    auto section = [&](const juce::String& txt, juce::Colour col, int y)
    {
        g.setColour(col.withAlpha(0.7f));
        g.setFont(juce::FontOptions(10.0f).withStyle("Bold"));
        g.drawText(txt, 16, y, 90, 14, juce::Justification::left);
        g.setColour(col.withAlpha(0.2f));
        g.drawHorizontalLine(y + 7, 90.0f, (float)getWidth() - 16.0f);
    };

    section("OSCILLATORS", GOLD,  54);
    section("FILTERS",     GREEN, 182);
    section("ENVELOPE",    BLUE,  318);
    section("SEQUENCER",   RED,   432);
}

void DiodeEditor::resized()
{
    const int W       = getWidth();
    const int pad     = 16;
    const int inner   = W - pad * 2;
    const int labelH  = 16;
    const int comboH  = 26;
    const int KNOB_MAX = 90;   // hard cap — knobs never grow beyond this

    int y = 55;  // cursor: top of current section content

    // ── OSCILLATORS ──────────────────────────────────────────────────────
    y += 18;  // section label
    {
        int comboW = juce::jmin(110, inner / 6);
        int knobW  = juce::jmin(KNOB_MAX, (inner - comboW * 2 - 12) / 3);
        int knobH  = knobW;
        int rowH   = juce::jmax(comboH + labelH + 4, knobH + labelH);

        osc1Box  .setBounds(pad,              y, comboW, comboH);
        osc1Label.setBounds(pad,              y + comboH + 2, comboW, labelH);
        osc2Box  .setBounds(pad + comboW + 4, y, comboW, comboH);
        osc2Label.setBounds(pad + comboW + 4, y + comboH + 2, comboW, labelH);

        int kx = pad + comboW * 2 + 12;
        int ky = y + (rowH - knobH - labelH) / 2;
        osc1VolSlider.setBounds(kx,           ky, knobW, knobH);
        osc1VolLabel .setBounds(kx,           ky + knobH, knobW, labelH);
        osc2VolSlider.setBounds(kx + knobW,   ky, knobW, knobH);
        osc2VolLabel .setBounds(kx + knobW,   ky + knobH, knobW, labelH);
        detuneSlider .setBounds(kx + knobW*2, ky, knobW, knobH);
        detuneLabel  .setBounds(kx + knobW*2, ky + knobH, knobW, labelH);

        y += rowH + 14;
    }

    // ── FILTERS ──────────────────────────────────────────────────────────
    y += 18;
    {
        int half    = inner / 2 - 4;
        int fComboW = juce::jmin(82, half / 4);
        int fKnobW  = juce::jmin(KNOB_MAX, (half - fComboW - 8) / 2);
        int fKnobH  = fKnobW;
        int rowH    = juce::jmax(comboH + labelH + 4, fKnobH + labelH);
        int ky      = y + (rowH - fKnobH - labelH) / 2;

        f1TypeBox  .setBounds(pad,                         y,  fComboW, comboH);
        f1TypeLabel.setBounds(pad,                         y + comboH + 2, fComboW, labelH);
        f1CutSlider.setBounds(pad + fComboW + 4,           ky, fKnobW, fKnobH);
        f1CutLabel .setBounds(pad + fComboW + 4,           ky + fKnobH, fKnobW, labelH);
        f1ResSlider.setBounds(pad + fComboW + fKnobW + 8,  ky, fKnobW, fKnobH);
        f1ResLabel .setBounds(pad + fComboW + fKnobW + 8,  ky + fKnobH, fKnobW, labelH);

        int fx2 = pad + half + 8;
        f2TypeBox  .setBounds(fx2,                         y,  fComboW, comboH);
        f2TypeLabel.setBounds(fx2,                         y + comboH + 2, fComboW, labelH);
        f2CutSlider.setBounds(fx2 + fComboW + 4,           ky, fKnobW, fKnobH);
        f2CutLabel .setBounds(fx2 + fComboW + 4,           ky + fKnobH, fKnobW, labelH);
        f2ResSlider.setBounds(fx2 + fComboW + fKnobW + 8,  ky, fKnobW, fKnobH);
        f2ResLabel .setBounds(fx2 + fComboW + fKnobW + 8,  ky + fKnobH, fKnobW, labelH);

        y += rowH + 14;
    }

    // ── ENVELOPE ─────────────────────────────────────────────────────────
    y += 18;
    {
        int kW = juce::jmin(KNOB_MAX, inner / 3);
        int kH = kW;
        int ex = pad + (inner - kW * 3) / 2;

        attackSlider .setBounds(ex + kW * 0, y, kW, kH);
        decaySlider  .setBounds(ex + kW * 1, y, kW, kH);
        sustainSlider.setBounds(ex + kW * 2, y, kW, kH);
        attackLabel .setBounds(ex + kW * 0, y + kH, kW, labelH);
        decayLabel  .setBounds(ex + kW * 1, y + kH, kW, labelH);
        sustainLabel.setBounds(ex + kW * 2, y + kH, kW, labelH);

        y += kH + labelH + 14;
    }

    // ── SEQUENCER ────────────────────────────────────────────────────────
    y += 18;
    {
        const int btnH  = 72;
        const int recW  = 52;
        const int recH  = 26;
        const int gateW = 68;
        const int spdW  = 62;
        const int spdH  = 26;
        const int ctrl  = recW + gateW + spdW + 18;  // total control area
        int btnTotal    = inner - ctrl;
        int btnW        = btnTotal / DiodeProcessor::SEQ_STEPS;

        recButton .setBounds(pad,                   y + (btnH - recH) / 2, recW, recH);
        gateSlider.setBounds(pad + recW + 6,        y,            gateW, btnH - labelH);
        gateLabel .setBounds(pad + recW + 6,        y + btnH - labelH, gateW, labelH);
        speedBox  .setBounds(pad + recW + gateW + 12, y + (btnH - spdH) / 2 - 10, spdW, spdH);
        speedLabel.setBounds(pad + recW + gateW + 12, y + (btnH - spdH) / 2 + spdH - 8, spdW, labelH);

        int bx = pad + ctrl + 4;
        for (int i = 0; i < DiodeProcessor::SEQ_STEPS; ++i)
            stepButtons[i].setBounds(bx + btnW * i, y, btnW, btnH);
    }
}
