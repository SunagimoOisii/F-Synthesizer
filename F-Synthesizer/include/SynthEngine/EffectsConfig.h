#pragma once

struct ReverbEffectConfig
{
    bool operator==(const ReverbEffectConfig&) const = default;
    bool enabled = false;
    double mix = 0.0;
    double roomSize = 0.45;
    double damping = 0.30;
};

struct DelayEffectConfig
{
    bool operator==(const DelayEffectConfig&) const = default;
    bool enabled = false;
    double mix = 0.0;
    double timeSec = 0.25;
    double feedback = 0.25;
    bool tempoSync = false;
    // 1.0=4分音符, 0.5=8分音符。
    double syncBeats = 1.0;
};

struct ChorusEffectConfig
{
    bool operator==(const ChorusEffectConfig&) const = default;
    bool enabled = false;
    double mix = 0.0;
    double baseDelayMs = 14.0;
    double depthMs = 6.0;
    double rateHz = 0.35;
    double feedback = 0.08;
};

struct FlangerEffectConfig
{
    bool operator==(const FlangerEffectConfig&) const = default;
    bool enabled = false;
    double mix = 0.0;
    double baseDelayMs = 1.5;
    double depthMs = 1.0;
    double rateHz = 0.25;
    double feedback = 0.15;
};

struct BitCrusherEffectConfig
{
    bool operator==(const BitCrusherEffectConfig&) const = default;
    // 1..16（16でバイパス）
    int bits = 16;
};

struct SampleRateReducerEffectConfig
{
    bool operator==(const SampleRateReducerEffectConfig&) const = default;
    // 0..1（1.0でバイパス）
    double ratio = 1.0;
};

struct MasterEffectConfig
{
    bool operator==(const MasterEffectConfig&) const = default;
    ReverbEffectConfig reverb{};
    DelayEffectConfig delay{};
    ChorusEffectConfig chorus{};
    FlangerEffectConfig flanger{};
    BitCrusherEffectConfig bitCrusher{};
    SampleRateReducerEffectConfig sampleRateReducer{};
};
