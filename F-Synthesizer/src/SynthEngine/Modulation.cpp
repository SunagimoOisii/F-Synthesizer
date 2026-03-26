#include "SynthEngine/Modulation.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

// Modulation matrix の実行部。
// 呼び出し経路: Voices初期化/NoteOn・NoteOff -> Renderer内で EvaluateModulation を1sampleごとに評価。
// 責務境界: 設定値の妥当性検証は上位層、実行時の値生成と合成はSynthEngine側で担当する。
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
    const double wrappedPhase = WrapPhase(phase);
    switch (wave)
    {
    case LfoWave::Sine:
        return std::sin(2.0 * kPi * wrappedPhase);
    case LfoWave::Triangle:
        return 1.0 - 4.0 * std::fabs(wrappedPhase - 0.5);
    case LfoWave::Square:
        return (wrappedPhase < 0.5) ? 1.0 : -1.0;
    case LfoWave::Saw:
        return 2.0 * wrappedPhase - 1.0;
    case LfoWave::SampleAndHold:
    {
        uint32_t seed = static_cast<uint32_t>(std::floor(phase));
        seed ^= seed << 13u;
        seed ^= seed >> 17u;
        seed ^= seed << 5u;
        return static_cast<double>(seed) / 2147483647.5 - 1.0;
    }
    }
    return 0.0;
}

double ApplyEnvelopeCurve(double value, double curve)
{
    const double clampedValue = std::clamp(value, 0.0, 1.0);
    const double clampedCurve = std::clamp(curve, 0.0, 1.0);
    if (clampedCurve <= 0.0)
    {
        return clampedValue;
    }
    const double exponent = 1.0 / (1.0 + clampedCurve * 4.0);
    return std::pow(clampedValue, exponent);
}
} // namespace

void ResetModulationState(ModulationRuntimeState& state)
{
    state.lfo1Phase = 0.0;
    state.lfo1ElapsedSec = 0.0;
    state.env2 = ADSRState{};
    state.env2Value = 0.0;
}

void NoteOnModulation(ModulationRuntimeState& state, const ModulationConfig& cfg)
{
    NoteOn(state.env2);
    state.lfo1ElapsedSec = 0.0;
    if (cfg.lfo1.keySync)
    {
        state.lfo1Phase = 0.0;
    }
}

void NoteOffModulation(ModulationRuntimeState& state)
{
    NoteOff(state.env2);
}

double StepLfoSample(ModulationRuntimeState& state, const LfoConfig& lfo, double deltaTimeSec)
{
    state.lfo1ElapsedSec += deltaTimeSec;

    const double delayEndSec = (std::max)(0.0, lfo.delayMs) * 0.001;
    if (state.lfo1ElapsedSec < delayEndSec)
    {
        return 0.0;
    }

    const double rate = std::clamp(lfo.rateHz, 0.0, 100.0);
    state.lfo1Phase += (rate * deltaTimeSec);
    double raw = SampleLfoWave(lfo.wave, state.lfo1Phase);
    if (!lfo.bipolar)
    {
        // unipolar 指定では -1..1 を 0..1 へ変換してから depth を適用する。
        raw = (raw * 0.5) + 0.5;
    }
    raw *= std::clamp(lfo.depth, 0.0, 1.0);

    const double fadeTotalSec = (std::max)(0.0, lfo.fadeMs) * 0.001;
    if (fadeTotalSec > 0.0)
    {
        const double activeElapsedSec = state.lfo1ElapsedSec - delayEndSec;
        const double fadeFactor = std::clamp(activeElapsedSec / fadeTotalSec, 0.0, 1.0);
        raw *= fadeFactor;
    }
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
    const double curved = ApplyEnvelopeCurve(v, env2.curve);
    state.env2Value = curved;
    return curved;
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
    double deltaTimeSec,
    const ModulationInput& input)
{
    ModulationResult out{};
    bool hasActiveRoute = false;
    bool useLfo1 = false;
    bool useEnv2 = false;
    bool useVelocity = false;
    bool useChannelPressure = false;
    bool usePolyPressure = false;
    bool useModWheel = false;

    // 目的: 未使用ソースのStep計算を省いて、1sampleあたりの固定コストを減らす。
    // 前提: route有効判定はこの関数呼び出し中に変化しない。
    // トレードオフ: route探索が2パスになるが、未使用ソースの評価は避けられる。
    for (const ModRoute& route : cfg.matrix.routes)
    {
        if (!IsActiveRoute(route))
        {
            continue;
        }
        hasActiveRoute = true;
        useLfo1 = useLfo1 || route.source == ModSource::Lfo1;
        useEnv2 = useEnv2 || route.source == ModSource::Env2;
        useVelocity = useVelocity || route.source == ModSource::Velocity;
        useChannelPressure = useChannelPressure || route.source == ModSource::ChannelPressure;
        usePolyPressure = usePolyPressure || route.source == ModSource::PolyPressure;
        useModWheel = useModWheel || route.source == ModSource::ModWheel;
    }
    if (!hasActiveRoute)
    {
        return out;
    }

    double lfo1 = useLfo1 ? StepLfoSample(state, cfg.lfo1, deltaTimeSec) : 0.0;
    const double env2 = useEnv2 ? StepEnv2Sample(state, cfg.env2, deltaTimeSec) : 0.0;
    const double velocity = useVelocity ? std::clamp(input.velGain, 0.0, 1.0) : 0.0;
    const double channelPressure = useChannelPressure ? std::clamp(input.channelPressure, 0.0, 1.0) : 0.0;
    const double polyPressure = usePolyPressure ? std::clamp(input.polyPressure, 0.0, 1.0) : 0.0;
    const double modWheel = useModWheel ? std::clamp(input.modwheel, 0.0, 1.0) : 0.0;
    lfo1 *= (1.0 + std::clamp(input.modwheel, 0.0, 1.0));

    for (const ModRoute& route : cfg.matrix.routes)
    {
        if (!IsActiveRoute(route))
        {
            continue;
        }
        double srcValue = 0.0;
        switch (route.source)
        {
        case ModSource::Lfo1: srcValue = lfo1; break;
        case ModSource::Env2: srcValue = env2; break;
        case ModSource::Velocity: srcValue = velocity; break;
        case ModSource::ChannelPressure: srcValue = channelPressure; break;
        case ModSource::PolyPressure: srcValue = polyPressure; break;
        case ModSource::ModWheel: srcValue = modWheel; break;
        case ModSource::None:
        default:
            break;
        }
        const double value = srcValue * std::clamp(route.amount, -1.0, 1.0);
        switch (route.destination)
        {
        case ModDestination::Pitch:
            // value をオクターブ比へ変換し、Pitchへ乗算で合成する。
            out.pitchMul *= std::pow(2.0, value);
            break;
        case ModDestination::Amp:
            out.ampMul *= (1.0 + value);
            break;
        case ModDestination::FilterCutoff:
            out.filterCutoffMul *= (1.0 + value);
            break;
        case ModDestination::FilterResonance:
            out.resonanceMul *= (1.0 + value);
            break;
        case ModDestination::PulseWidth:
            out.pulseWidthAdd += value * 0.45;
            break;
        case ModDestination::FmIndex:
            out.fmIndexMul *= (1.0 + value);
            break;
        case ModDestination::None:
        default:
            break;
        }
    }

    out.ampMul = (std::max)(0.0, out.ampMul);
    out.filterCutoffMul = (std::max)(0.0, out.filterCutoffMul);
    out.resonanceMul = std::clamp(out.resonanceMul, 0.0, 10.0);
    out.fmIndexMul = (std::max)(0.0, out.fmIndexMul);
    return out;
}
