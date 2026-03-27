#pragma once

#include <array>

#include "synth/Envelope.h"

enum class LfoWave
{
    Sine,
    Triangle,
    Square,
    Saw,
    SampleAndHold
};

struct LfoConfig
{
    LfoWave wave = LfoWave::Sine;
    double rateHz = 5.0;
    double depth = 0.0;
    bool bipolar = true;
    bool keySync = false;
    double delayMs = 0.0;
    double fadeMs = 0.0;
};

enum class ModSource
{
    None,
    Lfo1,
    Env2,
    Velocity,
    ChannelPressure,
    PolyPressure,
    ModWheel
};

enum class ModDestination
{
    None,
    Pitch,
    Amp,
    FilterCutoff,
    FilterResonance,
    PulseWidth,
    FmIndex
};

struct ModRoute
{
    ModSource source = ModSource::None;
    ModDestination destination = ModDestination::None;
    double amount = 0.0;
    bool enabled = false;
};

struct ModMatrix
{
    std::array<ModRoute, 8> routes{};
};

struct ModEnvelopeConfig
{
    double attackSec = 0.01;
    double decaySec = 0.1;
    double sustainLevel = 1.0;
    double releaseSec = 0.1;
    double curve = 0.0;
};

struct ModulationConfig
{
    LfoConfig lfo1{};
    ModEnvelopeConfig env2{};
    ModMatrix matrix = []() {
        ModMatrix m{};
        m.routes[0] = ModRoute{
            ModSource::Lfo1,
            ModDestination::FilterCutoff,
            0.0,
            false
        };
        return m;
    }();
};

struct ModulationRuntimeState
{
    double lfo1Phase = 0.0;
    double lfo1ElapsedSec = 0.0;
    ADSRState env2{};
    double env2Value = 0.0;
    // split-rate modulation state:
    // fast group: pitch/pulseWidth/fmIndex
    // slow group: amp/filterCutoff/resonance
    bool splitRatePrepared = false;
    bool splitRateHasFastDest = false;
    bool splitRateHasSlowDest = false;
    bool splitRateUseLfo = false;
    bool splitRateUseEnv = false;
    int splitRateFastIntervalSamples = 2;
    int splitRateSlowIntervalSamples = 4;
    int splitRateSamplesUntilFastUpdate = 0;
    int splitRateSamplesUntilSlowUpdate = 0;
    double splitRateElapsedSec = 0.0;
    double splitRateLastLfo1 = 0.0;
    double splitRateLastEnv2 = 0.0;
    bool splitRateFastInitialized = false;
    bool splitRateSlowInitialized = false;
    double splitRatePitchCurrent = 1.0;
    double splitRatePitchStep = 0.0;
    double splitRatePulseWidthCurrent = 0.0;
    double splitRatePulseWidthStep = 0.0;
    double splitRateFmIndexCurrent = 1.0;
    double splitRateFmIndexStep = 0.0;
    double splitRateAmpCurrent = 1.0;
    double splitRateAmpStep = 0.0;
    double splitRateFilterCurrent = 1.0;
    double splitRateFilterStep = 0.0;
    double splitRateResCurrent = 1.0;
    double splitRateResStep = 0.0;
};

struct ModulationResult
{
    double pitchMul = 1.0;
    double ampMul = 1.0;
    double filterCutoffMul = 1.0;
    double resonanceMul = 1.0;
    double pulseWidthAdd = 0.0;
    double fmIndexMul = 1.0;
};

struct ModulationInput
{
    double velGain = 1.0;
    double modwheel = 0.0;
    double channelPressure = 0.0;
    double polyPressure = 0.0;
};

void ResetModulationState(ModulationRuntimeState& state);
void NoteOnModulation(ModulationRuntimeState& state, const ModulationConfig& cfg);
void NoteOffModulation(ModulationRuntimeState& state);
double StepLfoSample(ModulationRuntimeState& state, const LfoConfig& lfo, double deltaTimeSec);
double StepEnv2Sample(ModulationRuntimeState& state, const ModEnvelopeConfig& env2, double deltaTimeSec);
ModulationResult EvaluateModulation(
    ModulationRuntimeState& state,
    const ModulationConfig& cfg,
    double deltaTimeSec,
    const ModulationInput& input = {});

ModulationResult EvaluateModulationSplitRate(
    ModulationRuntimeState& state,
    const ModulationConfig& cfg,
    double deltaTimeSec,
    const ModulationInput& input = {},
    int controlIntervalSamples = 4);
