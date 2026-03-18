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
    double mixGain = 1.0;
    double pitchFactor = 1.0;
    double ccGain = 1.0;
    double velGain = 1.0;
    double envGain = 1.0;
};

double WrapPhase(double phase)
{
    phase -= std::floor(phase);
    if (phase < 0.0)
    {
        phase += 1.0;
    }
    return phase;
}

void EnsureDrumFilters(Voice& voices, size_t i, double hpCut, double lpCut, int sampleRate)
{
    // 初回のみ係数を計算し、同一voice中は再利用する。
    if (voices.drumHpAlpha[i] <= 0.0)
    {
        voices.drumHpAlpha[i] = std::exp(-2.0 * kPi * hpCut / sampleRate);
    }
    if (voices.drumLpAlpha[i] <= 0.0)
    {
        voices.drumLpAlpha[i] = std::exp(-2.0 * kPi * lpCut / sampleRate);
    }
}

void PrepareDrumRelease(Voice& voices, size_t i)
{
    // Drum は attack+decay 到達で自動 NoteOff へ移す。
    if (voices.released[i] == 0 && voices.drumTime[i] >= (voices.attackSec[i] + voices.decaySec[i]))
    {
        NoteOff(voices.env[i]);
        voices.released[i] = 1;
    }
}

double RenderKickSample(const DrumConfig& src, Voice& voices, size_t i, int sampleRate)
{
    double pitchFactor = 1.0;
    if (voices.drumPitchDecaySec[i] > 0.0)
    {
        pitchFactor += (voices.drumPitchDrop[i] - 1.0) * std::exp(-voices.drumTime[i] / voices.drumPitchDecaySec[i]);
    }
    const double freq = voices.drumBaseFreq[i] * pitchFactor;
    voices.phase[i] += (freq / sampleRate);
    if (voices.phase[i] >= 1.0) voices.phase[i] -= 1.0;
    return SampleWavePhase(WaveType::Sine, voices.phase[i]);
}

double RenderSnareSample(const DrumConfig& src, Voice& voices, size_t i, int sampleRate)
{
    const double toneLevel = (src.toneLevel > 0.0) ? src.toneLevel : 0.3;
    const double noiseLevel = (src.noiseLevel > 0.0) ? src.noiseLevel : 0.7;
    const double hpCut = (src.hpCut > 0.0) ? src.hpCut : 1200.0;
    const double lpCut = (src.lpCut > 0.0) ? src.lpCut : 6000.0;
    const WaveType toneWave = static_cast<WaveType>(src.toneWave);
    const NoiseType noiseType = static_cast<NoiseType>(src.noiseType);
    EnsureDrumFilters(voices, i, hpCut, lpCut, sampleRate);
    voices.phase[i] += voices.drumBaseFreq[i] / sampleRate;
    if (voices.phase[i] >= 1.0) voices.phase[i] -= 1.0;
    const double toneInc = voices.drumBaseFreq[i] / sampleRate;
    const double tone = SampleWavePhase(toneWave, voices.phase[i], toneInc);
    const double noise = SampleNoise(noiseType);
    const double hp = voices.drumHpAlpha[i] * (voices.drumHpPrev[i] + noise - voices.drumNoisePrev[i]);
    const double lp = (1.0 - voices.drumLpAlpha[i]) * hp + voices.drumLpAlpha[i] * voices.drumLpPrev[i];
    voices.drumNoisePrev[i] = noise;
    voices.drumHpPrev[i] = hp;
    voices.drumLpPrev[i] = lp;
    return toneLevel * tone + noiseLevel * lp;
}

double RenderHatSample(const DrumConfig& src, Voice& voices, size_t i, int sampleRate)
{
    const double noiseLevel = (src.noiseLevel > 0.0) ? src.noiseLevel : 1.0;
    const double hpCut = (src.hpCut > 0.0) ? src.hpCut : 6000.0;
    const double lpCut = (src.lpCut > 0.0) ? src.lpCut : 12000.0;
    const double toneFreq = (src.toneFreq > 0.0) ? src.toneFreq : 8000.0;
    const double toneLevel = (src.toneLevel > 0.0) ? src.toneLevel : 0.2;
    const WaveType toneWave = static_cast<WaveType>(src.toneWave);
    const NoiseType noiseType = static_cast<NoiseType>(src.noiseType);
    EnsureDrumFilters(voices, i, hpCut, lpCut, sampleRate);
    const double noise = SampleNoise(noiseType);
    const double hp = voices.drumHpAlpha[i] * (voices.drumHpPrev[i] + noise - voices.drumNoisePrev[i]);
    const double lp = (1.0 - voices.drumLpAlpha[i]) * hp + voices.drumLpAlpha[i] * voices.drumLpPrev[i];
    voices.drumNoisePrev[i] = noise;
    voices.drumHpPrev[i] = hp;
    voices.drumLpPrev[i] = lp;
    voices.phase[i] += toneFreq / sampleRate;
    if (voices.phase[i] >= 1.0) voices.phase[i] -= 1.0;
    const double toneInc = toneFreq / sampleRate;
    const double tone = SampleWavePhase(toneWave, voices.phase[i], toneInc);
    return noiseLevel * lp + toneLevel * tone;
}

double RenderDrumSample(const DrumConfig& src, Voice& voices, size_t i, double dt, int sampleRate)
{
    PrepareDrumRelease(voices, i);

    double w = 0.0;
    if (src.type == DrumType::Kick)
    {
        w = RenderKickSample(src, voices, i, sampleRate);
    }
    else if (src.type == DrumType::Snare)
    {
        w = RenderSnareSample(src, voices, i, sampleRate);
    }
    else if (src.type == DrumType::Hat)
    {
        w = RenderHatSample(src, voices, i, sampleRate);
    }

    voices.drumTime[i] += dt;
    return w;
}

