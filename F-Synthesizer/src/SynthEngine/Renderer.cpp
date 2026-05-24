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
    double pluckMul = 1.0;
    double stringMul = 1.0;
    double bodyMul = 1.0;
    double padBrightnessAdd = 0.0;
    double stringBrightnessAdd = 0.0;
    double driveAdd = 0.0;
    double filterDriveAdd = 0.0;
};

double Clamp01(double v)
{
    return std::clamp(v, 0.0, 1.0);
}

double AmountMul(double expressionVelocity, double amount)
{
    return 1.0 + expressionVelocity * Clamp01(amount);
}

double OnePoleAlpha(double cutoffHz, double sampleRate)
{
    return 1.0 - std::exp(-2.0 * kPi * std::clamp(cutoffHz, 20.0, sampleRate * 0.45) / sampleRate);
}

double SoftClipNorm(double x, double drive)
{
    const double amount = std::clamp(drive, 0.0, 1.0);
    if (amount <= 0.0)
    {
        return x;
    }
    const double k = 1.0 + amount * 8.0;
    return std::tanh(x * k) / std::tanh(k);
}

void AddStereoLayer(StereoFrame& sum, const StereoFrame layer)
{
    sum.left += layer.left;
    sum.right += layer.right;
}

StereoFrame ApplyDrumBus(
    DrumBusRuntimeState& st,
    const DrumBusConfig& cfg,
    StereoFrame in,
    int sampleRate)
{
    if (!cfg.enabled)
    {
        return in;
    }

    double l = in.left;
    double r = in.right;
    const double monoAbs = std::abs((l + r) * 0.5);
    const double fastA = OnePoleAlpha(120.0, sampleRate);
    const double slowA = OnePoleAlpha(12.0, sampleRate);
    st.envFast += (monoAbs - st.envFast) * fastA;
    st.envSlow += (monoAbs - st.envSlow) * slowA;

    const double transient = std::max(0.0, st.envFast - st.envSlow);
    const double attackGain = 1.0 - std::clamp(cfg.attackTrim, 0.0, 1.0) * std::clamp(transient * 5.0, 0.0, 0.55);
    l *= attackGain;
    r *= attackGain;

    const double glue = std::clamp(cfg.glue, 0.0, 1.0);
    if (glue > 0.0)
    {
        const double threshold = 0.24 + (1.0 - glue) * 0.34;
        const double over = std::max(0.0, st.envFast - threshold);
        const double gain = 1.0 / (1.0 + over * (2.0 + glue * 8.0));
        l *= gain;
        r *= gain;
    }

    const double lowA = OnePoleAlpha(95.0 + std::clamp(cfg.lowTighten, 0.0, 1.0) * 80.0, sampleRate);
    st.lowLpL += (l - st.lowLpL) * lowA;
    st.lowLpR += (r - st.lowLpR) * lowA;
    l -= st.lowLpL * std::clamp(cfg.lowTighten, 0.0, 1.0) * 0.32;
    r -= st.lowLpR * std::clamp(cfg.lowTighten, 0.0, 1.0) * 0.32;

    const double presence = std::clamp(cfg.presenceCut, 0.0, 1.0);
    if (presence > 0.0)
    {
        const double presA = OnePoleAlpha(3200.0 - presence * 1000.0, sampleRate);
        st.presenceLpL += (l - st.presenceLpL) * presA;
        st.presenceLpR += (r - st.presenceLpR) * presA;
        l = l * (1.0 - presence * 0.48) + st.presenceLpL * (presence * 0.48);
        r = r * (1.0 - presence * 0.48) + st.presenceLpR * (presence * 0.48);
    }

    const double sustain = std::clamp(cfg.sustainLift, 0.0, 1.0);
    if (sustain > 0.0)
    {
        const double bodyGain = 1.0 + sustain * (0.18 + 0.18 * (1.0 - std::clamp(transient * 6.0, 0.0, 1.0)));
        l *= bodyGain;
        r *= bodyGain;
    }

    const double roomSend = std::clamp(cfg.roomSend, 0.0, 1.0);
    if (roomSend > 0.0)
    {
        const double mono = (l + r) * 0.5;
        st.roomL = st.roomL * 0.84 + mono * 0.16;
        st.roomR = st.roomR * 0.79 - mono * 0.13;
        st.roomDiffL = st.roomDiffL * 0.68 + st.roomR * 0.32;
        st.roomDiffR = st.roomDiffR * 0.72 - st.roomL * 0.28;
        l += (st.roomL + st.roomDiffL * 0.45) * roomSend * 0.32;
        r += (st.roomR + st.roomDiffR * 0.45) * roomSend * 0.32;
    }

    const double driveTrim = std::clamp(cfg.driveTrim, 0.0, 1.0);
    if (driveTrim > 0.0)
    {
        l = l * (1.0 - driveTrim * 0.18) + SoftClipNorm(l, 0.16) * (driveTrim * 0.18);
        r = r * (1.0 - driveTrim * 0.18) + SoftClipNorm(r, 0.16) * (driveTrim * 0.18);
    }

    const double level = std::clamp(cfg.level, 0.0, 2.0);
    return StereoFrame{ l * level, r * level };
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
    out.pluckMul = AmountMul(out.expressionVelocity, map.velocityToPluck);
    out.stringMul = AmountMul(out.expressionVelocity, map.velocityToString) + Clamp01(modwheel) * Clamp01(map.modWheelToString);
    out.bodyMul = AmountMul(out.expressionVelocity, map.velocityToBody);
    out.padBrightnessAdd = (Clamp01(cc74Brightness) - 0.5) * std::clamp(map.cc74ToPadBrightness, -1.0, 1.0);
    out.stringBrightnessAdd = (Clamp01(cc74Brightness) - 0.5) * std::clamp(map.cc74ToStringBrightness, -1.0, 1.0);
    out.driveAdd = Clamp01(pressure) * Clamp01(map.pressureToDrive);
    out.filterDriveAdd = Clamp01(pressure) * Clamp01(map.pressureToFilterDrive);
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
    auto& channelSums = state.renderChannelSums;
    auto& channelDrumBus = state.renderChannelDrumBus;
    auto& channelHasDrumBus = state.renderChannelHasDrumBus;
    channelSums.fill(StereoFrame{});
    channelHasDrumBus.fill(false);
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

        const int ch = voices.channelIndex[i];
        const double envGain = StepADSR(
            voices.env[i],
            dt,
            voices.attackSec[i] * state.channelAttackScale[ch],
            voices.decaySec[i] * state.channelDecayScale[ch],
            std::clamp(voices.sustainLevel[i] + state.channelSustainAdd[ch], 0.0, 1.0),
            voices.releaseSec[i] * state.channelReleaseScale[ch]);
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
        // mute/solo/mixGain 判定は事前計算済みフラグを参照する。
        // 目的: ホットループの分岐段数を減らし、分岐予測ミスを抑える。
        // 前提: channel mix 状態は RenderMIDIEvents 実行中に変化しない。
        // トレードオフ: 判定ロジックが初期化側へ移動し、追跡箇所が分かれる。
        if (!state.channelRenderable[ch])
        {
            continue;
        }
        const uint8_t fastPathMask = voices.fastPathMask[i];
        in.mixGainL = state.channelMixGainL[ch];
        in.mixGainR = state.channelMixGainR[ch];
        in.pitchFactor = state.channelPitch[ch];
        in.ccGain = state.channelCcGain[ch];
        in.modwheel = state.channelModwheel[ch];
        if (state.channelPressure[ch] > 0.0)
        {
            in.channelPressure = state.channelPressure[ch];
        }
        if (state.channelHasPolyPressure[ch])
        {
            const int note = std::clamp(voices.noteNumber[i], 0, 127);
            in.polyPressure = state.channelPolyPressure[ch][note];
        }
        if ((fastPathMask & kVoiceFastPathExpressionDisabled) != 0)
        {
            in.velocityNorm = voices.expressionDefaultVelocityNorm[i];
            in.expressionVelocity = in.velocityNorm;
            in.velGain = voices.expressionDefaultAmpVelocity[i];
            in.brightness = state.channelBrightness[ch];
            in.brightnessCutoffScale = state.channelBrightnessCutoffScale[ch];
        }
        else
        {
            const double pressure =
                (in.channelPressure > in.polyPressure) ? in.channelPressure : in.polyPressure;
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
            in.expressionPluckMul = expr.pluckMul;
            in.expressionStringMul = expr.stringMul;
            in.expressionBodyMul = expr.bodyMul;
            in.expressionPadBrightnessAdd = expr.padBrightnessAdd;
            in.expressionStringBrightnessAdd = expr.stringBrightnessAdd;
            in.expressionDriveAdd = expr.driveAdd;
            in.expressionFilterDriveAdd = expr.filterDriveAdd;
            in.brightness = std::clamp(state.channelBrightness[ch] + expr.brightnessAdd, 0.0, 1.0);
            in.brightnessCutoffScale = RenderCutoffScaleFromBrightness(in.brightness);
        }
        in.resonance = state.channelResonance[ch];
        in.resonanceScale = state.channelResonanceScale[ch];

        // ポルタメント: 現在ピッチをターゲットへ指数平滑しつつ pitchFactor へ反映する。
        if (state.channelPortamentoOn[ch])
        {
            const double effectivePortamentoTimeSec =
                ((fastPathMask & kVoiceFastPathPortamentoDisabled) != 0)
                    ? state.channelPortamentoTimeSec[ch]
                    : (std::max)(voices.portamentoTimeSec[i], state.channelPortamentoTimeSec[ch]);
            if (effectivePortamentoTimeSec > 0.0)
            {
                if (std::abs(voices.portamentoPitchHz[i] - voices.portamentoTargetHz[i]) > 0.01)
                {
                    voices.portamentoPitchHz[i] +=
                        (voices.portamentoTargetHz[i] - voices.portamentoPitchHz[i]) *
                        (1.0 - std::exp(-in.dt / effectivePortamentoTimeSec));
                }
                // portamentoPitchHz を phaseInc に対する倍率として pitchFactor へ乗算する。
                // (phaseInc = targetHz / sampleRate なので比率で戻す)
                if (voices.portamentoTargetHz[i] > 0.0)
                {
                    in.pitchFactor *= voices.portamentoPitchHz[i] / voices.portamentoTargetHz[i];
                }
            }
        }

        SourceRenderFrame frame{};
        RenderSourceFrame(voices.source[i], voices, i, in, sound.fs, frame);
        const uint32_t layerMask = voices.layerMask[i];
        if ((layerMask & kVoiceLayerAttack) != 0) frame.sample += RenderAttackLayer(voices, i, in);
        if ((layerMask & kVoiceLayerBass) != 0) frame.sample += RenderBassLayer(voices, i, in);
        if ((layerMask & kVoiceLayerLead) != 0) frame.sample += RenderLeadLayer(voices, i, in);
        StereoFrame layerSum{};
        if ((layerMask & kVoiceLayerPluck) != 0)
        {
            AddStereoLayer(layerSum, RenderPluckLayer(voices, i, in));
        }
        if ((layerMask & kVoiceLayerString) != 0)
        {
            AddStereoLayer(layerSum, RenderStringLayer(voices, i, in));
        }
        if ((layerMask & kVoiceLayerChord) != 0)
        {
            AddStereoLayer(layerSum, RenderChordLayer(voices, i, in));
        }
        if ((layerMask & kVoiceLayerPad) != 0)
        {
            AddStereoLayer(layerSum, RenderPadLayer(voices, i, in));
        }
        if ((layerMask & kVoiceLayerHarmonic) != 0)
        {
            AddStereoLayer(layerSum, RenderHarmonicLayer(voices, i, in));
        }
        if ((layerMask & kVoiceLayerPowerChord) != 0)
        {
            AddStereoLayer(layerSum, RenderPowerChordLayer(voices, i, in));
        }
        if ((layerMask & kVoiceLayerChug) != 0)
        {
            AddStereoLayer(layerSum, RenderChugLayer(voices, i, in));
        }
        frame.sample += (layerSum.left + layerSum.right) * 0.5;
        frame.stereoOffsetL += (layerSum.left - layerSum.right) * 0.5;
        frame.stereoOffsetR += (layerSum.right - layerSum.left) * 0.5;
        ApplyCommonShaper(voices.source[i], voices, i, in, frame);
        if ((layerMask & kVoiceLayerAmpCab) != 0) ApplyAmpCabLayer(voices, i, in, frame);
        if ((layerMask & kVoiceLayerBody) != 0) ApplyBodyLayer(voices, i, in, frame);
        ApplyModulationLayer(voices.source[i], voices, i, frame);
        voices.ageSec[i] += in.dt;

        const double gain =
            frame.sourceGain *
            voices.amp[i] *
            in.ccGain *
            in.velGain *
            in.envGain *
            frame.ampMul;
        const double mono = gain * frame.sample;
        const double stereoL = gain * (frame.sample + frame.stereoOffsetL);
        const double stereoR = gain * (frame.sample + frame.stereoOffsetR);
        channelSums[ch].left += stereoL;
        channelSums[ch].right += stereoR;
        if (voices.drumBus[i].enabled)
        {
            channelDrumBus[ch] = voices.drumBus[i];
            channelHasDrumBus[ch] = true;
        }
    }

    StereoFrame sum{};
    for (int ch = 0; ch < 16; ch++)
    {
        StereoFrame frame = channelSums[ch];
        if (channelHasDrumBus[ch])
        {
            frame = ApplyDrumBus(state.drumBusState[ch], channelDrumBus[ch], frame, sound.fs);
        }
        sum.left += frame.left * state.channelMixGainL[ch];
        sum.right += frame.right * state.channelMixGainR[ch];
    }
    return sum;
}
