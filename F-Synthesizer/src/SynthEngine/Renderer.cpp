#include "Internal.h"

#include <algorithm>
#include <cmath>
#include <type_traits>

#include "synth/Oscillator.h"

namespace
{
constexpr double kPi = 3.14159265358979323846;

#include "renderer/RenderCommon.inl"
#include "renderer/RenderWaveform.inl"
#include "renderer/RenderFm.inl"
#include "renderer/RenderDrum.inl"
#include "renderer/RenderNoise.inl"
#include "renderer/RenderMix.inl"
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