void RenderWaveformSource(
    const WaveformConfig& src,
    Voice& voices,
    size_t i,
    const VoiceRenderInput& in,
    SourceRenderFrame& frame)
{
    ModulationResult mod = EvaluateModulation(
        voices.modulation[i],
        src.modulation,
        in.dt);
    double pitchMul = mod.pitchMul;
    if (src.smoothing.enabled && src.smoothing.pitchEnabled)
    {
        SetSmoothedTarget(voices.waveformPitchSmoothing[i], pitchMul);
        pitchMul = StepSmoothedParam(voices.waveformPitchSmoothing[i]);
    }

    const double phaseInc = voices.phaseInc[i] * in.pitchFactor * pitchMul;
    const int unisonVoices = std::clamp(src.unisonVoices, 1, 8);
    const double detuneCents = std::clamp(src.unisonDetuneCents, 0.0, 120.0);
    const double spread = std::clamp(src.unisonSpread, 0.0, 1.0);
    const double subOscLevel = std::clamp(src.subOscLevel, 0.0, 2.0);

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
        unisonSum += SampleWavePhase(src.wave, uvPhase, uvInc);
    }

    double mainWave = unisonSum / unisonVoices;
    if (subOscLevel > 0.0)
    {
        const double subPhase = WrapPhase(voices.phase[i] * 0.5);
        const double subWave = SampleWavePhase(src.wave, subPhase, phaseInc * 0.5);
        mainWave = (mainWave + (subOscLevel * subWave)) / (1.0 + subOscLevel);
    }

    frame.sample = mainWave;
    frame.ampMul = mod.ampMul;
    frame.shaperKind = CommonShaperKind::WaveformFilter;
    frame.shaperCutoffHz = src.filterCutoffHz * mod.filterCutoffMul;

    voices.phase[i] += phaseInc;
    if (voices.phase[i] >= 1.0) voices.phase[i] -= 1.0;
}

void RenderNoiseSource(const NoiseConfig& src, SourceRenderFrame& frame)
{
    frame.sample = SampleNoise(src.noise);
}

void RenderFmSource(
    const FmConfig& src,
    Voice& voices,
    size_t i,
    const VoiceRenderInput& in,
    SourceRenderFrame& frame)
{
    const ModulationResult mod = EvaluateModulation(
        voices.modulation[i],
        src.modulation,
        in.dt);
    const double carrierInc = voices.phaseInc[i] * in.pitchFactor * mod.pitchMul * src.carrierRatio;
    const double modInc = voices.phaseInc[i] * in.pitchFactor * mod.pitchMul * src.modRatio;
    const double fmIndex = src.index * mod.fmIndexMul;
    frame.sample = SampleFmPhase(
        src.carrierWave,
        src.modWave,
        voices.fmCarrierPhase[i],
        voices.fmModPhase[i],
        carrierInc,
        modInc,
        fmIndex);
    frame.ampMul = mod.ampMul;
    frame.sourceGain = src.outLevel;

    voices.fmCarrierPhase[i] += carrierInc;
    if (voices.fmCarrierPhase[i] >= 1.0) voices.fmCarrierPhase[i] -= 1.0;
    voices.fmModPhase[i] += modInc;
    if (voices.fmModPhase[i] >= 1.0) voices.fmModPhase[i] -= 1.0;
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
    }, src);
}

void ApplyCommonShaper(
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
            if (frame.shaperKind != CommonShaperKind::WaveformFilter)
            {
                return;
            }

            double filterCutoffHz = frame.shaperCutoffHz;
            if (source.smoothing.enabled)
            {
                SetSmoothedTarget(voices.waveformFilterCutoffSmoothing[i], filterCutoffHz);
                filterCutoffHz = StepSmoothedParam(voices.waveformFilterCutoffSmoothing[i]);
            }
            SetFilterCutoffHz(voices.waveformFilter[i], filterCutoffHz);
            frame.sample = ProcessFilterSample(voices.waveformFilter[i], frame.sample);
        }
    }, src);
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
            double ampMul = frame.ampMul;
            if (source.smoothing.enabled)
            {
                SetSmoothedTarget(voices.waveformAmpSmoothing[i], ampMul);
                ampMul = StepSmoothedParam(voices.waveformAmpSmoothing[i]);
            }
            frame.ampMul = ampMul;
        }
    }, src);
}
} // namespace

double RenderVoices(RenderState& state, const SoundData& sound)
{
    // 前提: audio thread のサンプルループから1サンプル単位で呼ぶ。
    double sum = 0.0;
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
            voices.env[i], dt, voices.attackSec[i], voices.decaySec[i], voices.sustainLevel[i], voices.releaseSec[i]);
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
        in.mixGain = state.channelMixGain[ch];
        in.pitchFactor = state.channelPitch[ch];
        in.ccGain = state.channelCc7[ch] * state.channelCc11[ch];

        SourceRenderFrame frame{};
        RenderSourceFrame(voices.source[i], voices, i, in, sound.fs, frame);
        ApplyCommonShaper(voices.source[i], voices, i, frame);
        ApplyModulationLayer(voices.source[i], voices, i, frame);

        sum += in.mixGain *
            frame.sourceGain *
            voices.amp[i] *
            in.ccGain *
            in.velGain *
            frame.sample *
            in.envGain *
            frame.ampMul;
    }

    return sum;
}
