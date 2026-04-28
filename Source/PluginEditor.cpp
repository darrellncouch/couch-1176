#include "PluginEditor.h"

// ─────────────────────────────────────────────────────────────────────────────
// Colour palette  (1176 aesthetic: dark anodised panel, warm amber meter)
// ─────────────────────────────────────────────────────────────────────────────
namespace Col
{
    // Panel
    const juce::Colour bg          { 0xff1c1c1c };
    const juce::Colour bgDark      { 0xff101010 };
    const juce::Colour chrome      { 0xff8a8a8a };
    const juce::Colour chromeDark  { 0xff484848 };
    const juce::Colour silver      { 0xffb8b8b8 };
    const juce::Colour cream       { 0xffd0c8a0 };
    const juce::Colour dimCream    { 0xff786850 };

    // VU meter face – warm amber backlit look
    const juce::Colour meterBezel  { 0xff6a6a6a };
    const juce::Colour meterFace   { 0xfff0d850 };   // warm amber / cream
    const juce::Colour meterShadow { 0xffa88800 };   // darker amber for shadow
    const juce::Colour meterPrint  { 0xff100800 };   // very dark brown (printed scale)
    const juce::Colour meterNeedle { 0xff0a0600 };   // almost-black needle

    // Buttons
    const juce::Colour btnOff      { 0xff1e1e1e };
    const juce::Colour btnBorder   { 0xff3e3e3e };
    const juce::Colour btnOn       { 0xffcc2400 };   // red-orange lit
    const juce::Colour btnAllOn    { 0xffcc8800 };   // amber ALL mode
    const juce::Colour btnMeter    { 0xff1a1a30 };   // meter btn off (dark)
    const juce::Colour btnMeterOn  { 0xff0044cc };   // blue lit meter btn

    // Knob
    const juce::Colour knobRing    { 0xff646464 };
    const juce::Colour knobBody1   { 0xff303030 };
    const juce::Colour knobBody2   { 0xff0e0e0e };

    // Power LED
    const juce::Colour ledGreen    { 0xff00cc44 };
}

// ─────────────────────────────────────────────────────────────────────────────
// LookAndFeel  (knob renderer)
// ─────────────────────────────────────────────────────────────────────────────

Couch1176LookAndFeel::Couch1176LookAndFeel()
{
    setColour (juce::Slider::rotarySliderFillColourId,    Col::knobRing);
    setColour (juce::Slider::rotarySliderOutlineColourId, Col::chromeDark);
}

void Couch1176LookAndFeel::drawRotarySlider (juce::Graphics& g,
                                              int x, int y, int w, int h,
                                              float sliderPos,
                                              float startAngle, float endAngle,
                                              juce::Slider& slider)
{
    juce::ignoreUnused (slider);

    const float cx  = x + w * 0.5f;
    const float cy  = y + h * 0.5f;
    const float rad = juce::jmin (w, h) * 0.5f - 4.f;

    // Chrome outer ring
    g.setColour (Col::knobRing);
    g.fillEllipse (cx - rad - 3.f, cy - rad - 3.f,
                   (rad + 3.f) * 2.f, (rad + 3.f) * 2.f);

    // Knob body gradient (top-left highlight)
    juce::ColourGradient bodyGrad (Col::knobBody1, cx - rad * 0.4f, cy - rad * 0.5f,
                                   Col::knobBody2, cx + rad * 0.5f, cy + rad * 0.6f, false);
    g.setGradientFill (bodyGrad);
    g.fillEllipse (cx - rad, cy - rad, rad * 2.f, rad * 2.f);

    // Subtle top-left sheen
    g.setColour (juce::Colour (0x1cffffff));
    g.fillEllipse (cx - rad, cy - rad, rad * 2.f, rad * 2.f);

    // Pointer line (indicator mark)
    const float angle = startAngle + sliderPos * (endAngle - startAngle);
    // JUCE clock-face convention: tipX = cx + sin(angle)*len, tipY = cy - cos(angle)*len
    const float pLen  = rad * 0.70f;
    const float px    = cx + std::sin (angle) * pLen;
    const float py    = cy - std::cos (angle) * pLen;

    g.setColour (Col::cream);
    g.drawLine (cx, cy, px, py, 2.5f);

    // Center cap
    const float capR = rad * 0.12f;
    g.setColour (Col::chromeDark);
    g.fillEllipse (cx - capR, cy - capR, capR * 2.f, capR * 2.f);
}

