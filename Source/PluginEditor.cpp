#include "PluginEditor.h"
#include <BinaryData.h>

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

    // Select image based on knob size (large knobs: INPUT/OUTPUT at 88px, small: ATK/REL at 72px)
    const bool isLarge = (w >= 80);

    static const juce::Image largeImg = juce::ImageCache::getFromMemory (
        BinaryData::_76large_png, BinaryData::_76large_pngSize);
    static const juce::Image smallImg = juce::ImageCache::getFromMemory (
        BinaryData::_76small_png, BinaryData::_76small_pngSize);

    const juce::Image& img = isLarge ? largeImg : smallImg;

    if (! img.isValid())
        return;

    // Angle of the indicator mark in the source image (measured from 12-o'clock, CW positive)
    // large: red indicator at -134.6° = -2.349 rad
    // small: dark dot at    -133.0° = -2.321 rad
    const float srcIndicatorAngle = isLarge ? -2.349f : -2.321f;

    // Current knob angle in JUCE clock-face space
    const float currentAngle = startAngle + sliderPos * (endAngle - startAngle);

    // Rotation needed to move the indicator from its source position to currentAngle
    const float rotation = currentAngle - srcIndicatorAngle;

    const float imgW = (float) img.getWidth();
    const float imgH = (float) img.getHeight();
    const float cx   = x + w * 0.5f;
    const float cy   = y + h * 0.5f;
    const float scale = (float) juce::jmin (w, h) / imgW;

    g.drawImageTransformed (img,
        juce::AffineTransform::translation (-imgW * 0.5f, -imgH * 0.5f)
            .scaled (scale)
            .rotated (rotation)
            .translated (cx, cy));
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

    // Custom tooltip text for attack/release — must be set AFTER SliderAttachment
    attackSlider.textFromValueFunction = [] (double v)
    {
        const float norm   = (float) v * 0.01f;
        const float timeUs = 0.0008f * std::pow (0.025f, norm) * 1e6f;
        return juce::String (juce::roundToInt (timeUs)) + " \xc2\xb5s";
    };

    releaseSlider.textFromValueFunction = [] (double v)
    {
        const float norm  = (float) v * 0.01f;
        const float timeS = 1.1f * std::pow (0.04545f, norm);
        if (timeS >= 1.0f)
            return juce::String (timeS, 2) + " s";
        return juce::String (timeS * 1000.f, 0) + " ms";
    };

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
//
// Scale landmarks measured from the needleless-vu.png image:
//   -20 dB  →  -53°  =  -0.9250 rad  (far left)
//     0 VU  →   +3°  =  +0.0524 rad  (black/red boundary)
//    +3 VU  →  +25°  =  +0.4363 rad  (far right, red zone)
//
// GR mode:   0 dB GR (no compression) → 0 VU mark (+3°)
//           20 dB GR (heavy)          → -20 mark  (-53°)
// Output:  -20 dBFS shifted → -20 mark (-53°)
//            0 dBFS shifted →  0 VU   (+3°)
//           +3 dBFS shifted → +3 VU   (+25°)
// ─────────────────────────────────────────────────────────────────────────────

static constexpr float kAngle_minus20 = -1.0036f;   // -57.5° — far left (-20 dB)
static constexpr float kAngle_zero_VU =  0.3431f;   // +19.7° — 0 VU mark (black/red boundary)
static constexpr float kAngle_plus3VU =  0.6656f;   // +38.1° — +3 VU mark (far right red)

float Couch1176Editor::needleAngleForGR (float grDb) const noexcept
{
    // 0 dB GR → 0 VU mark,  20 dB GR → -20 mark
    const float norm = juce::jlimit (0.f, 1.f, grDb / 20.f);
    return kAngle_zero_VU + norm * (kAngle_minus20 - kAngle_zero_VU);
}

