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
    void drawLargeKnobScale  (juce::Graphics&, float cx, float cy) const;
    void drawSmallKnobScale  (juce::Graphics&, float cx, float cy) const;
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
    static constexpr int PLUGIN_W = 920;
    static constexpr int PLUGIN_H = 265;

    // Large knobs — 25% bigger (110→138), centers at PLUGIN_H/2 = 132
    static constexpr int INP_X = 48,  INP_Y = 63, INP_W = 138, INP_H = 138;
    static constexpr int OUT_X = 235, OUT_Y = 63, OUT_W = 138, OUT_H = 138;

    // Small knobs — stacked, 35px above/below center (132), 8px gap between them
    static constexpr int ATK_X = 422, ATK_Y = 66,  ATK_W = 62, ATK_H = 62;
    static constexpr int REL_X = 422, REL_Y = 136, REL_W = 62, REL_H = 62;

    // Ratio button column
    static constexpr int RB_X = 533, RB_W = 40, RB_H = 23;
    static constexpr int RB_Y0 = 69,  RB_Y1 = 96,  RB_Y2 = 123, RB_Y3 = 150;
    static constexpr int RB_Y_ALL = 181, RB_ALL_H = 15;

    // VU meter face  (sprite frames stay 184×158; display at 230×182)
    static constexpr int   VM_X = 587, VM_Y = 38, VM_W = 230, VM_H = 182;
    static constexpr float VM_PX = VM_X + VM_W * 0.5f;
    static constexpr float VM_PY = VM_Y + VM_H - 4.f;
    static constexpr float NEEDLE_R    = 66.f;
    static constexpr float SCALE_R_OUT = 60.f;
    static constexpr float SCALE_R_IN  = 51.f;
    static constexpr float LABEL_R     = 42.f;

    // Meter mode button column
    static constexpr int MB_X = 831, MB_W = 40, MB_H = 23;
    static constexpr int MB_Y0 = 69, MB_Y1 = 96, MB_Y2 = 123, MB_Y3 = 150;

    // Bottom bar
    static constexpr int BOT_Y = 225;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Couch1176Editor)
};
