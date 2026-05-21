#include "Internal.h"

#include <algorithm>
#include <cmath>
#include <type_traits>

#include "synth/Oscillator.h"

namespace
{
constexpr double kPi = 3.14159265358979323846;

struct ExpressionRuntime
{
    double velocityNorm = 1.0;
    double expressionVelocity = 1.0;
    double ampVelocity = 1.0;
    double brightnessAdd = 0.0;
    double fmIndexMul = 1.0;
    double attackMul = 1.0;
    double bassMul = 1.0;
    double leadMul = 1.0;
    double chordMul = 1.0;
    double padMul = 1.0;
    double padBrightnessAdd = 0.0;
    double driveAdd = 0.0;
};

double Clamp01(double v)
{
    return std::clamp(v, 0.0, 1.0);
}

double AmountMul(double expressionVelocity, double amount)
{
    return 1.0 + expressionVelocity * Clamp01(amount);
}

ExpressionRuntime EvaluateExpressionMap(
    const ExpressionMapConfig& map,
    int velocity,
    double modwheel,
    double pressure,
    double cc74Brightness)
{
    ExpressionRuntime out{};
    out.velocityNorm = Clamp01(static_cast<double>(std::clamp(velocity, 0, 127)) / 127.0);

    if (!map.enabled)
    {
        out.expressionVelocity = out.velocityNorm;
        out.ampVelocity = VelocityToGain(velocity);
        return out;
    }

    const double curve = std::clamp(map.velocityCurve, 0.2, 3.0);
    out.expressionVelocity = std::pow(std::max(out.velocityNorm, 1.0e-6), curve);

    const double velocityToAmp = Clamp01(map.velocityToAmp);
    out.ampVelocity = std::clamp((1.0 - velocityToAmp) + out.expressionVelocity * velocityToAmp, 0.0, 1.0);
    out.brightnessAdd =
        (out.expressionVelocity - 0.5) * std::clamp(map.velocityToBrightness, -1.0, 1.0) +
        Clamp01(modwheel) * std::clamp(map.modWheelToBrightness, -1.0, 1.0) +
        (Clamp01(cc74Brightness) - 0.5) * std::clamp(map.cc74ToBrightness, -1.0, 1.0);
    out.fmIndexMul = AmountMul(out.expressionVelocity, map.velocityToFmIndex);
    out.attackMul = AmountMul(out.expressionVelocity, map.velocityToAttack);
    out.bassMul = AmountMul(out.expressionVelocity, map.velocityToBass);
    out.leadMul = AmountMul(out.expressionVelocity, map.velocityToLead);
    out.chordMul = AmountMul(out.expressionVelocity, map.velocityToChord);
    out.padMul = AmountMul(out.expressionVelocity, map.velocityToPad) + Clamp01(modwheel) * Clamp01(map.modWheelToPad);
    out.padBrightnessAdd = (Clamp01(cc74Brightness) - 0.5) * std::clamp(map.cc74ToPadBrightness, -1.0, 1.0);
    out.driveAdd = Clamp01(pressure) * Clamp01(map.pressureToDrive);
    return out;
}

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
        const double pressure = (std::max)(in.channelPressure, in.polyPressure);
        const ExpressionRuntime expr = EvaluateExpressionMap(
            voices.expressionMap[i],
            voices.velocity[i],
            in.modwheel,
            pressure,
            state.channelBrightness[ch]);
        in.velocityNorm = expr.velocityNorm;
        in.expressionVelocity = expr.expressionVelocity;
        in.velGain = expr.ampVelocity;
        in.expressionFmIndexMul = expr.fmIndexMul;
        in.expressionAttackMul = expr.attackMul;
        in.expressionBassMul = expr.bassMul;
        in.expressionLeadMul = expr.leadMul;
        in.expressionChordMul = expr.chordMul;
        in.expressionPadMul = expr.padMul;
        in.expressionPadBrightnessAdd = expr.padBrightnessAdd;
        in.expressionDriveAdd = expr.driveAdd;
        in.brightness = std::clamp(state.channelBrightness[ch] + expr.brightnessAdd, 0.0, 1.0);
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
        frame.sample += RenderAttackLayer(voices, i, in);
        frame.sample += RenderBassLayer(voices, i, in);
        frame.sample += RenderLeadLayer(voices, i, in);
        frame.sample += RenderChordLayer(voices, i, in);
        frame.sample += RenderPadLayer(voices, i, in);
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