float Couch1176Editor::needleAngleForOut (float shiftedDbFS) const noexcept
{
    // -20 dBFS → -20 mark,  0 dBFS → 0 VU,  +3 dBFS → +3 VU mark
    const float clamped = juce::jlimit (-20.f, 3.f, shiftedDbFS);
    if (clamped >= 0.f)
    {
        // 0 → +3 dBFS maps to 0 VU → +3 VU
        const float t = clamped / 3.f;
        return kAngle_zero_VU + t * (kAngle_plus3VU - kAngle_zero_VU);
    }
    // -20 → 0 dBFS maps to -20 mark → 0 VU
    const float t = (clamped + 20.f) / 20.f;
    return kAngle_minus20 + t * (kAngle_zero_VU - kAngle_minus20);
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

    // Draw dial scales for large knobs (INPUT, OUTPUT)
    // Radii are fixed (not knob-relative) so the marks don't grow with the larger knob
    const float largeInnerR = 42.f, largeOuterR = 53.f, largeLabelR = 60.f;
    juce::ignoreUnused (largeInnerR, largeOuterR, largeLabelR);

    drawLargeKnobScale (g, INP_X + INP_W * 0.5f, INP_Y + INP_H * 0.5f);
    drawLargeKnobScale (g, OUT_X + OUT_W * 0.5f, OUT_Y + OUT_H * 0.5f);

    // Smaller dial scales for ATTACK, RELEASE
    drawSmallKnobScale (g, ATK_X + ATK_W * 0.5f, ATK_Y + ATK_H * 0.5f);
    drawSmallKnobScale (g, REL_X + REL_W * 0.5f, REL_Y + REL_H * 0.5f);

    drawRatioButtons     (g);
    drawVUMeterFace      (g);
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
// Large knob scale  (INPUT / OUTPUT)
// Labels: ∞  48  36  30  24  18  12  6  0  (9 positions, CW from min)
// One small inter-dot between each adjacent pair of labeled positions.
// ─────────────────────────────────────────────────────────────────────────────

void Couch1176Editor::drawLargeKnobScale (juce::Graphics& g, float cx, float cy) const
{
    const float startAngle = juce::MathConstants<float>::pi * 1.25f;
    const float sweep      = juce::MathConstants<float>::pi * 1.5f;

    // Radii fixed regardless of knob image size
    // Knob body edge measured at 44px for 138px display size
    const float dotR   = 47.f;
    const float labelR = 59.f;

    struct Mark { float t; const char* label; };
    const Mark marks[] = {
        { 0.f/8.f, "\xe2\x88\x9e" },   // ∞
        { 1.f/8.f, "48" },
        { 2.f/8.f, "36" },
        { 3.f/8.f, "30" },
        { 4.f/8.f, "24" },
        { 5.f/8.f, "18" },
        { 6.f/8.f, "12" },
        { 7.f/8.f, "6"  },
        { 8.f/8.f, "0"  },
    };

    g.setFont (juce::Font ("Arial", 8.5f, juce::Font::plain));

    // One inter-dot between each labeled pair
    for (int i = 0; i < 8; ++i)
    {
        const float t  = (marks[i].t + marks[i + 1].t) * 0.5f;
        const float a  = startAngle + t * sweep;
        const float dx = cx + std::sin (a) * dotR;
        const float dy = cy - std::cos (a) * dotR;
        g.setColour (Col::silver.withAlpha (0.35f));
        g.fillEllipse (dx - 1.2f, dy - 1.2f, 2.4f, 2.4f);
    }

    // Main labeled dots + numbers
    for (const auto& m : marks)
    {
        const float a  = startAngle + m.t * sweep;
        const float sa = std::sin (a);
        const float ca = std::cos (a);

        const float dx = cx + sa * dotR;
        const float dy = cy - ca * dotR;
        g.setColour (Col::silver);
        g.fillEllipse (dx - 1.8f, dy - 1.8f, 3.6f, 3.6f);

        const float lx = cx + sa * labelR;
        const float ly = cy - ca * labelR;
        g.setColour (Col::silver.withAlpha (0.85f));
        g.drawText (m.label,
                    juce::Rectangle<float> (lx - 12.f, ly - 7.f, 24.f, 14.f),
                    juce::Justification::centred);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Small knob scale  (ATTACK / RELEASE)
// Positions: OFF  1  2  3  4  5  6  7  (8 positions, CW from min)
// Labels shown for: OFF, 1, 3, 5, 7  —  2, 4, 6 are dots only.
// ─────────────────────────────────────────────────────────────────────────────

void Couch1176Editor::drawSmallKnobScale (juce::Graphics& g, float cx, float cy) const
{
    const float startAngle = juce::MathConstants<float>::pi * 1.25f;
    const float sweep      = juce::MathConstants<float>::pi * 1.5f;

    const float dotR   = 22.f;
    const float labelR = 33.f;

    struct Mark { float t; const char* label; };   // label=nullptr → dot only
    const Mark marks[] = {
        { 0.f/6.f, "1"   },
        { 1.f/6.f, nullptr },   // 2
        { 2.f/6.f, "3"   },
        { 3.f/6.f, nullptr },   // 4
        { 4.f/6.f, "5"   },
        { 5.f/6.f, nullptr },   // 6
        { 6.f/6.f, "7"   },
    };

    g.setFont (juce::Font ("Arial", 7.5f, juce::Font::plain));

    for (const auto& m : marks)
    {
        const float a  = startAngle + m.t * sweep;
        const float sa = std::sin (a);
        const float ca = std::cos (a);

        const bool hasLabel = (m.label != nullptr);
        const float r = hasLabel ? 1.8f : 1.3f;

        const float dx = cx + sa * dotR;
        const float dy = cy - ca * dotR;
        g.setColour (hasLabel ? Col::silver : Col::silver.withAlpha (0.45f));
        g.fillEllipse (dx - r, dy - r, r * 2.f, r * 2.f);

        if (hasLabel)
        {
            const float lx = cx + sa * labelR;
            const float ly = cy - ca * labelR;
            g.setColour (Col::silver.withAlpha (0.85f));
            g.drawText (m.label,
                        juce::Rectangle<float> (lx - 12.f, ly - 6.f, 24.f, 12.f),
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
        { INP_X, INP_Y + INP_H + 14, INP_W, "INPUT",   true  },
        { OUT_X, OUT_Y + OUT_H + 14, OUT_W, "OUTPUT",  true  },
        { ATK_X, ATK_Y - 13,         ATK_W, "ATTACK",  false },
        { REL_X, REL_Y + REL_H + 4,  REL_W, "RELEASE", false },  // below knob
    };

    for (const auto& s : specs)
    {
        g.setFont (juce::Font ("Arial", s.large ? 9.5f : 8.5f, juce::Font::bold));
        g.setColour (Col::cream);
        g.drawText (s.text, s.x, s.y, s.w, 13, juce::Justification::centred);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// VU Meter face  (amber backlit)
// ─────────────────────────────────────────────────────────────────────────────

void Couch1176Editor::drawVUMeterFace (juce::Graphics& g) const
{
    const juce::Rectangle<float> face ((float) VM_X, (float) VM_Y,
                                       (float) VM_W, (float) VM_H);

    // ── Black background behind sprite (no bezel) ──────────────────────────
    g.setColour (juce::Colours::black);
    g.fillRect (face);

    // ── Sprite sheet frame selection ───────────────────────────────────────
    // 121 frames: frame 0 = −60° (20 dB GR, left), frame 120 = +60° (0 dB GR, right)
    static const juce::Image sprite = juce::ImageCache::getFromMemory (
                                          BinaryData::vu_sprite_png,
                                          BinaryData::vu_sprite_pngSize);

    if (sprite.isValid())
    {
        // Sprite frame dimensions are fixed at generation time (184×158)
        constexpr int   SPRITE_FRAME_W = 184;
        constexpr int   SPRITE_FRAME_H = 158;
        constexpr int   TOTAL_FRAMES   = 121;
        constexpr float SWEEP          = juce::MathConstants<float>::pi / 3.f;

        const float clampedAngle = juce::jlimit (-SWEEP, SWEEP, needleAngle);
        const float t            = (clampedAngle + SWEEP) / (2.f * SWEEP);
        const int   frameIdx     = juce::roundToInt (t * (TOTAL_FRAMES - 1));

        const int srcX = frameIdx * SPRITE_FRAME_W;
        g.drawImage (sprite,
                     (int) face.getX(), (int) face.getY(), VM_W, VM_H,
                     srcX, 0, SPRITE_FRAME_W, SPRITE_FRAME_H);
    }

    // ── Mode label (small, bottom of face) ────────────────────────────────
    const char* const modeLabels[] = { "GR  dB", "+8 VU OUTPUT", "+4 VU OUTPUT", "OFF" };
    g.setFont (juce::Font ("Arial", 7.5f, juce::Font::bold));
    g.setColour (Col::meterPrint.withAlpha (0.75f));
    g.drawText (modeLabels[meterMode],
                VM_X, static_cast<int> (VM_PY) + 2, VM_W, 10,
                juce::Justification::centred);
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