juce::Label* Couch1176LookAndFeel::createSliderTextBox (juce::Slider& s)
{
    auto* l = LookAndFeel_V4::createSliderTextBox (s);
    l->setColour (juce::Label::textColourId,       juce::Colours::transparentBlack);
    l->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
    l->setColour (juce::Label::outlineColourId,    juce::Colours::transparentBlack);
    return l;
}

// ─────────────────────────────────────────────────────────────────────────────
// Constructor / Destructor
// ─────────────────────────────────────────────────────────────────────────────

Couch1176Editor::Couch1176Editor (Couch1176Processor& p)
    : AudioProcessorEditor (&p),
      processor (p)
{
    setLookAndFeel (&lnf);
    setSize (PLUGIN_W, PLUGIN_H);

    // Rotary params: 270° sweep, 7 o'clock → 5 o'clock (JUCE clock-face radians)
    const float startA = juce::MathConstants<float>::pi * 1.25f;
    const float endA   = juce::MathConstants<float>::pi * 2.75f;

    auto configSlider = [&] (juce::Slider& s)
    {
        s.setSliderStyle   (juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle  (juce::Slider::NoTextBox, true, 0, 0);
        s.setRotaryParameters (startA, endA, true);
        s.setPopupDisplayEnabled (true, true, this);
        addAndMakeVisible (s);
    };

    configSlider (inputSlider);
    configSlider (outputSlider);
    configSlider (attackSlider);
    configSlider (releaseSlider);

    inputAttach   = std::make_unique<SliderAttach> (p.apvts, "input",   inputSlider);
    outputAttach  = std::make_unique<SliderAttach> (p.apvts, "output",  outputSlider);
    attackAttach  = std::make_unique<SliderAttach> (p.apvts, "attack",  attackSlider);
    releaseAttach = std::make_unique<SliderAttach> (p.apvts, "release", releaseSlider);

    // Rest needle to 0 dB GR (far right = +60° from vertical)
    needleAngle = needleAngleForGR (0.f);

    startTimerHz (30);
}

Couch1176Editor::~Couch1176Editor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
// resized
// ─────────────────────────────────────────────────────────────────────────────

void Couch1176Editor::resized()
{
    inputSlider  .setBounds (INP_X, INP_Y, INP_W, INP_H);
    outputSlider .setBounds (OUT_X, OUT_Y, OUT_W, OUT_H);
    attackSlider .setBounds (ATK_X, ATK_Y, ATK_W, ATK_H);
    releaseSlider.setBounds (REL_X, REL_Y, REL_W, REL_H);
}

// ─────────────────────────────────────────────────────────────────────────────
// Timer – meter needle physics
// ─────────────────────────────────────────────────────────────────────────────

void Couch1176Editor::timerCallback()
{
    float target;

    switch (meterMode)
    {
        case 0:  // GR
            target = needleAngleForGR (processor.getGainReductionDb());
            break;
        case 1:  // +8 VU output
            target = needleAngleForOut (processor.getOutputLevelDb() + 8.f);
            break;
        case 2:  // +4 VU output
            target = needleAngleForOut (processor.getOutputLevelDb() + 4.f);
            break;
        default: // OFF
            target = needleAngleForGR (0.f);   // rest position
            break;
    }

    // Spring-damper (feels like a real VU ballistic)
    needleVelocity = needleVelocity * 0.78f + (target - needleAngle) * 0.10f;
    needleAngle   += needleVelocity;

    repaint (VM_X - 4, VM_Y - 4, VM_W + 8, VM_H + 8);
}

// ─────────────────────────────────────────────────────────────────────────────
// Needle angle helpers  (clock-face: 0 = 12 o'clock, CW positive)
// tipX = pivotX + sin(angle)*R,  tipY = pivotY - cos(angle)*R
// Sweep: ±60° (±π/3) from vertical
//   0 dB GR → full right →  +π/3
//  20 dB GR → full left  →  -π/3
// ─────────────────────────────────────────────────────────────────────────────

float Couch1176Editor::needleAngleForGR (float grDb) const noexcept
{
    const float norm  = juce::jlimit (0.f, 1.f, grDb / 20.f);  // 0=right, 1=left
    return juce::MathConstants<float>::pi / 3.f * (1.f - 2.f * norm);
}

float Couch1176Editor::needleAngleForOut (float shiftedDbFS) const noexcept
{
    // Map: 0 dBFS (shifted) → full right, −20 dBFS → full left
    const float norm = juce::jlimit (0.f, 1.f, (0.f - shiftedDbFS) / 20.f);
    return juce::MathConstants<float>::pi / 3.f * (1.f - 2.f * norm);
}

// ─────────────────────────────────────────────────────────────────────────────
// Ratio button geometry / interaction
// ─────────────────────────────────────────────────────────────────────────────

int Couch1176Editor::getCurrentRatioParam() const noexcept
{
    auto* raw = processor.apvts.getRawParameterValue ("ratio");
    return raw ? static_cast<int> (raw->load()) : 0;
}

void Couch1176Editor::setRatioFromBtn (int btnIndex)
{
    // Visual top→bottom:  0=20:1 (param 3), 1=12:1 (param 2),
    //                     2=8:1  (param 1), 3=4:1  (param 0), 4=ALL (param 4)
    const int paramMap[] = { 3, 2, 1, 0, 4 };
    const int paramIdx   = paramMap[btnIndex];

    if (auto* param = processor.apvts.getParameter ("ratio"))
        param->setValueNotifyingHost (static_cast<float> (paramIdx) / 4.f);
}

juce::Rectangle<float> Couch1176Editor::getRatioBtnBounds (int i) const noexcept
{
    const int ys[] = { RB_Y0, RB_Y1, RB_Y2, RB_Y3, RB_Y_ALL };
    const int hs[] = { RB_H,  RB_H,  RB_H,  RB_H,  RB_ALL_H };
    return { (float) RB_X, (float) ys[i], (float) RB_W, (float) hs[i] };
}

juce::Rectangle<float> Couch1176Editor::getMeterBtnBounds (int i) const noexcept
{
    const int ys[] = { MB_Y0, MB_Y1, MB_Y2, MB_Y3 };
    return { (float) MB_X, (float) ys[i], (float) MB_W, (float) MB_H };
}

// ─────────────────────────────────────────────────────────────────────────────
// Mouse – ratio buttons + meter mode buttons
// ─────────────────────────────────────────────────────────────────────────────

void Couch1176Editor::mouseDown (const juce::MouseEvent& e)
{
    const auto pt = e.position;

    // Ratio column (5 buttons incl. ALL)
    for (int i = 0; i < 5; ++i)
    {
        if (getRatioBtnBounds (i).contains (pt))
        {
            setRatioFromBtn (i);
            repaint (RB_X - 4, RB_Y0 - 4, RB_W + 8, RB_Y_ALL + RB_ALL_H - RB_Y0 + 8);
            return;
        }
    }

    // Meter mode column (4 buttons)
    for (int i = 0; i < 4; ++i)
    {
        if (getMeterBtnBounds (i).contains (pt))
        {
            meterMode = i;
            repaint (MB_X - 4, MB_Y0 - 4, MB_W + 8, MB_Y3 + MB_H - MB_Y0 + 8);
            return;
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// paint
// ─────────────────────────────────────────────────────────────────────────────

void Couch1176Editor::paint (juce::Graphics& g)
{
    drawBackground       (g);
    drawKnobLabels       (g);

    // Draw dial scales for large knobs (INPUT, OUTPUT) in editor space
    drawKnobDialScale (g,
        INP_X + INP_W * 0.5f, INP_Y + INP_H * 0.5f,
        INP_W * 0.5f - 2.f,   // inner radius (just outside knob chrome)
        INP_W * 0.5f + 9.f,   // outer radius
        INP_W * 0.5f + 16.f,  // label radius
        11, true);

    drawKnobDialScale (g,
        OUT_X + OUT_W * 0.5f, OUT_Y + OUT_H * 0.5f,
        OUT_W * 0.5f - 2.f,
        OUT_W * 0.5f + 9.f,
        OUT_W * 0.5f + 16.f,
        11, true);

    // Smaller dial scales for ATTACK, RELEASE
    drawKnobDialScale (g,
        ATK_X + ATK_W * 0.5f, ATK_Y + ATK_H * 0.5f,
        ATK_W * 0.5f - 2.f,
        ATK_W * 0.5f + 7.f,
        ATK_W * 0.5f + 14.f,
        11, false);

    drawKnobDialScale (g,
        REL_X + REL_W * 0.5f, REL_Y + REL_H * 0.5f,
        REL_W * 0.5f - 2.f,
        REL_W * 0.5f + 7.f,
        REL_W * 0.5f + 14.f,
        11, false);

    drawRatioButtons     (g);
    drawVUMeterFace      (g);
    drawVUNeedle         (g);
    drawMeterModeButtons (g);
    drawBranding         (g);
    drawBottomBar        (g);
}

// ─────────────────────────────────────────────────────────────────────────────
// Background
// ─────────────────────────────────────────────────────────────────────────────

void Couch1176Editor::drawBackground (juce::Graphics& g) const
{
    // Panel base
    juce::ColourGradient panelGrad (juce::Colour (0xff202020), 0.f, 0.f,
                                    juce::Colour (0xff141414), 0.f, (float) PLUGIN_H, false);
    g.setGradientFill (panelGrad);
    g.fillAll();

    // Subtle horizontal brushed-metal lines
    g.setColour (juce::Colour (0x08000000));
    for (int y = 0; y < PLUGIN_H; y += 2)
        g.drawHorizontalLine (y, 0.f, (float) PLUGIN_W);

    // Outer chrome frame
    g.setColour (Col::chrome.withAlpha (0.4f));
    g.drawRect (getLocalBounds(), 2);
    g.setColour (Col::chromeDark.withAlpha (0.5f));
    g.drawRect (getLocalBounds().reduced (2), 1);
}

// ─────────────────────────────────────────────────────────────────────────────
// Dial scale  (drawn in editor paint space around a knob center)
// Uses JUCE clock-face convention:  tipX = cx + sin(a)*r,  tipY = cy - cos(a)*r
// ─────────────────────────────────────────────────────────────────────────────

void Couch1176Editor::drawKnobDialScale (juce::Graphics& g,
                                          float cx, float cy,
                                          float innerR, float outerR, float labelR,
                                          int   nTicks, bool large) const
{
    // Match the slider's rotary params (set in constructor)
    const float startAngle = juce::MathConstants<float>::pi * 1.25f;
    const float endAngle   = juce::MathConstants<float>::pi * 2.75f;

    g.setFont (juce::Font ("Arial", large ? 8.f : 7.f, juce::Font::plain));

    for (int i = 0; i < nTicks; ++i)
    {
        const float t     = static_cast<float> (i) / static_cast<float> (nTicks - 1);
        const float angle = startAngle + t * (endAngle - startAngle);

        const float sa = std::sin (angle);
        const float ca = std::cos (angle);

        const bool isMajor = (i == 0 || i == (nTicks - 1) / 2 || i == nTicks - 1);
        const float r1 = isMajor ? innerR : innerR + (outerR - innerR) * 0.25f;

        // Tick
        g.setColour (isMajor ? Col::silver : Col::silver.withAlpha (0.5f));
        g.drawLine (cx + sa * r1,     cy - ca * r1,
                    cx + sa * outerR, cy - ca * outerR,
                    isMajor ? 1.5f : 0.8f);

        // Number at major ticks
        if (isMajor)
        {
            const juce::String label = (i == 0) ? "0"
                                     : (i == (nTicks - 1) / 2) ? "5"
                                     : "10";
            g.setColour (Col::dimCream);
            const float lx = cx + sa * labelR;
            const float ly = cy - ca * labelR;
            g.drawText (label,
                        juce::Rectangle<float> (lx - 8.f, ly - 6.f, 16.f, 12.f),
                        juce::Justification::centred);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Knob section labels
// ─────────────────────────────────────────────────────────────────────────────

void Couch1176Editor::drawKnobLabels (juce::Graphics& g) const
{
    struct L { int x, y, w; const char* text; bool large; };
    const L specs[] = {
        { INP_X, INP_Y + INP_H + 18, INP_W, "INPUT",   true  },
        { OUT_X, OUT_Y + OUT_H + 18, OUT_W, "OUTPUT",  true  },
        { ATK_X, ATK_Y - 14,         ATK_W, "ATTACK",  false },
        { REL_X, REL_Y - 14,         REL_W, "RELEASE", false },
    };

    for (const auto& s : specs)
    {
        g.setFont (juce::Font ("Arial", s.large ? 9.5f : 8.5f, juce::Font::bold));
        g.setColour (Col::cream);
        g.drawText (s.text, s.x, s.y, s.w, 13, juce::Justification::centred);
    }

    // Sub-labels
    g.setFont (juce::Font ("Arial", 7.f, juce::Font::plain));
    g.setColour (Col::dimCream);
    g.drawText ("CW = \xe2\x86\x91",
                ATK_X, ATK_Y + ATK_H + 2, ATK_W, 10, juce::Justification::centred);
    g.drawText ("CW = \xe2\x86\x91",
                REL_X, REL_Y + REL_H + 2, REL_W, 10, juce::Justification::centred);
}

// ─────────────────────────────────────────────────────────────────────────────
// VU Meter face  (amber backlit)
// ─────────────────────────────────────────────────────────────────────────────

void Couch1176Editor::drawVUMeterFace (juce::Graphics& g) const
{
    const juce::Rectangle<float> face ((float) VM_X, (float) VM_Y,
                                       (float) VM_W, (float) VM_H);

    // ── Outer chrome bezel ─────────────────────────────────────────────────
    g.setColour (Col::meterBezel);
    g.fillRoundedRectangle (face.expanded (5.f), 6.f);
    g.setColour (Col::chromeDark);
    g.fillRoundedRectangle (face.expanded (2.f), 4.f);

    // ── Amber meter face ───────────────────────────────────────────────────
    juce::ColourGradient faceGrad (juce::Colour (0xfff8e850), face.getCentreX(), face.getY(),
                                   juce::Colour (0xffd4a800), face.getCentreX(), face.getBottom(),
                                   false);
    g.setGradientFill (faceGrad);
    g.fillRoundedRectangle (face, 3.f);

    // Amber back-light glow overlay
    g.setColour (juce::Colour (0x28ffcc00));
    g.fillRoundedRectangle (face, 3.f);

    // ── Scale arc tick marks and labels ────────────────────────────────────
    // Pivot in component space
    const float px = VM_PX;
    const float py = VM_PY;

    // GR positions: -20 dB (left, -60°) … 0 dB (right, +60°)
    // Clock-face angle: a = π/3 * (1 - 2*norm), norm = grDb/20
    // tipX = px + sin(a)*R,  tipY = py - cos(a)*R
    struct Tick { float grDb; const char* label; bool major; };
    const Tick ticks[] = {
        { 20.f, "20", true  },
        { 14.f, "14", true  },
        { 10.f, "10", true  },
        {  7.f, "7",  false },
        {  5.f, "5",  true  },
        {  3.f, "3",  false },
        {  2.f, "2",  false },
        {  1.f, "1",  false },
        {  0.f, "0",  true  },
    };

    for (const auto& t : ticks)
    {
        const float norm  = t.grDb / 20.f;
        const float angle = juce::MathConstants<float>::pi / 3.f * (1.f - 2.f * norm);
        const float sa    = std::sin (angle);
        const float ca    = std::cos (angle);

        const float r1 = t.major ? SCALE_R_IN - 3.f : SCALE_R_IN + 2.f;

        // Tick (dark brown, printed-look)
        g.setColour (t.major ? Col::meterPrint : Col::meterPrint.withAlpha (0.65f));
        g.drawLine (px + sa * r1,          py - ca * r1,
                    px + sa * SCALE_R_OUT, py - ca * SCALE_R_OUT,
                    t.major ? 1.6f : 0.9f);

        // Label at major ticks
        if (t.major)
        {
            const float lx = px + sa * LABEL_R;
            const float ly = py - ca * LABEL_R;
            g.setFont  (juce::Font ("Arial", 8.f, juce::Font::bold));
            g.setColour (Col::meterPrint);
            g.drawText (t.label,
                        juce::Rectangle<float> (lx - 9.f, ly - 6.f, 18.f, 12.f),
                        juce::Justification::centred);
        }
    }

    // Thin arc baseline along scale
    juce::Path arc;
    for (int i = 0; i <= 60; ++i)
    {
        const float f  = i / 60.f;
        const float a  = juce::MathConstants<float>::pi / 3.f * (1.f - 2.f * f);
        const float ax = px + std::sin (a) * SCALE_R_OUT;
        const float ay = py - std::cos (a) * SCALE_R_OUT;
        if (i == 0) arc.startNewSubPath (ax, ay);
        else        arc.lineTo (ax, ay);
    }
    g.setColour (Col::meterPrint.withAlpha (0.35f));
    g.strokePath (arc, juce::PathStrokeType (0.6f));

    // Mode label (small, bottom of face)
    const char* const modeLabels[] = { "GR  dB", "+8 VU OUTPUT", "+4 VU OUTPUT", "OFF" };
    g.setFont (juce::Font ("Arial", 7.5f, juce::Font::bold));
    g.setColour (Col::meterPrint.withAlpha (0.75f));
    g.drawText (modeLabels[meterMode],
                VM_X, static_cast<int> (VM_PY) + 2, VM_W, 10,
                juce::Justification::centred);

    // Glass sheen (top half)
    juce::ColourGradient glassGrad (juce::Colour (0x14ffffff), face.getX(), face.getY(),
                                    juce::Colour (0x00000000), face.getX(), face.getCentreY(), false);
    g.setGradientFill (glassGrad);
    g.fillRoundedRectangle (face.withHeight (face.getHeight() * 0.5f), 3.f);
}

// ─────────────────────────────────────────────────────────────────────────────
// VU Needle
// ─────────────────────────────────────────────────────────────────────────────

void Couch1176Editor::drawVUNeedle (juce::Graphics& g) const
{
    const float px = VM_PX;
    const float py = VM_PY;

    // Clamp to valid arc
    const float maxAng = juce::MathConstants<float>::pi / 3.f;
    const float angle  = juce::jlimit (-maxAng, maxAng, needleAngle);

    const float sa = std::sin (angle);
    const float ca = std::cos (angle);

    const float tipX = px + sa * NEEDLE_R;
    const float tipY = py - ca * NEEDLE_R;
    const float midX = px + sa * NEEDLE_R * 0.68f;
    const float midY = py - ca * NEEDLE_R * 0.68f;

    // Shadow
    g.setColour (juce::Colours::black.withAlpha (0.3f));
    g.drawLine (px + 1.f, py + 1.f, tipX + 1.f, tipY + 1.f, 1.5f);

    // Needle body (dark, thin)
    g.setColour (Col::meterNeedle);
    g.drawLine (px, py, midX, midY, 1.8f);

    // Needle tip accent (dark red on 1176 hardware the needle is just dark)
    g.setColour (juce::Colour (0xff2a0800));
    g.drawLine (midX, midY, tipX, tipY, 1.2f);

    // Pivot jewel
    g.setColour (juce::Colour (0xff2a1800));
    g.fillEllipse (px - 4.5f, py - 4.5f, 9.f, 9.f);
    g.setColour (juce::Colour (0xff805020));
    g.fillEllipse (px - 2.5f, py - 2.5f, 5.f, 5.f);
}

// ─────────────────────────────────────────────────────────────────────────────
// Ratio buttons (vertical column)
// ─────────────────────────────────────────────────────────────────────────────

void Couch1176Editor::drawRatioButtons (juce::Graphics& g) const
{
    const int   paramIdx = getCurrentRatioParam();
    const bool  allMode  = (paramIdx == 4);
    // paramIdx 0=4:1, 1=8:1, 2=12:1, 3=20:1
    // visual btn 0(20:1)=param3, 1(12:1)=param2, 2(8:1)=param1, 3(4:1)=param0
    const int   paramForBtn[] = { 3, 2, 1, 0 };
    const char* btnLabels[]   = { "20", "12", "8", "4" };

    // Section heading
    g.setFont (juce::Font ("Arial", 7.5f, juce::Font::bold));
    g.setColour (Col::dimCream);
    g.drawText ("RATIO", RB_X, RB_Y0 - 14, RB_W, 12, juce::Justification::centred);

    for (int i = 0; i < 4; ++i)
    {
        const auto b   = getRatioBtnBounds (i);
        const bool lit = allMode || (paramIdx == paramForBtn[i]);

        // Button body
        juce::ColourGradient grad (
            lit ? Col::btnOn.brighter (0.3f) : juce::Colour (0xff282828), b.getX(), b.getY(),
            lit ? Col::btnOn.darker   (0.4f) : juce::Colour (0xff121212), b.getX(), b.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (b, 3.f);

        // Border
        g.setColour (lit ? Col::btnOn.brighter (0.1f) : Col::btnBorder);
        g.drawRoundedRectangle (b, 3.f, 1.f);

        // Glow halo when lit
        if (lit)
        {
            g.setColour (Col::btnOn.withAlpha (0.18f));
            g.fillRoundedRectangle (b.expanded (4.f), 5.f);
        }

        // Label
        g.setFont (juce::Font ("Arial", 10.5f, juce::Font::bold));
        g.setColour (lit ? juce::Colours::white : Col::chrome);
        g.drawText (btnLabels[i], b.toNearestInt(), juce::Justification::centred);
    }

    // ALL button
    {
        const auto b = getRatioBtnBounds (4);
        juce::ColourGradient aGrad (
            allMode ? Col::btnAllOn.brighter (0.3f) : juce::Colour (0xff222210), b.getX(), b.getY(),
            allMode ? Col::btnAllOn.darker   (0.4f) : juce::Colour (0xff0c0c08), b.getX(), b.getBottom(), false);
        g.setGradientFill (aGrad);
        g.fillRoundedRectangle (b, 3.f);

        g.setColour (allMode ? Col::btnAllOn : Col::btnBorder);
        g.drawRoundedRectangle (b, 3.f, 1.f);

        if (allMode)
        {
            g.setColour (Col::btnAllOn.withAlpha (0.2f));
            g.fillRoundedRectangle (b.expanded (4.f), 5.f);
        }

        g.setFont (juce::Font ("Arial", 7.f, juce::Font::bold));
        g.setColour (allMode ? juce::Colours::white : Col::dimCream);
        g.drawText ("ALL", b.toNearestInt(), juce::Justification::centred);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Meter mode buttons (vertical column, right of VU)
// ─────────────────────────────────────────────────────────────────────────────

void Couch1176Editor::drawMeterModeButtons (juce::Graphics& g) const
{
    const char* labels[] = { "GR", "+8", "+4", "OFF" };

    // Section heading
    g.setFont (juce::Font ("Arial", 7.5f, juce::Font::bold));
    g.setColour (Col::dimCream);
    g.drawText ("METER", MB_X, MB_Y0 - 14, MB_W, 12, juce::Justification::centred);

    for (int i = 0; i < 4; ++i)
    {
        const auto b   = getMeterBtnBounds (i);
        const bool lit = (meterMode == i);

        juce::ColourGradient grad (
            lit ? Col::btnMeterOn.brighter (0.3f) : juce::Colour (0xff161620), b.getX(), b.getY(),
            lit ? Col::btnMeterOn.darker   (0.4f) : juce::Colour (0xff0a0a14), b.getX(), b.getBottom(), false);
        g.setGradientFill (grad);
        g.fillRoundedRectangle (b, 3.f);

        g.setColour (lit ? Col::btnMeterOn.brighter (0.1f) : Col::btnBorder);
        g.drawRoundedRectangle (b, 3.f, 1.f);

        if (lit)
        {
            g.setColour (Col::btnMeterOn.withAlpha (0.18f));
            g.fillRoundedRectangle (b.expanded (4.f), 5.f);
        }

        g.setFont (juce::Font ("Arial", 10.f, juce::Font::bold));
        g.setColour (lit ? juce::Colours::white : Col::chrome);
        g.drawText (labels[i], b.toNearestInt(), juce::Justification::centred);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Branding text  (below VU face)
// ─────────────────────────────────────────────────────────────────────────────

void Couch1176Editor::drawBranding (juce::Graphics& g) const
{
    const int   bx = VM_X;
    const int   bw = VM_W;
    const int   by = VM_Y + VM_H + 2;

    g.setFont (juce::Font ("Arial", 7.5f, juce::Font::bold));
    g.setColour (Col::silver.withAlpha (0.7f));
    g.drawText ("1176LN  LIMITING AMPLIFIER", bx, by, bw, 10,
                juce::Justification::centred);

    g.setFont (juce::Font ("Arial", 6.5f, juce::Font::plain));
    g.setColour (Col::dimCream.withAlpha (0.7f));
    g.drawText ("COUCH  AUDIO", bx, by + 10, bw, 9,
                juce::Justification::centred);
}

// ─────────────────────────────────────────────────────────────────────────────
// Bottom bar
// ─────────────────────────────────────────────────────────────────────────────

void Couch1176Editor::drawBottomBar (juce::Graphics& g) const
{
    const juce::Rectangle<float> bar (0.f, (float) BOT_Y,
                                      (float) PLUGIN_W,
                                      (float) (PLUGIN_H - BOT_Y));

    juce::ColourGradient botGrad (juce::Colour (0xff0c0c0c), 0.f, (float) BOT_Y,
                                  juce::Colour (0xff161616), 0.f, (float) PLUGIN_H, false);
    g.setGradientFill (botGrad);
    g.fillRect (bar);

    g.setColour (Col::chromeDark.withAlpha (0.5f));
    g.drawHorizontalLine (BOT_Y, 0.f, (float) PLUGIN_W);

    // Power LED (bottom-right)
    const float lx = PLUGIN_W - 20.f;
    const float ly = BOT_Y + (PLUGIN_H - BOT_Y) * 0.5f;
    const float lr = 4.5f;

    g.setColour (Col::ledGreen.withAlpha (0.25f));
    g.fillEllipse (lx - lr * 2.f, ly - lr * 2.f, lr * 4.f, lr * 4.f);
    juce::ColourGradient ledGrad (Col::ledGreen.brighter(), lx - lr * 0.4f, ly - lr * 0.4f,
                                  Col::ledGreen.darker(), lx + lr, ly + lr, false);
    g.setGradientFill (ledGrad);
    g.fillEllipse (lx - lr, ly - lr, lr * 2.f, lr * 2.f);
    g.setColour (juce::Colours::white.withAlpha (0.4f));
    g.fillEllipse (lx - lr * 0.45f, ly - lr * 0.55f, lr * 0.55f, lr * 0.45f);

    // "SOLID STATE LIMITING AMPLIFIER" tagline
    g.setFont (juce::Font ("Arial", 7.f, juce::Font::plain));
    g.setColour (Col::dimCream.withAlpha (0.6f));
    g.drawText ("SOLID STATE  FET  LIMITING AMPLIFIER",
                8, BOT_Y + 4, PLUGIN_W - 40, 14,
                juce::Justification::centredLeft);
}
