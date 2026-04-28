#include "Compressor1176.h"

// ── Static helpers ────────────────────────────────────────────────────────────

float Compressor1176::gainComputer (float levelDb, float threshDb,
                                    float ratio,   float kneeDb) noexcept
{
    const float overshoot = levelDb - threshDb;
    const float cs        = 1.f - 1.f / ratio;   // compression slope

    if (2.f * overshoot < -kneeDb)
        return 0.f;   // below knee – no reduction

    if (2.f * std::abs(overshoot) <= kneeDb)
    {
        // Inside the soft knee
        const float t = (overshoot + kneeDb * 0.5f) / kneeDb;
        return -cs * (overshoot + kneeDb * 0.5f) * t;
    }

    // Above threshold – full compression
    return -cs * overshoot;
}

Compressor1176::RatioParams Compressor1176::ratioParams (Ratio mode) noexcept
{
    //  Per UA documentation: higher ratios have a higher threshold.
    //  Knee width narrows at higher ratios (harder knee).
    switch (mode)
    {
        case Ratio::R4:  return { 4.f,  -20.f, 8.f  };
        case Ratio::R8:  return { 8.f,  -18.f, 5.f  };
        case Ratio::R12: return { 12.f, -16.f, 3.f  };
        case Ratio::R20: return { 20.f, -12.f, 1.f  };
        case Ratio::ALL: return { 13.f, -14.f, 2.f  };   // ~12-14:1 per UA docs
        default:         return { 4.f,  -20.f, 8.f  };
    }
}

float Compressor1176::saturate (float x, float drive) noexcept
{
    // Soft tanh clipper – normalised so low-level signals pass cleanly.
    // Models the FET bias-point instability in all-buttons mode.
    const float driven = x * (1.f + drive * 3.f);
    return std::tanh(driven) / (1.f + drive * 3.f);
}

// ── Lifecycle ─────────────────────────────────────────────────────────────────

void Compressor1176::prepare (double sr)
{
    sampleRate   = sr;
    // ~50 ms LP for meter display
    meterSmoothA = std::exp(-1.f / static_cast<float>(sr * 0.05));
    reset();
}

void Compressor1176::reset()
{
    for (auto& c : ch)
        c = {};
    grDisplayDb.store(0.f);
    outLevelDb.store(-100.f);
}

// ── Per-sample processing ─────────────────────────────────────────────────────

float Compressor1176::process (float inputSample, int channel)
{
    const int ci = channel & 1;
    Channel&  c  = ch[ci];

    // ── Load parameters ────────────────────────────────────────────────────
    const float inGainDb  = inputGainDb.load();
    const float outGainDb = outputGainDb.load();
    const float atkSec    = attackTime.load();
    const float relSec    = releaseTime.load();
    const Ratio mode      = static_cast<Ratio>(ratioMode.load());
    const bool  allBtn    = (mode == Ratio::ALL);

    const auto [ratio, threshDb, kneeDb] = ratioParams(mode);

    // ── Time constants ─────────────────────────────────────────────────────
    //  All-buttons: attack is ~2× slower on initial transients (lets punch through)
    const float atkMod = allBtn ? 2.0f : 1.0f;
    const float relMod = allBtn ? 0.85f : 1.0f;

    const float atkCoeff      = std::exp(-1.f / (static_cast<float>(sampleRate) * atkSec * atkMod));
    const float fastRelCoeff  = std::exp(-1.f / (static_cast<float>(sampleRate) * relSec * relMod * 0.2f));
    const float slowRelCoeff  = std::exp(-1.f / (static_cast<float>(sampleRate) * relSec * relMod));

    // ── Input stage ────────────────────────────────────────────────────────
    const float x = inputSample * dBToLin(inGainDb);

    // ── Feedback detection (from previous gain-reduced output, pre-makeup) ──
    const float detLevel = std::abs(c.prevOut);

    // ── Dual-integrator envelope follower ──────────────────────────────────
    //  Fast integrator: recovers quickly (good for transient clarity)
    if (detLevel > c.fastEnv)
        c.fastEnv += (detLevel - c.fastEnv) * (1.f - atkCoeff);
    else
        c.fastEnv *= fastRelCoeff;

    //  Slow integrator: holds longer on sustained compression (prevents pumping)
    if (detLevel > c.slowEnv)
        c.slowEnv += (detLevel - c.slowEnv) * (1.f - atkCoeff);
    else
        c.slowEnv *= slowRelCoeff;

    //  Program-dependent blend:
    //  • transients → fast env dominates  (fast release after peaks)
    //  • sustained  → slow env dominates  (holds compression, less pumping)
    const float envelope = std::max(c.fastEnv, c.slowEnv);

    // ── Gain computer ──────────────────────────────────────────────────────
    const float levelDb     = linTodB(envelope);
    const float targetGrDb  = gainComputer(levelDb, threshDb, ratio, kneeDb);

    // Smooth gain changes through attack/release (the gain cell itself has inertia)
    if (targetGrDb < c.gainDb)
        c.gainDb = atkCoeff * c.gainDb + (1.f - atkCoeff) * targetGrDb;
    else
        c.gainDb = slowRelCoeff * c.gainDb + (1.f - slowRelCoeff) * targetGrDb;

    // ── Apply gain reduction ───────────────────────────────────────────────
    float gainReduced = x * dBToLin(c.gainDb);

    // ── All-buttons FET saturation ─────────────────────────────────────────
    if (allBtn)
        gainReduced = saturate(gainReduced, 0.35f);

    // ── Store for feedback (before makeup gain) ───────────────────────────
    c.prevOut = gainReduced;

    // ── Output makeup gain ─────────────────────────────────────────────────
    const float output = gainReduced * dBToLin(outGainDb);

    // ── Metering (channel 0 drives the display) ────────────────────────────
    if (ci == 0)
    {
        const float grNow  = -c.gainDb;   // positive = gain reduction
        const float lvNow  = linTodB(std::abs(output));

        grDisplayDb.store(meterSmoothA * grDisplayDb.load() + (1.f - meterSmoothA) * grNow);
        outLevelDb.store (meterSmoothA * outLevelDb.load()  + (1.f - meterSmoothA) * lvNow);
    }

    return output;
}
