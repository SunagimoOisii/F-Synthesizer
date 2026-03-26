#pragma once

#include <array>

#include "synth/Envelope.h"

enum class LfoWave
{
    Sine,
    Triangle
};

struct LfoConfig
{
    LfoWave wave = LfoWave::Sine;
    double rateHz = 5.0;
    double depth = 0.0;
    bool bipolar = true;
};

enum class ModSource
{
    None,
    Lfo1,
    Env2,
    Velocity,
    ChannelPressure,
    PolyPressure
};

enum class ModDestination
{
    None,
    Pitch,
    Amp,
    FilterCutoff,
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
    ADSRState env2{};
    double env2Value = 0.0;
};

struct ModulationResult
{
    double pitchMul = 1.0;
    double ampMul = 1.0;
    double filterCutoffMul = 1.0;
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
void NoteOnModulation(ModulationRuntimeState& state);
void NoteOffModulation(ModulationRuntimeState& state);
double StepLfoSample(double& phase, const LfoConfig& lfo, double deltaTimeSec);
double StepEnv2Sample(ModulationRuntimeState& state, const ModEnvelopeConfig& env2, double deltaTimeSec);
ModulationResult EvaluateModulation(
    ModulationRuntimeState& state,
    const ModulationConfig& cfg,
    double deltaTimeSec,
    const ModulationInput& input = {});
