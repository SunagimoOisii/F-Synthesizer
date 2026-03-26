#include "Internal.h"

#include <algorithm>
#include <cmath>
#include <type_traits>

#include "synth/Oscillator.h"

namespace
{
constexpr double kPi = 3.14159265358979323846;

struct VoiceRenderInput
{
    double dt = 0.0;
    double mixGainL = 1.0;
    double mixGainR = 1.0;
    double pitchFactor = 1.0;
    double ccGain = 1.0;
    double velGain = 1.0;
    double envGain = 1.0;
    double modwheel = 0.0;
    double channelPressure = 0.0;
    double polyPressure = 0.0;
    double brightness = 0.5;
    double resonance = 0.5;
};

double TimeScaleFromOffset(double offset)
{
    return std::pow(2.0, std::clamp(offset, -1.0, 1.0) * 2.0);
}

double CutoffScaleFromBrightness(double brightness)
{
    return std::pow(2.0, (std::clamp(brightness, 0.0, 1.0) - 0.5) * 4.0);
}

double ResonanceScaleFromCc(double resonance)
{
    return std::pow(2.0, (std::clamp(resonance, 0.0, 1.0) - 0.5) * 2.0);
}

double SourceFilterResonance(const SourceConfig& src)
{
    return std::visit([](const auto& sourceCfg) -> double
    {
        using T = std::decay_t<decltype(sourceCfg)>;
        if constexpr (
            std::is_same_v<T, WaveformConfig> ||
            std::is_same_v<T, FmConfig> ||
            std::is_same_v<T, AnalogConfig> ||
            std::is_same_v<T, NoiseConfig>)
        {
            return sourceCfg.filterResonance;
        }
        return 0.707;
    }, src);
}

double WrapPhase(double phase)
{
    phase -= std::floor(phase);
    if (phase < 0.0)
    {
        phase += 1.0;
    }
    return phase;
}

// オペレータ1個分の位相サンプリング（FM変調込み）。
// modSample: 前段オペレータの出力。0.0 = 変調なし。
double SampleOp(
    WaveType wave,
    double phase,
    double modSample,
    double index)
{
    return SampleWavePhase(wave, phase + modSample * index);
}

void EnsureDrumFilters(DrumVoiceState& ds, double hpCut, double lpCut, int sampleRate)
{
    // 初回のみ係数を計算し、同一voice中は再利用する。
    if (ds.hpAlpha <= 0.0)
    {
        ds.hpAlpha = std::exp(-2.0 * kPi * hpCut / sampleRate);
    }
    if (ds.lpAlpha <= 0.0)
    {
        ds.lpAlpha = std::exp(-2.0 * kPi * lpCut / sampleRate);
    }
}

void PrepareDrumRelease(Voice& voices, const DrumVoiceState& ds, size_t i)
{
    // Drum は attack+decay 到達で自動 NoteOff へ移す。
    if (voices.released[i] == 0 && ds.time >= (voices.attackSec[i] + voices.decaySec[i]))
    {
        NoteOff(voices.env[i]);
        voices.released[i] = 1;
    }
}

double RenderKickSample(const DrumConfig& src, Voice& voices, DrumVoiceState& ds, size_t i, int sampleRate)
{
    double pitchFactor = 1.0;
    if (ds.pitchDecaySec > 0.0)
    {
        pitchFactor += (ds.pitchDrop - 1.0) * std::exp(-ds.time / ds.pitchDecaySec);
    }
    const double freq = ds.baseFreq * pitchFactor;
    voices.phase[i] += (freq / sampleRate);
    if (voices.phase[i] >= 1.0) voices.phase[i] -= 1.0;
    return SampleWavePhase(WaveType::Sine, voices.phase[i]);
}

double RenderSnareSample(const DrumConfig& src, Voice& voices, DrumVoiceState& ds, size_t i, int sampleRate)
{
    const double toneLevel = (src.toneLevel > 0.0) ? src.toneLevel : 0.3;
    const double noiseLevel = (src.noiseLevel > 0.0) ? src.noiseLevel : 0.7;
    const double hpCut = (src.hpCut > 0.0) ? src.hpCut : 1200.0;
    const double lpCut = (src.lpCut > 0.0) ? src.lpCut : 6000.0;
    const WaveType toneWave = static_cast<WaveType>(src.toneWave);
    const NoiseType noiseType = static_cast<NoiseType>(src.noiseType);
    EnsureDrumFilters(ds, hpCut, lpCut, sampleRate);
    voices.phase[i] += ds.baseFreq / sampleRate;
    if (voices.phase[i] >= 1.0) voices.phase[i] -= 1.0;
    const double toneInc = ds.baseFreq / sampleRate;
    const double tone = SampleWavePhase(toneWave, voices.phase[i], toneInc);
    const double noise = SampleNoise(noiseType);
    const double hp = ds.hpAlpha * (ds.hpPrev + noise - ds.noisePrev);
    const double lp = (1.0 - ds.lpAlpha) * hp + ds.lpAlpha * ds.lpPrev;
    ds.noisePrev = noise;
    ds.hpPrev = hp;
    ds.lpPrev = lp;
    return toneLevel * tone + noiseLevel * lp;
}

double RenderHatSample(const DrumConfig& src, Voice& voices, DrumVoiceState& ds, size_t i, int sampleRate)
{
    const double noiseLevel = (src.noiseLevel > 0.0) ? src.noiseLevel : 1.0;
    const double hpCut = (src.hpCut > 0.0) ? src.hpCut : 6000.0;
    const double lpCut = (src.lpCut > 0.0) ? src.lpCut : 12000.0;
    const double toneFreq = (src.toneFreq > 0.0) ? src.toneFreq : 8000.0;
    const double toneLevel = (src.toneLevel > 0.0) ? src.toneLevel : 0.2;
    const WaveType toneWave = static_cast<WaveType>(src.toneWave);
    const NoiseType noiseType = static_cast<NoiseType>(src.noiseType);
    EnsureDrumFilters(ds, hpCut, lpCut, sampleRate);
    const double noise = SampleNoise(noiseType);
    const double hp = ds.hpAlpha * (ds.hpPrev + noise - ds.noisePrev);
    const double lp = (1.0 - ds.lpAlpha) * hp + ds.lpAlpha * ds.lpPrev;
    ds.noisePrev = noise;
    ds.hpPrev = hp;
    ds.lpPrev = lp;
    voices.phase[i] += toneFreq / sampleRate;
    if (voices.phase[i] >= 1.0) voices.phase[i] -= 1.0;
    const double toneInc = toneFreq / sampleRate;
    const double tone = SampleWavePhase(toneWave, voices.phase[i], toneInc);
    return noiseLevel * lp + toneLevel * tone;
}

double RenderDrumSample(const DrumConfig& src, Voice& voices, size_t i, double dt, int sampleRate)
{
    auto& ds = std::get<DrumVoiceState>(voices.sourceState[i]);
    PrepareDrumRelease(voices, ds, i);

    double w = 0.0;
    if (src.type == DrumType::Kick)
    {
        w = RenderKickSample(src, voices, ds, i, sampleRate);
    }
    else if (src.type == DrumType::Snare)
    {
        w = RenderSnareSample(src, voices, ds, i, sampleRate);
    }
    else if (src.type == DrumType::Hat)
    {
        w = RenderHatSample(src, voices, ds, i, sampleRate);
    }

    ds.time += dt;
    return w;
}

void RenderWaveformSource(
    const WaveformConfig& src,
    Voice& voices,
    size_t i,
    const VoiceRenderInput& in,
    SourceRenderFrame& frame)
{
    auto& ws = std::get<WaveformVoiceState>(voices.sourceState[i]);
    ModulationResult mod = EvaluateModulation(
        ws.modulation,
        src.modulation,
        in.dt,
        ModulationInput{ in.velGain, in.modwheel, in.channelPressure, in.polyPressure });
    double pitchMul = mod.pitchMul;
    if (src.smoothing.enabled && src.smoothing.pitchEnabled)
    {
        SetSmoothedTarget(ws.pitchSmoothing, pitchMul);
        pitchMul = StepSmoothedParam(ws.pitchSmoothing);
    }
    if (src.arpeggio.enabled && src.arpeggio.steps > 0)
    {
        ws.arpElapsedSec += in.dt;
        const int steps = std::clamp(src.arpeggio.steps, 1, 8);
        const double stepDur = 1.0 / (std::max)(0.5, src.arpeggio.rateHz);
        while (ws.arpElapsedSec >= stepDur)
        {
            ws.arpElapsedSec -= stepDur;
            ws.arpStep = (ws.arpStep + 1) % steps;
        }
        const int semitoneOffset = src.arpeggio.semitones[static_cast<size_t>(ws.arpStep)];
        pitchMul *= std::pow(2.0, static_cast<double>(semitoneOffset) / 12.0);
    }

    const double phaseInc = voices.phaseInc[i] * in.pitchFactor * pitchMul;
    const int unisonVoices = std::clamp(src.unisonVoices, 1, 8);
    const double detuneCents = std::clamp(src.unisonDetuneCents, 0.0, 120.0);
    const double spread = std::clamp(src.unisonSpread, 0.0, 1.0);
    const double subOscLevel = std::clamp(src.subOscLevel, 0.0, 2.0);
    const double effectivePW = std::clamp(src.pulseWidth + mod.pulseWidthAdd, 0.05, 0.95);

    double unisonSum = 0.0;
    for (int uv = 0; uv < unisonVoices; uv++)
    {
        const double pos = (unisonVoices <= 1) ? 0.0 : (static_cast<double>(uv) / (unisonVoices - 1));
        const double centered = (pos * 2.0) - 1.0;
        const double cents = centered * detuneCents;
        const double ratio = std::pow(2.0, cents / 1200.0);
        const double phaseOffset = centered * spread * 0.08;
        const double uvPhase = WrapPhase(voices.phase[i] * ratio + phaseOffset);
        const double uvInc = phaseInc * ratio;
        unisonSum += SampleWavePhase(src.wave, uvPhase, uvInc, effectivePW);
    }

    double mainWave = unisonSum / unisonVoices;
    if (subOscLevel > 0.0)
    {
        const double subPhase = WrapPhase(voices.phase[i] * 0.5);
        const double subWave = SampleWavePhase(src.wave, subPhase, phaseInc * 0.5, effectivePW);
        mainWave = (mainWave + (subOscLevel * subWave)) / (1.0 + subOscLevel);
    }
    if (src.ringModEnabled && src.ringModMix > 0.0)
    {
        const double ringInc = phaseInc * std::clamp(src.ringModRatio, 0.125, 16.0);
        ws.ringPhase = WrapPhase(ws.ringPhase + ringInc);
        const double ringSignal = std::sin(2.0 * kPi * ws.ringPhase);
        const double mix = std::clamp(src.ringModMix, 0.0, 1.0);
        mainWave *= (1.0 - mix) + mix * ringSignal;
    }

    frame.sample = mainWave;
    frame.ampMul = mod.ampMul;
    frame.shaperKind = CommonShaperKind::BiquadFilter;
    double effectiveCutoff = src.filterCutoffHz * mod.filterCutoffMul;
    if (src.filterKeytrack != 0.0)
    {
        const double keytrackRatio = std::pow(2.0, src.filterKeytrack * (voices.noteNumber[i] - 60) / 12.0);
        effectiveCutoff *= keytrackRatio;
    }
    frame.shaperCutoffHz = effectiveCutoff;
    frame.shaperResonanceMul = mod.resonanceMul;
    frame.shaperDrive = src.drive;

    if (src.hardSyncEnabled)
    {
        const double syncInc = phaseInc * std::clamp(src.hardSyncRatio, 0.5, 8.0);
        const double prevSync = ws.syncPhase;
        ws.syncPhase = WrapPhase(ws.syncPhase + syncInc);
        if (ws.syncPhase < prevSync)
        {
            voices.phase[i] = 0.0;
        }
        else
        {
            voices.phase[i] = WrapPhase(voices.phase[i] + phaseInc);
        }
    }
    else
    {
        voices.phase[i] = WrapPhase(voices.phase[i] + phaseInc);
    }
}

void RenderAnalogSource(
    const AnalogConfig& src,
    Voice& voices,
    size_t i,
    const VoiceRenderInput& in,
    SourceRenderFrame& frame)
{
    auto& as = std::get<AnalogVoiceState>(voices.sourceState[i]);
    ModulationResult mod = EvaluateModulation(
        as.modulation,
        src.modulation,
        in.dt,
        ModulationInput{ in.velGain, in.modwheel, in.channelPressure, in.polyPressure });

    double pitchMul = mod.pitchMul;

    // ドリフト適用: ボイスごとの正弦LFOでピッチをゆらす。
    if (src.driftDepthCents > 0.0)
    {
        const double driftSine = std::sin((as.driftPhase + as.driftPhaseOffset) * 2.0 * kPi);
        const double driftCents = driftSine * src.driftDepthCents;
        pitchMul *= std::pow(2.0, driftCents / 1200.0);
        as.driftPhase += src.driftRateHz * in.dt;
        if (as.driftPhase >= 1.0) as.driftPhase -= 1.0;
    }

    if (src.smoothing.enabled && src.smoothing.pitchEnabled)
    {
        SetSmoothedTarget(as.pitchSmoothing, pitchMul);
        pitchMul = StepSmoothedParam(as.pitchSmoothing);
    }
    if (src.arpeggio.enabled && src.arpeggio.steps > 0)
    {
        as.arpElapsedSec += in.dt;
        const int steps = std::clamp(src.arpeggio.steps, 1, 8);
        const double stepDur = 1.0 / (std::max)(0.5, src.arpeggio.rateHz);
        while (as.arpElapsedSec >= stepDur)
        {
            as.arpElapsedSec -= stepDur;
            as.arpStep = (as.arpStep + 1) % steps;
        }
        const int semitoneOffset = src.arpeggio.semitones[static_cast<size_t>(as.arpStep)];
        pitchMul *= std::pow(2.0, static_cast<double>(semitoneOffset) / 12.0);
    }

    const double phaseInc = voices.phaseInc[i] * in.pitchFactor * pitchMul;
    const int unisonVoices = std::clamp(src.unisonVoices, 1, 8);
    const double detuneCents = std::clamp(src.unisonDetuneCents, 0.0, 120.0);
    const double spread = std::clamp(src.unisonSpread, 0.0, 1.0);
    const double subOscLevel = std::clamp(src.subOscLevel, 0.0, 2.0);
    const double effectivePW = std::clamp(src.pulseWidth + mod.pulseWidthAdd, 0.05, 0.95);

    double unisonSum = 0.0;
    for (int uv = 0; uv < unisonVoices; uv++)
    {
        const double pos = (unisonVoices <= 1) ? 0.0 : (static_cast<double>(uv) / (unisonVoices - 1));
        const double centered = (pos * 2.0) - 1.0;
        const double cents = centered * detuneCents;
        const double ratio = std::pow(2.0, cents / 1200.0);
        const double phaseOffset = centered * spread * 0.08;
        const double uvPhase = WrapPhase(voices.phase[i] * ratio + phaseOffset);
        const double uvInc = phaseInc * ratio;
        unisonSum += SampleWavePhase(src.wave, uvPhase, uvInc, effectivePW);
    }

    double mainWave = unisonSum / unisonVoices;
    if (subOscLevel > 0.0)
    {
        const double subPhase = WrapPhase(voices.phase[i] * 0.5);
        const double subWave = SampleWavePhase(src.wave, subPhase, phaseInc * 0.5, effectivePW);
        mainWave = (mainWave + (subOscLevel * subWave)) / (1.0 + subOscLevel);
    }
    if (src.ringModEnabled && src.ringModMix > 0.0)
    {
        const double ringInc = phaseInc * std::clamp(src.ringModRatio, 0.125, 16.0);
        as.ringPhase = WrapPhase(as.ringPhase + ringInc);
        const double ringSignal = std::sin(2.0 * kPi * as.ringPhase);
        const double mix = std::clamp(src.ringModMix, 0.0, 1.0);
        mainWave *= (1.0 - mix) + mix * ringSignal;
    }

    frame.sample = mainWave;
    frame.ampMul = mod.ampMul;
    frame.shaperKind = CommonShaperKind::BiquadFilter;
    double effectiveCutoff = src.filterCutoffHz * mod.filterCutoffMul;
    if (src.filterKeytrack != 0.0)
    {
        const double keytrackRatio = std::pow(2.0, src.filterKeytrack * (voices.noteNumber[i] - 60) / 12.0);
        effectiveCutoff *= keytrackRatio;
    }
    frame.shaperCutoffHz = effectiveCutoff;
    frame.shaperResonanceMul = mod.resonanceMul;
    frame.shaperDrive = src.drive;

    if (src.hardSyncEnabled)
    {
        const double syncInc = phaseInc * std::clamp(src.hardSyncRatio, 0.5, 8.0);
        const double prevSync = as.syncPhase;
        as.syncPhase = WrapPhase(as.syncPhase + syncInc);
        if (as.syncPhase < prevSync)
        {
            voices.phase[i] = 0.0;
        }
        else
        {
            voices.phase[i] = WrapPhase(voices.phase[i] + phaseInc);
        }
    }
    else
    {
        voices.phase[i] = WrapPhase(voices.phase[i] + phaseInc);
    }
}

void RenderNoiseSource(const NoiseConfig& src, SourceRenderFrame& frame)
{
    frame.sample = SampleNoise(src.noise);
    frame.shaperKind = CommonShaperKind::BiquadFilter;
    frame.shaperCutoffHz = src.filterCutoffHz;
}

void RenderFmSource(
    const FmConfig& src,
    Voice& voices,
    size_t i,
    const VoiceRenderInput& in,
    SourceRenderFrame& frame)
{
    auto& fs = std::get<FmVoiceState>(voices.sourceState[i]);
    const ModulationResult mod = EvaluateModulation(
        fs.modulation,
        src.modulation,
        in.dt,
        ModulationInput{ in.velGain, in.modwheel, in.channelPressure, in.polyPressure });
    const double indexScale = mod.fmIndexMul;
    const double feedbackSample = fs.op0FeedbackSample * src.feedback;
    const double outLevelScale = mod.ampMul;
    const double op0 = SampleOp(src.ops[0].wave, fs.opPhase[0], feedbackSample, src.ops[0].index * indexScale);

    switch (src.algorithm)
    {
    case 1:
    {
        const double mod0 = op0 * src.ops[0].level;
        const double out0 = SampleOp(src.ops[1].wave, fs.opPhase[1], mod0, src.ops[1].index * indexScale) * src.ops[1].level;
        const double mod2 = SampleOp(src.ops[2].wave, fs.opPhase[2], 0.0, src.ops[2].index * indexScale) * src.ops[2].level;
        const double out2 = SampleOp(src.ops[3].wave, fs.opPhase[3], mod2, src.ops[3].index * indexScale) * src.ops[3].level;
        frame.sample = (out0 + out2) * 0.5 * outLevelScale;
        break;
    }
    case 2:
    {
        const double mod0 = op0 * src.ops[0].level;
        const double out1 = SampleOp(src.ops[1].wave, fs.opPhase[1], mod0, src.ops[1].index * indexScale) * src.ops[1].level;
        const double out2 = SampleOp(src.ops[2].wave, fs.opPhase[2], mod0, src.ops[2].index * indexScale) * src.ops[2].level;
        const double out3 = SampleOp(src.ops[3].wave, fs.opPhase[3], mod0, src.ops[3].index * indexScale) * src.ops[3].level;
        frame.sample = (out1 + out2 + out3) / 3.0 * outLevelScale;
        break;
    }
    case 3:
    {
        const double mod0 = op0 * src.ops[0].level;
        const double mod1 = SampleOp(src.ops[1].wave, fs.opPhase[1], mod0, src.ops[1].index * indexScale) * src.ops[1].level;
        const double mod2 = SampleOp(src.ops[2].wave, fs.opPhase[2], mod1, src.ops[2].index * indexScale) * src.ops[2].level;
        const double out = SampleOp(src.ops[3].wave, fs.opPhase[3], mod2, src.ops[3].index * indexScale) * src.ops[3].level;
        frame.sample = out * outLevelScale;
        break;
    }
    case 4:
    {
        const double mod0 = op0 * src.ops[0].level;
        const double out1 = SampleOp(src.ops[1].wave, fs.opPhase[1], mod0, src.ops[1].index * indexScale) * src.ops[1].level;
        const double mod2 = SampleOp(src.ops[2].wave, fs.opPhase[2], 0.0, src.ops[2].index * indexScale) * src.ops[2].level;
        const double out3 = SampleOp(src.ops[3].wave, fs.opPhase[3], mod2, src.ops[3].index * indexScale) * src.ops[3].level;
        frame.sample = (out1 + out3) * 0.5 * outLevelScale;
        break;
    }
    case 5:
    {
        const double mod0 = op0 * src.ops[0].level;
        const double out1 = SampleOp(src.ops[1].wave, fs.opPhase[1], mod0, src.ops[1].index * indexScale) * src.ops[1].level;
        const double out2 = SampleOp(src.ops[2].wave, fs.opPhase[2], mod0, src.ops[2].index * indexScale) * src.ops[2].level;
        const double out3 = SampleOp(src.ops[3].wave, fs.opPhase[3], mod0, src.ops[3].index * indexScale) * src.ops[3].level;
        frame.sample = (out1 + out2 + out3) / 3.0 * outLevelScale;
        break;
    }
    case 6:
    {
        const double mod0 = op0 * src.ops[0].level;
        const double out1 = SampleOp(src.ops[1].wave, fs.opPhase[1], mod0, src.ops[1].index * indexScale) * src.ops[1].level;
        const double out2 = SampleOp(src.ops[2].wave, fs.opPhase[2], 0.0, src.ops[2].index * indexScale) * src.ops[2].level;
        const double out3 = SampleOp(src.ops[3].wave, fs.opPhase[3], 0.0, src.ops[3].index * indexScale) * src.ops[3].level;
        frame.sample = (out1 + out2 + out3) / 3.0 * outLevelScale;
        break;
    }
    case 7:
    {
        const double out0 = op0 * src.ops[0].level;
        const double out1 = SampleOp(src.ops[1].wave, fs.opPhase[1], 0.0, src.ops[1].index * indexScale) * src.ops[1].level;
        const double out2 = SampleOp(src.ops[2].wave, fs.opPhase[2], 0.0, src.ops[2].index * indexScale) * src.ops[2].level;
        const double out3 = SampleOp(src.ops[3].wave, fs.opPhase[3], 0.0, src.ops[3].index * indexScale) * src.ops[3].level;
        frame.sample = (out0 + out1 + out2 + out3) * 0.25 * outLevelScale;
        break;
    }
    case 0:
    default:
    {
        const double mod0 = op0 * src.ops[0].level;
        const double out = SampleOp(src.ops[1].wave, fs.opPhase[1], mod0, src.ops[1].index * indexScale) * src.ops[1].level;
        frame.sample = out * outLevelScale;
        break;
    }
    }

    fs.op0FeedbackSample = op0;
    frame.shaperKind = CommonShaperKind::BiquadFilter;
    frame.shaperCutoffHz = src.filterCutoffHz;
    frame.shaperResonanceMul = mod.resonanceMul;
    frame.shaperDrive = src.drive;

    for (int k = 0; k < 4; k++)
    {
        fs.opPhase[k] += voices.phaseInc[i] * in.pitchFactor * mod.pitchMul * src.ops[k].ratio;
        if (fs.opPhase[k] >= 1.0)
        {
            fs.opPhase[k] -= 1.0;
        }
    }
}

void RenderDrumSource(
    const DrumConfig& src,
    Voice& voices,
    size_t i,
    const VoiceRenderInput& in,
    int sampleRate,
    SourceRenderFrame& frame)
{
    const double drumGain = (src.gain > 0.0) ? src.gain : 1.0;
    frame.sample = RenderDrumSample(src, voices, i, in.dt, sampleRate);
    frame.sourceGain = drumGain;
}

void RenderPsgSource(
    const PsgConfig& src,
    Voice& voices,
    size_t i,
    const VoiceRenderInput& in,
    SourceRenderFrame& frame)
{
    auto& ps = std::get<PsgVoiceState>(voices.sourceState[i]);

    const double phaseInc = voices.phaseInc[i] * in.pitchFactor;
    ps.phase += phaseInc;
    if (ps.phase >= 1.0) ps.phase -= 1.0;

    double s = 0.0;
    switch (src.wave)
    {
    case PsgWaveType::Square:
        s = (ps.phase < 0.5) ? 1.0 : -1.0;
        break;
    case PsgWaveType::Pulse:
    {
        const double threshold = (src.duty + 1) / 8.0;
        s = (ps.phase < threshold) ? 1.0 : -1.0;
        break;
    }
    case PsgWaveType::Triangle:
        if (ps.phase < 0.25)      s = ps.phase * 4.0;
        else if (ps.phase < 0.75) s = 2.0 - ps.phase * 4.0;
        else                      s = ps.phase * 4.0 - 4.0;
        break;
    case PsgWaveType::Noise:
    {
        const uint16_t bit = ((ps.lfsrState >> 0) ^ (ps.lfsrState >> 1)) & 1u;
        ps.lfsrState = static_cast<uint16_t>((ps.lfsrState >> 1) | (bit << 15));
        s = (ps.lfsrState & 1u) ? 1.0 : -1.0;
        break;
    }
    }

    if (src.volumeSteps < 15)
    {
        const int steps = src.volumeSteps;
        s = (steps <= 0) ? 0.0 : std::round(s * steps) / steps;
    }

    frame.sample = s;
    frame.sourceGain = 1.0;
    frame.shaperKind = CommonShaperKind::None;
}

void RenderSourceFrame(
    const SourceConfig& src,
    Voice& voices,
    size_t i,
    const VoiceRenderInput& in,
    int sampleRate,
    SourceRenderFrame& frame)
{
    std::visit([&](const auto& source)
    {
        using T = std::decay_t<decltype(source)>;
        if constexpr (std::is_same_v<T, WaveformConfig>)
        {
            RenderWaveformSource(source, voices, i, in, frame);
        }
        else if constexpr (std::is_same_v<T, AnalogConfig>)
        {
            RenderAnalogSource(source, voices, i, in, frame);
        }
        else if constexpr (std::is_same_v<T, NoiseConfig>)
        {
            RenderNoiseSource(source, frame);
        }
        else if constexpr (std::is_same_v<T, FmConfig>)
        {
            RenderFmSource(source, voices, i, in, frame);
        }
        else if constexpr (std::is_same_v<T, DrumConfig>)
        {
            RenderDrumSource(source, voices, i, in, sampleRate, frame);
        }
        else if constexpr (std::is_same_v<T, PsgConfig>)
        {
            RenderPsgSource(source, voices, i, in, frame);
        }
    }, src);
}

void ApplyCommonShaper(
    const SourceConfig& src,
    Voice& voices,
    size_t i,
    const VoiceRenderInput& in,
    SourceRenderFrame& frame)
{
    if (frame.shaperKind == CommonShaperKind::BiquadFilter)
    {
        // filter を持つ方式のみ処理する（NoteOffVoiceModulation と同パターン）。
        // filterCutoffSmoothing を持つ方式（現行: waveform）のみ smoothing を適用する。
        std::visit([&](auto& st)
        {
            if constexpr (requires { st.filter; })
            {
                double filterCutoffHz = frame.shaperCutoffHz;
                if constexpr (requires { st.filterCutoffSmoothing; })
                {
                    const auto* waveformSrc = std::get_if<WaveformConfig>(&src);
                    const auto* analogSrc = std::get_if<AnalogConfig>(&src);
                    const bool smoothingEnabled =
                        (waveformSrc && waveformSrc->smoothing.enabled) ||
                        (analogSrc && analogSrc->smoothing.enabled);
                    if (smoothingEnabled)
                    {
                        SetSmoothedTarget(st.filterCutoffSmoothing, filterCutoffHz);
                        filterCutoffHz = StepSmoothedParam(st.filterCutoffSmoothing);
                    }
                }
                filterCutoffHz *= CutoffScaleFromBrightness(in.brightness);
                SetFilterCutoffHz(st.filter, filterCutoffHz);
                const double baseResonance = SourceFilterResonance(src);
                SetFilterResonance(st.filter, baseResonance * ResonanceScaleFromCc(in.resonance) * frame.shaperResonanceMul);
                frame.sample = ProcessFilterSample(st.filter, frame.sample);
            }
        }, voices.sourceState[i]);
    }

    if (frame.shaperDrive > 0.0)
    {
        // tanh ソフトクリップ: tanh(k*x) / tanh(k), k = drive * 20.0
        const double k = frame.shaperDrive * 20.0;
        const double tanhK = std::tanh(k);
        if (tanhK > 1e-9)
        {
            frame.sample = std::tanh(k * frame.sample) / tanhK;
        }
    }
}

void ApplyModulationLayer(
    const SourceConfig& src,
    Voice& voices,
    size_t i,
    SourceRenderFrame& frame)
{
    std::visit([&](const auto& source)
    {
        using T = std::decay_t<decltype(source)>;
        if constexpr (std::is_same_v<T, WaveformConfig>)
        {
            auto& ws = std::get<WaveformVoiceState>(voices.sourceState[i]);
            double ampMul = frame.ampMul;
            if (source.smoothing.enabled)
            {
                SetSmoothedTarget(ws.ampSmoothing, ampMul);
                ampMul = StepSmoothedParam(ws.ampSmoothing);
            }
            frame.ampMul = ampMul;
        }
        else if constexpr (std::is_same_v<T, AnalogConfig>)
        {
            auto& as = std::get<AnalogVoiceState>(voices.sourceState[i]);
            double ampMul = frame.ampMul;
            if (source.smoothing.enabled)
            {
                SetSmoothedTarget(as.ampSmoothing, ampMul);
                ampMul = StepSmoothedParam(as.ampSmoothing);
            }
            frame.ampMul = ampMul;
        }
    }, src);
}
} // namespace

