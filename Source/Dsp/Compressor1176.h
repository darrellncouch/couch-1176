#pragma once
#include <cmath>
#include <algorithm>
#include <atomic>

/**
 *  Couch 1176 – FET Compressor DSP Core
 *
 *  Topology: feedback peak compressor (detector reads the post-GR signal,
 *  exactly as the original UREI 1176 hardware does).
 *
 *  Key authenticity points:
 *    • Feedback detection – softer, more musical compression curve
 *    • Per-ratio thresholds (higher ratios = higher threshold, per UA documentation)
 *    • Per-ratio soft-knee widths  (narrower at higher ratios)
 *    • Program-dependent release via dual-integrator blend
 *    • "All-buttons" mode: ~13:1, tighter knee, slower initial attack
 *      (lets transients through first), plus FET soft saturation
 *    • Input gain range:  0 → +44 dB  (drives signal into compression)
 *    • Output gain range: –20 → +32 dB (makeup gain)
 *    • Attack:  800 µs (slowest) → 20 µs (fastest)   [CW = faster, per hardware]
 *    • Release: 1100 ms (slowest) → 50 ms (fastest)  [CW = faster, per hardware]
 */
class Compressor1176
{
public:
    // ── Ratio modes ───────────────────────────────────────────────────────────
    enum class Ratio : int
    {
        R4   = 0,   // 4:1
        R8   = 1,   // 8:1
        R12  = 2,   // 12:1
        R20  = 3,   // 20:1
        ALL  = 4    // All-buttons (British) mode
    };

    // ── Thread-safe parameters (written by audio thread every block) ──────────
    std::atomic<float> inputGainDb  { 22.0f };   // 0 → +44 dB
    std::atomic<float> outputGainDb {  6.0f };   // –20 → +32 dB
    std::atomic<float> attackTime   { 0.0002f };  // seconds
    std::atomic<float> releaseTime  { 0.5f };     // seconds
    std::atomic<int>   ratioMode    { 0 };        // see Ratio enum

    // ── API ───────────────────────────────────────────────────────────────────
    void  prepare (double sampleRate);
    void  reset   ();

    /** Process one sample for the given channel (0=L, 1=R). */
    float process (float inputSample, int channel);

    /** GR in dB (positive = gain reduction). Thread-safe for metering. */
    float getGainReductionDb() const noexcept { return grDisplayDb.load(); }

    /** Output level in dBFS, smoothed. Thread-safe for metering. */
    float getOutputLevelDb()   const noexcept { return outLevelDb.load(); }

private:
    // ── Per-channel state ─────────────────────────────────────────────────────
    struct Channel
    {
        float fastEnv    = 0.f;   // fast-release integrator
        float slowEnv    = 0.f;   // slow-release integrator
        float gainDb     = 0.f;   // current applied GR (negative = reduction)
        float prevOut    = 0.f;   // previous output sample (feedback source)
    };

    Channel ch[2];
    double  sampleRate    = 44100.0;
    float   meterSmoothA  = 0.f;   // meter LP coefficient

    std::atomic<float> grDisplayDb { 0.f };
    std::atomic<float> outLevelDb  { -100.f };

    // ── Helpers ───────────────────────────────────────────────────────────────
    static float dBToLin (float db) noexcept { return std::pow(10.f, db * 0.05f); }
    static float linTodB (float lin) noexcept { return 20.f * std::log10(std::max(lin, 1e-9f)); }

    /** Gain-computer (feed-forward formula applied to feedback-detected level).
     *  Returns a negative dB value (the gain reduction to apply). */
    static float gainComputer (float levelDb, float threshDb,
                               float ratio,   float kneeDb) noexcept;

    /** Ratio-specific parameters table. */
    struct RatioParams { float ratio; float threshDb; float kneeDb; };
    static RatioParams ratioParams (Ratio mode) noexcept;

    /** Soft saturation (tanh-based) used in all-buttons mode. */
    static float saturate (float x, float drive) noexcept;
};
