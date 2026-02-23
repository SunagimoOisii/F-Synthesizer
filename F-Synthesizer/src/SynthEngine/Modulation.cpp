#include "SynthEngine/Modulation.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr double kPi = 3.14159265358979323846;

double WrapPhase(double phase)
{
    phase -= std::floor(phase);
    if (phase < 0.0)
    {
        phase += 1.0;
    }
    return phase;
}

double SampleLfoWave(LfoWave wave, double phase)
{
    phase = WrapPhase(phase);
    switch (wave)
    {
    case LfoWave::Sine:
        return std::sin(2.0 * kPi * phase);
    case LfoWave::Triangle:
        return 1.0 - 4.0 * std::fabs(phase - 0.5);
    }
    return 0.0;
}
} // namespace

void ResetModulationState(ModulationRuntimeState& state)
{
    state.lfo1Phase = 0.0;
    state.env2 = ADSRState{};
    state.env2Value = 0.0;
}

void NoteOnModulation(ModulationRuntimeState& state)
{
    NoteOn(state.env2);
}

void NoteOffModulation(ModulationRuntimeState& state)
{
    NoteOff(state.env2);
}

double StepLfoSample(double& phase, const LfoConfig& lfo, double deltaTimeSec)
{
    const double rate = std::clamp(lfo.rateHz, 0.0, 100.0);
    phase = WrapPhase(phase + (rate * deltaTimeSec));
    double raw = SampleLfoWave(lfo.wave, phase);
    if (!lfo.bipolar)
    {
        raw = (raw * 0.5) + 0.5;
    }
    raw *= std::clamp(lfo.depth, 0.0, 1.0);
    return raw;
}

double StepEnv2Sample(ModulationRuntimeState& state, const ModEnvelopeConfig& env2, double deltaTimeSec)
{
    const double v = StepADSR(
        state.env2,
        deltaTimeSec,
        (std::max)(0.0, env2.attackSec),
        (std::max)(0.0, env2.decaySec),
        std::clamp(env2.sustainLevel, 0.0, 1.0),
        (std::max)(0.0, env2.releaseSec));
    state.env2Value = v;
    return v;
}

bool IsActiveRoute(const ModRoute& route)
{
    return route.enabled &&
        route.source != ModSource::None &&
        route.destination != ModDestination::None &&
        route.amount != 0.0;
}

ModulationResult EvaluateModulation(
    ModulationRuntimeState& state,
    const ModulationConfig& cfg,
    double deltaTimeSec)
{
    ModulationResult out{};
    bool hasActiveRoute = false;
    bool useLfo1 = false;
    bool useEnv2 = false;
    for (const ModRoute& route : cfg.matrix.routes)
    {
        if (!IsActiveRoute(route))
        {
            continue;
        }
        hasActiveRoute = true;
        useLfo1 = useLfo1 || route.source == ModSource::Lfo1;
        useEnv2 = useEnv2 || route.source == ModSource::Env2;
    }
    if (!hasActiveRoute)
    {
        return out;
    }

    const double lfo1 = useLfo1 ? StepLfoSample(state.lfo1Phase, cfg.lfo1, deltaTimeSec) : 0.0;
    const double env2 = useEnv2 ? StepEnv2Sample(state, cfg.env2, deltaTimeSec) : 0.0;

    for (const ModRoute& route : cfg.matrix.routes)
    {
        if (!IsActiveRoute(route))
        {
            continue;
        }
        const double srcValue = (route.source == ModSource::Lfo1) ? lfo1 :
            ((route.source == ModSource::Env2) ? env2 : 0.0);
        const double value = srcValue * std::clamp(route.amount, -1.0, 1.0);
        switch (route.destination)
        {
        case ModDestination::Pitch:
            // 1.0 = no change, 12 semitones per +1.0 amount.
            out.pitchMul *= std::pow(2.0, value);
            break;
        case ModDestination::Amp:
            out.ampMul *= (1.0 + value);
            break;
        case ModDestination::FilterCutoff:
            out.filterCutoffMul *= (1.0 + value);
            break;
        case ModDestination::None:
        default:
            break;
        }
    }

    out.ampMul = (std::max)(0.0, out.ampMul);
    out.filterCutoffMul = (std::max)(0.0, out.filterCutoffMul);
    return out;
}