StereoFrame RenderVoices(RenderState& state, const SoundData& sound)
{
    // 前提: audio thread のサンプルループから1サンプル単位で呼ぶ。
    StereoFrame sum{};
    auto& voices = state.voices;
    const double dt = 1.0 / sound.fs;

    // SoA 配列を先頭から順に処理するホットパス。
    // 目的: キャッシュ局所性を高め、Voice 数増加時の劣化を抑える。
    for (size_t i = 0; i < voices.size(); i++)
    {
        if (voices.env[i].stage == ADSRStage::Off)
        {
            continue;
        }

        const double envGain = StepADSR(
            voices.env[i],
            dt,
            voices.attackSec[i] * TimeScaleFromOffset(state.channelAdsrOffset[voices.channelIndex[i]].attack),
            voices.decaySec[i] * TimeScaleFromOffset(state.channelAdsrOffset[voices.channelIndex[i]].decay),
            std::clamp(
                voices.sustainLevel[i] + (state.channelAdsrOffset[voices.channelIndex[i]].sustain * 0.5),
                0.0,
                1.0),
            voices.releaseSec[i] * TimeScaleFromOffset(state.channelAdsrOffset[voices.channelIndex[i]].release));
        if (voices.pendingRemove[i] == 0 && voices.env[i].stage == ADSRStage::Off)
        {
            // 即時 erase は O(n) 連鎖になるため、削除フラグだけ立てて後段でまとめて圧縮する。
            voices.pendingRemove[i] = 1;
            state.pendingRemoveCount++;
            continue;
        }

        VoiceRenderInput in{};
        in.dt = dt;
        in.velGain = VelocityToGain(voices.velocity[i]);
        in.envGain = envGain;
        const int ch = voices.channelIndex[i];
        // mute/solo/mixGain 判定は事前計算済みフラグを参照する。
        // 目的: ホットループの分岐段数を減らし、分岐予測ミスを抑える。
        // 前提: channel mix 状態は RenderMIDIEvents 実行中に変化しない。
        // トレードオフ: 判定ロジックが初期化側へ移動し、追跡箇所が分かれる。
        if (!state.channelRenderable[ch])
        {
            continue;
        }
        in.mixGainL = state.channelMixGainL[ch];
        in.mixGainR = state.channelMixGainR[ch];
        in.pitchFactor = state.channelPitch[ch];
        in.ccGain = state.channelCc7[ch] * state.channelCc11[ch];
        in.modwheel = state.channelModwheel[ch];
        in.channelPressure = state.channelPressure[ch];
        const int note = std::clamp(voices.noteNumber[i], 0, 127);
        in.polyPressure = state.channelPolyPressure[ch][note];
        in.brightness = state.channelBrightness[ch];
        in.resonance = state.channelResonance[ch];

        // ポルタメント: 現在ピッチをターゲットへ指数平滑しつつ pitchFactor へ反映する。
        const double effectivePortamentoTimeSec = state.channelPortamentoOn[ch]
            ? (std::max)(voices.portamentoTimeSec[i], state.channelPortamentoTimeSec[ch])
            : 0.0;
        if (effectivePortamentoTimeSec > 0.0 &&
            std::abs(voices.portamentoPitchHz[i] - voices.portamentoTargetHz[i]) > 0.01)
        {
            const double tau = effectivePortamentoTimeSec;
            voices.portamentoPitchHz[i] +=
                (voices.portamentoTargetHz[i] - voices.portamentoPitchHz[i]) *
                (1.0 - std::exp(-in.dt / tau));
        }
        // portamentoPitchHz を phaseInc に対する倍率として pitchFactor へ乗算する。
        // (phaseInc = targetHz / sampleRate なので比率で戻す)
        if (effectivePortamentoTimeSec > 0.0 && voices.portamentoTargetHz[i] > 0.0)
        {
            in.pitchFactor *= voices.portamentoPitchHz[i] / voices.portamentoTargetHz[i];
        }

        SourceRenderFrame frame{};
        RenderSourceFrame(voices.source[i], voices, i, in, sound.fs, frame);
        ApplyCommonShaper(voices.source[i], voices, i, in, frame);
        ApplyModulationLayer(voices.source[i], voices, i, frame);

        const double mono = 
            frame.sourceGain *
            voices.amp[i] *
            in.ccGain *
            in.velGain *
            frame.sample *
            in.envGain *
            frame.ampMul;
        sum.left += in.mixGainL * mono;
        sum.right += in.mixGainR * mono;
    }

    return sum;
}
