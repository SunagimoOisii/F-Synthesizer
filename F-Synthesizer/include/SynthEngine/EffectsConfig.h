#pragma once

struct ReverbEffectConfig
{
    bool enabled = false;
    double mix = 0.0;
    double roomSize = 0.45;
    double damping = 0.30;
};

struct DelayEffectConfig
{
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
    bool enabled = false;
    double mix = 0.0;
    double baseDelayMs = 14.0;
    double depthMs = 6.0;
    double rateHz = 0.35;
    double feedback = 0.08;
};

struct FlangerEffectConfig
{
    bool enabled = false;
    double mix = 0.0;
    double baseDelayMs = 1.5;
    double depthMs = 1.0;
    double rateHz = 0.25;
    double feedback = 0.15;
};

struct BitCrusherEffectConfig
{
    // 1..16（16でバイパス）
    int bits = 16;
};

struct SampleRateReducerEffectConfig
{
    // 0..1（1.0でバイパス）
    double ratio = 1.0;
};

struct MasterEffectConfig
{
    ReverbEffectConfig reverb{};
    DelayEffectConfig delay{};
    ChorusEffectConfig chorus{};
    FlangerEffectConfig flanger{};
    BitCrusherEffectConfig bitCrusher{};
    SampleRateReducerEffectConfig sampleRateReducer{};
};
