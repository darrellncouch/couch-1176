#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// ── Custom LookAndFeel ─────────────────────────────────────────────────────────
class Couch1176LookAndFeel : public juce::LookAndFeel_V4
{
public:
    Couch1176LookAndFeel();

    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h,
                           float sliderPos, float startAngle, float endAngle,
                           juce::Slider&) override;

    juce::Label* createSliderTextBox (juce::Slider&) override;
};

// ── Main editor ────────────────────────────────────────────────────────────────
class Couch1176Editor final : public juce::AudioProcessorEditor,
                              private juce::Timer
{
public:
    explicit Couch1176Editor (Couch1176Processor&);
    ~Couch1176Editor() override;

    void paint   (juce::Graphics&) override;
    void resized () override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    // ── Timer (meter refresh at 30 Hz) ────────────────────────────────────
    void timerCallback() override;

    // ── Draw helpers ──────────────────────────────────────────────────────
    void drawBackground      (juce::Graphics&) const;
    void drawVUMeterFace     (juce::Graphics&) const;
    void drawVUNeedle        (juce::Graphics&) const;
    void drawKnobDialScale   (juce::Graphics&, float cx, float cy,
                              float innerR, float outerR, float labelR,
                              int   nTicks, bool large) const;
    void drawKnobLabels      (juce::Graphics&) const;
    void drawRatioButtons    (juce::Graphics&) const;
    void drawMeterModeButtons(juce::Graphics&) const;
    void drawBranding        (juce::Graphics&) const;
    void drawBottomBar       (juce::Graphics&) const;

    // ── Ratio helpers ─────────────────────────────────────────────────────
    // btnIndex: 0=20:1(top), 1=12:1, 2=8:1, 3=4:1(bot), 4=ALL
    int  getCurrentRatioParam () const noexcept;
    void setRatioFromBtn      (int btnIndex);
    juce::Rectangle<float> getRatioBtnBounds  (int btnIndex) const noexcept;
    juce::Rectangle<float> getMeterBtnBounds  (int btnIndex) const noexcept;

    // ── Needle angle (clock-face convention, radians from 12 o'clock CW) ──
    // Returns angle such that tipX = pivotX + sin(a)*R, tipY = pivotY - cos(a)*R
    float needleAngleForGR  (float grDb)    const noexcept;
    float needleAngleForOut (float outDbFS) const noexcept;

    // ── Members ───────────────────────────────────────────────────────────
    Couch1176Processor& processor;
    Couch1176LookAndFeel lnf;

    juce::Slider inputSlider, outputSlider, attackSlider, releaseSlider;

    using SliderAttach = juce::AudioProcessorValueTreeState::SliderAttachment;
    std::unique_ptr<SliderAttach> inputAttach, outputAttach, attackAttach, releaseAttach;

    // Needle physics (angle in clock-face radians)
    float needleAngle    = 0.f;
    float needleVelocity = 0.f;

    // Meter mode: 0=GR, 1=+8 VU output, 2=+4 VU output, 3=OFF
    int meterMode = 0;

    // ── Layout constants ──────────────────────────────────────────────────
    static constexpr int PLUGIN_W = 700;
    static constexpr int PLUGIN_H = 195;

    // Large knobs
    static constexpr int INP_X = 10,  INP_Y = 24, INP_W = 88, INP_H = 88;
    static constexpr int OUT_X = 106, OUT_Y = 24, OUT_W = 88, OUT_H = 88;

    // Small knobs
    static constexpr int ATK_X = 206, ATK_Y = 30, ATK_W = 72, ATK_H = 72;
    static constexpr int REL_X = 284, REL_Y = 30, REL_W = 72, REL_H = 72;

    // Ratio button column (btnIndex 0-3 = 20/12/8/4, 4 = ALL)
    static constexpr int RB_X = 364, RB_W = 54, RB_H = 27;
    static constexpr int RB_Y0 = 23,  RB_Y1 = 54,  RB_Y2 = 85, RB_Y3 = 116;
    static constexpr int RB_Y_ALL = 148, RB_ALL_H = 18;

    // VU meter face
    static constexpr int   VM_X = 426, VM_Y = 5,  VM_W = 184, VM_H = 158;
    static constexpr float VM_PX = VM_X + VM_W * 0.5f;   // pivot x = 518
    static constexpr float VM_PY = VM_Y + VM_H - 4.f;    // pivot y = 159
    static constexpr float NEEDLE_R    = 66.f;
    static constexpr float SCALE_R_OUT = 60.f;
    static constexpr float SCALE_R_IN  = 51.f;
    static constexpr float LABEL_R     = 42.f;

    // Meter mode button column (GR / +8 / +4 / OFF)
    static constexpr int MB_X = 618, MB_W = 54, MB_H = 27;
    static constexpr int MB_Y0 = 23, MB_Y1 = 54, MB_Y2 = 85, MB_Y3 = 116;

    // Bottom bar
    static constexpr int BOT_Y = 168;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Couch1176Editor)
};
