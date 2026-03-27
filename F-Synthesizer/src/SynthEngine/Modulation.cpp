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

bool IsFastDestination(ModDestination dst)
{
    return dst == ModDestination::Pitch ||
        dst == ModDestination::PulseWidth ||
        dst == ModDestination::FmIndex;
}

bool IsSlowDestination(ModDestination dst)
{
    return dst == ModDestination::Amp ||
        dst == ModDestination::FilterCutoff ||
        dst == ModDestination::FilterResonance;
}

struct RouteSourceValues
{
    double lfo1 = 0.0;
    double env2 = 0.0;
    double velocity = 0.0;
    double channelPressure = 0.0;
    double polyPressure = 0.0;
    double modWheel = 0.0;
};

double SourceValueByRoute(const RouteSourceValues& s, ModSource source)
{
    switch (source)
    {
    case ModSource::Lfo1: return s.lfo1;
    case ModSource::Env2: return s.env2;
    case ModSource::Velocity: return s.velocity;
    case ModSource::ChannelPressure: return s.channelPressure;
    case ModSource::PolyPressure: return s.polyPressure;
    case ModSource::ModWheel: return s.modWheel;
    case ModSource::None:
    default:
        return 0.0;
    }
}

void ComputeRouteTargets(
    const ModulationConfig& cfg,
    const RouteSourceValues& s,
    bool fastGroup,
    double& outPitchMul,
    double& outPulseWidthAdd,
    double& outFmIndexMul,
    double& outAmpMul,
    double& outFilterMul,
    double& outResMul)
{
    outPitchMul = 1.0;
    outPulseWidthAdd = 0.0;
    outFmIndexMul = 1.0;
    outAmpMul = 1.0;
    outFilterMul = 1.0;
    outResMul = 1.0;

    for (const ModRoute& route : cfg.matrix.routes)
    {
        if (!route.enabled ||
            route.source == ModSource::None ||
            route.destination == ModDestination::None ||
            route.amount == 0.0)
        {
            continue;
        }

        const bool inGroup = fastGroup ? IsFastDestination(route.destination) : IsSlowDestination(route.destination);
        if (!inGroup)
        {
            continue;
        }

        const double srcValue = SourceValueByRoute(s, route.source);
        const double value = srcValue * std::clamp(route.amount, -1.0, 1.0);
        switch (route.destination)
        {
        case ModDestination::Pitch:
            outPitchMul *= std::exp2(value);
            break;
        case ModDestination::PulseWidth:
            outPulseWidthAdd += value * 0.45;
            break;
        case ModDestination::FmIndex:
            outFmIndexMul *= (1.0 + value);
            break;
        case ModDestination::Amp:
            outAmpMul *= (1.0 + value);
            break;
        case ModDestination::FilterCutoff:
            outFilterMul *= (1.0 + value);
            break;
        case ModDestination::FilterResonance:
            outResMul *= (1.0 + value);
            break;
        default:
            break;
        }
    }

    outFmIndexMul = (std::max)(0.0, outFmIndexMul);
    outAmpMul = (std::max)(0.0, outAmpMul);
    outFilterMul = (std::max)(0.0, outFilterMul);
    outResMul = std::clamp(outResMul, 0.0, 10.0);
}
} // namespace

void ResetModulationState(ModulationRuntimeState& state)
{
    state.lfo1Phase = 0.0;
    state.lfo1ElapsedSec = 0.0;
    state.env2 = ADSRState{};
    state.env2Value = 0.0;
    state.splitRatePrepared = false;
    state.splitRateHasFastDest = false;
    state.splitRateHasSlowDest = false;
    state.splitRateUseLfo = false;
    state.splitRateUseEnv = false;
    state.splitRateFastIntervalSamples = 2;
    state.splitRateSlowIntervalSamples = 4;
    state.splitRateSamplesUntilFastUpdate = 0;
    state.splitRateSamplesUntilSlowUpdate = 0;
    state.splitRateElapsedSec = 0.0;
    state.splitRateLastLfo1 = 0.0;
    state.splitRateLastEnv2 = 0.0;
    state.splitRateFastInitialized = false;
    state.splitRateSlowInitialized = false;
    state.splitRatePitchCurrent = 1.0;
    state.splitRatePitchStep = 0.0;
    state.splitRatePulseWidthCurrent = 0.0;
    state.splitRatePulseWidthStep = 0.0;
    state.splitRateFmIndexCurrent = 1.0;
    state.splitRateFmIndexStep = 0.0;
    state.splitRateAmpCurrent = 1.0;
    state.splitRateAmpStep = 0.0;
    state.splitRateFilterCurrent = 1.0;
    state.splitRateFilterStep = 0.0;
    state.splitRateResCurrent = 1.0;
    state.splitRateResStep = 0.0;
}

