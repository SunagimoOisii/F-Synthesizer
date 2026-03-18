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
        in.dt);
    double pitchMul = mod.pitchMul;
    if (src.smoothing.enabled && src.smoothing.pitchEnabled)
    {
        SetSmoothedTarget(ws.pitchSmoothing, pitchMul);
        pitchMul = StepSmoothedParam(ws.pitchSmoothing);
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
    frame.shaperKind = CommonShaperKind::BiquadFilter;
    double effectiveCutoff = src.filterCutoffHz * mod.filterCutoffMul;
    if (src.filterKeytrack != 0.0)
    {
        const double keytrackRatio = std::pow(2.0, src.filterKeytrack * (voices.noteNumber[i] - 60) / 12.0);
        effectiveCutoff *= keytrackRatio;
    }
    frame.shaperCutoffHz = effectiveCutoff;

    voices.phase[i] += phaseInc;
    if (voices.phase[i] >= 1.0) voices.phase[i] -= 1.0;
}

void RenderNoiseSource(const NoiseConfig& src, SourceRenderFrame& frame)
{
    frame.sample = SampleNoise(src.noise);
}

void RenderSubtractiveSource(
    const SubtractiveConfig& src,
    Voice& voices,
    size_t i,
    const VoiceRenderInput& in,
    SourceRenderFrame& frame)
{
    auto& ss = std::get<SubtractiveVoiceState>(voices.sourceState[i]);
    const ModulationResult mod = EvaluateModulation(ss.modulation, src.modulation, in.dt);

    const double phaseInc = voices.phaseInc[i] * in.pitchFactor * mod.pitchMul;
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

    // filterKeytrack: 基準ノート C4(60) からの半音差に比例してカットオフを動かす。
    double effectiveCutoff = src.filterCutoffHz * mod.filterCutoffMul;
    if (src.filterKeytrack != 0.0)
    {
        const double keytrackRatio = std::pow(2.0, src.filterKeytrack * (voices.noteNumber[i] - 60) / 12.0);
        effectiveCutoff *= keytrackRatio;
    }

    frame.sample = mainWave;
    frame.ampMul = mod.ampMul;
    frame.shaperKind = CommonShaperKind::BiquadFilter;
    frame.shaperCutoffHz = effectiveCutoff;

    voices.phase[i] += phaseInc;
    if (voices.phase[i] >= 1.0) voices.phase[i] -= 1.0;
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
        in.dt);
    const double carrierInc = voices.phaseInc[i] * in.pitchFactor * mod.pitchMul * src.carrierRatio;
    const double modInc = voices.phaseInc[i] * in.pitchFactor * mod.pitchMul * src.modRatio;
    const double fmIndex = src.index * mod.fmIndexMul;
    frame.sample = SampleFmPhase(
        src.carrierWave,
        src.modWave,
        fs.carrierPhase,
        fs.modPhase,
        carrierInc,
        modInc,
        fmIndex);
    frame.ampMul = mod.ampMul;
    frame.sourceGain = src.outLevel;
    frame.shaperKind = CommonShaperKind::BiquadFilter;
    frame.shaperCutoffHz = src.filterCutoffHz;

    fs.carrierPhase += carrierInc;
    if (fs.carrierPhase >= 1.0) fs.carrierPhase -= 1.0;
    fs.modPhase += modInc;
    if (fs.modPhase >= 1.0) fs.modPhase -= 1.0;
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
        else if constexpr (std::is_same_v<T, SubtractiveConfig>)
        {
            RenderSubtractiveSource(source, voices, i, in, frame);
        }
    }, src);
}

void ApplyCommonShaper(
    const SourceConfig& src,
    Voice& voices,
    size_t i,
    SourceRenderFrame& frame)
{
    if (frame.shaperKind != CommonShaperKind::BiquadFilter)
    {
        return;
    }

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
                if (waveformSrc && waveformSrc->smoothing.enabled)
                {
                    SetSmoothedTarget(st.filterCutoffSmoothing, filterCutoffHz);
                    filterCutoffHz = StepSmoothedParam(st.filterCutoffSmoothing);
                }
            }
            SetFilterCutoffHz(st.filter, filterCutoffHz);
            frame.sample = ProcessFilterSample(st.filter, frame.sample);
        }
    }, voices.sourceState[i]);
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
