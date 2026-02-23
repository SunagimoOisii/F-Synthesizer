#include "Internal.h"

#include <algorithm>
#include <cmath>
#include <type_traits>

#include "Oscillator.h"

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

void EnsureDrumFilters(VoicesSoA& voices, size_t i, double hpCut, double lpCut, int sampleRate)
{
    if (voices.drumHpAlpha[i] <= 0.0)
    {
        voices.drumHpAlpha[i] = std::exp(-2.0 * kPi * hpCut / sampleRate);
    }
    if (voices.drumLpAlpha[i] <= 0.0)
    {
        voices.drumLpAlpha[i] = std::exp(-2.0 * kPi * lpCut / sampleRate);
    }
}

void PrepareDrumRelease(VoicesSoA& voices, size_t i)
{
    if (voices.released[i] == 0 && voices.drumTime[i] >= (voices.attackSec[i] + voices.decaySec[i]))
    {
        NoteOff(voices.env[i]);
        voices.released[i] = 1;
    }
}

double RenderKickSample(const DrumConfig& src, VoicesSoA& voices, size_t i, int sampleRate)
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

double RenderSnareSample(const DrumConfig& src, VoicesSoA& voices, size_t i, int sampleRate)
{
    const double toneLevel = (src.toneLevel > 0.0) ? src.toneLevel : 0.3;
    const double noiseLevel = (src.noiseLevel > 0.0) ? src.noiseLevel : 0.7;
    const double hpCut = (src.hpCut > 0.0) ? src.hpCut : 1200.0;
    const double lpCut = (src.lpCut > 0.0) ? src.lpCut : 6000.0;
    const WaveType toneWave = (src.toneWave >= 0) ? static_cast<WaveType>(src.toneWave) : WaveType::Sine;
    const NoiseType noiseType = (src.noiseType >= 0) ? static_cast<NoiseType>(src.noiseType) : NoiseType::White;
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

double RenderHatSample(const DrumConfig& src, VoicesSoA& voices, size_t i, int sampleRate)
{
    const double noiseLevel = (src.noiseLevel > 0.0) ? src.noiseLevel : 1.0;
    const double hpCut = (src.hpCut > 0.0) ? src.hpCut : 6000.0;
    const double lpCut = (src.lpCut > 0.0) ? src.lpCut : 12000.0;
    const double toneFreq = (src.toneFreq > 0.0) ? src.toneFreq : 8000.0;
    const double toneLevel = (src.toneLevel > 0.0) ? src.toneLevel : 0.2;
    const WaveType toneWave = (src.toneWave >= 0) ? static_cast<WaveType>(src.toneWave) : WaveType::Square;
    const NoiseType noiseType = (src.noiseType >= 0) ? static_cast<NoiseType>(src.noiseType) : NoiseType::White;
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

double RenderDrumSample(const DrumConfig& src, VoicesSoA& voices, size_t i, double dt, int sampleRate)
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
} // namespace

double RenderVoices(RenderState& state, const SoundData& sound)
{
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

        const double velGain = VelocityToGain(voices.velocity[i]);
        const int ch = voices.channelIndex[i];
        // mute/solo/mixGain 判定は事前計算済みフラグを参照する。
        // 目的: ホットループの分岐段数を減らし、分岐予測ミスを抑える。
        // 前提: channel mix 状態は RenderMIDIEvents 実行中に変化しない。
        // トレードオフ: 判定ロジックが初期化側へ移動し、追跡箇所が分かれる。
        if (!state.channelRenderable[ch])
        {
            continue;
        }
        const double mixGain = state.channelMixGain[ch];
        const double pitchFactor = state.channelPitch[ch];
        const double ccGain = state.channelCc7[ch] * state.channelCc11[ch];

        double w = 0.0;
        std::visit([&](const auto& src)
        {
            using T = std::decay_t<decltype(src)>;
            if constexpr (std::is_same_v<T, WaveformConfig>)
            {
                const double phaseInc = voices.phaseInc[i] * pitchFactor;
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
                w = mainWave;
                sum += mixGain * voices.amp[i] * ccGain * velGain * w * envGain;

                voices.phase[i] += phaseInc;
                if (voices.phase[i] >= 1.0) voices.phase[i] -= 1.0;
            }
            else if constexpr (std::is_same_v<T, NoiseConfig>)
            {
                w = SampleNoise(src.noise);
                sum += mixGain * voices.amp[i] * ccGain * velGain * w * envGain;
            }
            else if constexpr (std::is_same_v<T, FmConfig>)
            {
                const double carrierInc = voices.phaseInc[i] * pitchFactor * src.carrierRatio;
                const double modInc = voices.phaseInc[i] * pitchFactor * src.modRatio;
                w = SampleFmPhase(
                    src.carrierWave,
                    src.modWave,
                    voices.fmCarrierPhase[i],
                    voices.fmModPhase[i],
                    carrierInc,
                    modInc,
                    src.index);
                sum += mixGain * voices.amp[i] * src.outLevel * ccGain * velGain * w * envGain;

                voices.fmCarrierPhase[i] += carrierInc;
                if (voices.fmCarrierPhase[i] >= 1.0) voices.fmCarrierPhase[i] -= 1.0;
                voices.fmModPhase[i] += modInc;
                if (voices.fmModPhase[i] >= 1.0) voices.fmModPhase[i] -= 1.0;
            }
            else if constexpr (std::is_same_v<T, DrumConfig>)
            {
                // Drum は NoteOn 後に自動リリースへ遷移する one-shot 系を想定する。
                const double drumGain = (src.gain > 0.0) ? src.gain : 1.0;
                w = RenderDrumSample(src, voices, i, dt, sound.fs);
                sum += mixGain * drumGain * voices.amp[i] * ccGain * velGain * w * envGain;
            }
        }, voices.source[i]);
    }

    return sum;
}