void NoteOnModulation(ModulationRuntimeState& state, const ModulationConfig& cfg)
{
    NoteOn(state.env2);
    state.lfo1ElapsedSec = 0.0;
    if (cfg.lfo1.keySync)
    {
        state.lfo1Phase = 0.0;
    }
    state.splitRatePrepared = false;
    state.splitRateSamplesUntilFastUpdate = 0;
    state.splitRateSamplesUntilSlowUpdate = 0;
    state.splitRateElapsedSec = 0.0;
    state.splitRateLastLfo1 = 0.0;
    state.splitRateLastEnv2 = 0.0;
    state.splitRateFastInitialized = false;
    state.splitRateSlowInitialized = false;
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

void PrepareSplitRateRouting(ModulationRuntimeState& state, const ModulationConfig& cfg)
{
    state.splitRatePrepared = true;
    state.splitRateHasFastDest = false;
    state.splitRateHasSlowDest = false;
    state.splitRateUseLfo = false;
    state.splitRateUseEnv = false;
    state.splitRateFastInitialized = false;
    state.splitRateSlowInitialized = false;
    state.splitRatePitchCurrent = 1.0;
    state.splitRatePitchStep = 0.0;
    state.splitRatePulseWidthCurrent = 0.0;
    state.splitRatePulseWidthStep = 0.0;
    state.splitRateFmIndexCurrent = 1.0;
    state.splitRateFmIndexStep = 0.0;
    state.splitRateAmpCurrent = 1.0;
    state.splitRateAmpStep = 0.0;
    state.splitRateFilterCurrent = 1.0;
    state.splitRateFilterStep = 0.0;
    state.splitRateResCurrent = 1.0;
    state.splitRateResStep = 0.0;
    state.splitRateSamplesUntilFastUpdate = 0;
    state.splitRateSamplesUntilSlowUpdate = 0;
    state.splitRateElapsedSec = 0.0;
    state.splitRateLastLfo1 = 0.0;
    state.splitRateLastEnv2 = 0.0;

    for (const ModRoute& route : cfg.matrix.routes)
    {
        if (!IsActiveRoute(route))
        {
            continue;
        }
        state.splitRateHasFastDest = state.splitRateHasFastDest || IsFastDestination(route.destination);
        state.splitRateHasSlowDest = state.splitRateHasSlowDest || IsSlowDestination(route.destination);
        state.splitRateUseLfo = state.splitRateUseLfo || route.source == ModSource::Lfo1;
        state.splitRateUseEnv = state.splitRateUseEnv || route.source == ModSource::Env2;
    }
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
            out.pitchMul *= std::exp2(value);
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

ModulationResult EvaluateModulationSplitRate(
    ModulationRuntimeState& state,
    const ModulationConfig& cfg,
    double deltaTimeSec,
    const ModulationInput& input,
    int controlIntervalSamples)
{
    ModulationResult out{};

    if (!state.splitRatePrepared)
    {
        PrepareSplitRateRouting(state, cfg);
    }
    state.splitRateSlowIntervalSamples = (std::max)(1, controlIntervalSamples);
    state.splitRateFastIntervalSamples = (std::max)(1, state.splitRateSlowIntervalSamples / 2);

    if (!state.splitRateHasFastDest && !state.splitRateHasSlowDest)
    {
        return out;
    }

    state.splitRateElapsedSec += deltaTimeSec;

    const bool needFastUpdate = state.splitRateHasFastDest &&
        state.splitRateSamplesUntilFastUpdate <= 0;
    const bool needSlowUpdate = state.splitRateHasSlowDest &&
        state.splitRateSamplesUntilSlowUpdate <= 0;

    RouteSourceValues src{};
    const bool needSourceStep = (needFastUpdate || needSlowUpdate) && (state.splitRateUseLfo || state.splitRateUseEnv);

    if (needSourceStep)
    {
        const double stepDeltaSec = state.splitRateElapsedSec;
        state.splitRateElapsedSec = 0.0;
        if (state.splitRateUseLfo)
        {
            src.lfo1 = StepLfoSample(state, cfg.lfo1, stepDeltaSec);
            state.splitRateLastLfo1 = src.lfo1;
        }
        if (state.splitRateUseEnv)
        {
            src.env2 = StepEnv2Sample(state, cfg.env2, stepDeltaSec);
            state.splitRateLastEnv2 = src.env2;
        }
    }
    else
    {
        src.lfo1 = state.splitRateLastLfo1;
        src.env2 = state.splitRateLastEnv2;
    }

    src.velocity = std::clamp(input.velGain, 0.0, 1.0);
    src.channelPressure = std::clamp(input.channelPressure, 0.0, 1.0);
    src.polyPressure = std::clamp(input.polyPressure, 0.0, 1.0);
    src.modWheel = std::clamp(input.modwheel, 0.0, 1.0);
    if (state.splitRateUseLfo)
    {
        src.lfo1 *= (1.0 + src.modWheel);
    }

    if (state.splitRateHasFastDest && needFastUpdate)
    {
        double pitchTarget = 1.0;
        double pulseTarget = 0.0;
        double fmTarget = 1.0;
        double dummyAmp = 1.0;
        double dummyFilter = 1.0;
        double dummyRes = 1.0;
        ComputeRouteTargets(cfg, src, true, pitchTarget, pulseTarget, fmTarget, dummyAmp, dummyFilter, dummyRes);

        if (!state.splitRateFastInitialized)
        {
            state.splitRatePitchCurrent = pitchTarget;
            state.splitRatePulseWidthCurrent = pulseTarget;
            state.splitRateFmIndexCurrent = fmTarget;
            state.splitRatePitchStep = 0.0;
            state.splitRatePulseWidthStep = 0.0;
            state.splitRateFmIndexStep = 0.0;
            state.splitRateFastInitialized = true;
        }
        else
        {
            const double n = static_cast<double>(state.splitRateFastIntervalSamples);
            state.splitRatePitchStep = (pitchTarget - state.splitRatePitchCurrent) / n;
            state.splitRatePulseWidthStep = (pulseTarget - state.splitRatePulseWidthCurrent) / n;
            state.splitRateFmIndexStep = (fmTarget - state.splitRateFmIndexCurrent) / n;
        }
        state.splitRateSamplesUntilFastUpdate = state.splitRateFastIntervalSamples;
    }

    if (state.splitRateHasSlowDest && needSlowUpdate)
    {
        double dummyPitch = 1.0;
        double dummyPulse = 0.0;
        double dummyFm = 1.0;
        double targetAmp = 1.0;
        double targetFilter = 1.0;
        double targetRes = 1.0;
        ComputeRouteTargets(cfg, src, false, dummyPitch, dummyPulse, dummyFm, targetAmp, targetFilter, targetRes);

        if (!state.splitRateSlowInitialized)
        {
            state.splitRateAmpCurrent = targetAmp;
            state.splitRateFilterCurrent = targetFilter;
            state.splitRateResCurrent = targetRes;
            state.splitRateAmpStep = 0.0;
            state.splitRateFilterStep = 0.0;
            state.splitRateResStep = 0.0;
            state.splitRateSlowInitialized = true;
        }
        else
        {
            const double n = static_cast<double>(state.splitRateSlowIntervalSamples);
            state.splitRateAmpStep = (targetAmp - state.splitRateAmpCurrent) / n;
            state.splitRateFilterStep = (targetFilter - state.splitRateFilterCurrent) / n;
            state.splitRateResStep = (targetRes - state.splitRateResCurrent) / n;
        }
        state.splitRateSamplesUntilSlowUpdate = state.splitRateSlowIntervalSamples;
    }

    if (state.splitRateHasFastDest)
    {
        out.pitchMul = state.splitRatePitchCurrent;
        out.pulseWidthAdd = state.splitRatePulseWidthCurrent;
        out.fmIndexMul = (std::max)(0.0, state.splitRateFmIndexCurrent);
        if (state.splitRateSamplesUntilFastUpdate > 0)
        {
            state.splitRatePitchCurrent += state.splitRatePitchStep;
            state.splitRatePulseWidthCurrent += state.splitRatePulseWidthStep;
            state.splitRateFmIndexCurrent += state.splitRateFmIndexStep;
            state.splitRateSamplesUntilFastUpdate--;
        }
    }
    else
    {
        out.pitchMul = 1.0;
        out.pulseWidthAdd = 0.0;
        out.fmIndexMul = 1.0;
    }

    if (state.splitRateHasSlowDest)
    {
        out.ampMul = (std::max)(0.0, state.splitRateAmpCurrent);
        out.filterCutoffMul = (std::max)(0.0, state.splitRateFilterCurrent);
        out.resonanceMul = std::clamp(state.splitRateResCurrent, 0.0, 10.0);
        if (state.splitRateSamplesUntilSlowUpdate > 0)
        {
            state.splitRateAmpCurrent += state.splitRateAmpStep;
            state.splitRateFilterCurrent += state.splitRateFilterStep;
            state.splitRateResCurrent += state.splitRateResStep;
            state.splitRateSamplesUntilSlowUpdate--;
        }
    }
    else
    {
        out.ampMul = 1.0;
        out.filterCutoffMul = 1.0;
        out.resonanceMul = 1.0;
    }

    out.fmIndexMul = (std::max)(0.0, out.fmIndexMul);

    return out;
}
