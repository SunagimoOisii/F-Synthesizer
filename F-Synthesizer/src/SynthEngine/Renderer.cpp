#include "Internal.h"

#include <algorithm>
#include <cmath>
#include <type_traits>

#include "synth/Oscillator.h"

RenderWorkerPool::RenderWorkerPool(size_t workerCount)
{
    workers_.reserve(workerCount);
    for (size_t i = 0; i < workerCount; i++)
    {
        workers_.emplace_back([this]() { WorkerLoop(); });
    }
}

RenderWorkerPool::~RenderWorkerPool()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stopping_ = true;
        generation_++;
    }
    workCv_.notify_all();
    for (auto& worker : workers_)
    {
        if (worker.joinable())
        {
            worker.join();
        }
    }
}

bool RenderWorkerPool::Run(size_t jobCount, const std::function<void(size_t)>& job)
{
    return RunWithCaller(jobCount, job, {});
}

bool RenderWorkerPool::RunWithCaller(
    size_t jobCount,
    const std::function<void(size_t)>& job,
    const std::function<void()>& callerJob)
{
    if (jobCount == 0)
    {
        if (callerJob)
        {
            try
            {
                callerJob();
            }
            catch (...)
            {
                return false;
            }
        }
        return true;
    }

    {
        std::lock_guard<std::mutex> lock(mutex_);
        job_ = job;
        jobCount_ = jobCount;
        nextJob_ = 0;
        completedJobs_ = 0;
        exception_ = nullptr;
        generation_++;
    }
    workCv_.notify_all();

    if (callerJob)
    {
        try
        {
            callerJob();
        }
        catch (...)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (exception_ == nullptr)
            {
                exception_ = std::current_exception();
            }
        }
    }

    std::unique_lock<std::mutex> lock(mutex_);
    doneCv_.wait(lock, [this]() { return completedJobs_ >= jobCount_; });
    const bool ok = (exception_ == nullptr);
    job_ = nullptr;
    return ok;
}

void RenderWorkerPool::WorkerLoop()
{
    size_t seenGeneration = 0;
    for (;;)
    {
        size_t jobIndex = 0;
        std::function<void(size_t)> job;
        {
            std::unique_lock<std::mutex> lock(mutex_);
            workCv_.wait(lock, [this, &seenGeneration]() {
                return stopping_ || generation_ != seenGeneration;
            });
            if (stopping_)
            {
                return;
            }
            seenGeneration = generation_;
            for (;;)
            {
                if (stopping_)
                {
                    return;
                }
                if (nextJob_ >= jobCount_)
                {
                    break;
                }
                jobIndex = nextJob_++;
                job = job_;
                break;
            }
        }

        while (job)
        {
            try
            {
                job(jobIndex);
            }
            catch (...)
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (exception_ == nullptr)
                {
                    exception_ = std::current_exception();
                }
            }

            {
                std::unique_lock<std::mutex> lock(mutex_);
                completedJobs_++;
                if (completedJobs_ >= jobCount_)
                {
                    doneCv_.notify_all();
                }
                if (nextJob_ >= jobCount_)
                {
                    break;
                }
                jobIndex = nextJob_++;
                job = job_;
            }
        }
    }
}

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

struct ChannelRenderContext
{
    int ch = 0;
    int sampleRate = 44100;
    double dt = 1.0 / 44100.0;
    double mixGainL = 1.0;
    double mixGainR = 1.0;
    double ccGain = 1.0;
    double pitch = 1.0;
    double modwheel = 0.0;
    double pressure = 0.0;
    double brightness = 0.5;
    double resonance = 0.5;
    double attackScale = 1.0;
    double decayScale = 1.0;
    double sustainAdd = 0.0;
    double releaseScale = 1.0;
    double brightnessCutoffScale = 1.0;
    double resonanceScale = 1.0;
    double portamentoTimeSec = 0.0;
    bool renderable = true;
    bool hasPolyPressure = false;
    bool portamentoOn = true;
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

void RenderSourceFrameByKind(
    config::SourceKind kind,
    const SourceConfig& src,
    Voice& voices,
    size_t i,
    const VoiceRenderInput& in,
    int sampleRate,
    SourceRenderFrame& frame)
{
    switch (kind)
    {
    case config::SourceKind::Waveform:
        if (const auto* source = std::get_if<WaveformConfig>(&src))
        {
            RenderWaveformSource(*source, voices, i, in, frame);
            return;
        }
        break;
    case config::SourceKind::Analog:
        if (const auto* source = std::get_if<AnalogConfig>(&src))
        {
            RenderAnalogSource(*source, voices, i, in, frame);
            return;
        }
        break;
    case config::SourceKind::Noise:
        if (const auto* source = std::get_if<NoiseConfig>(&src))
        {
            RenderNoiseSource(*source, frame);
            return;
        }
        break;
    case config::SourceKind::Fm:
        if (const auto* source = std::get_if<FmConfig>(&src))
        {
            RenderFmSource(*source, voices, i, in, frame);
            return;
        }
        break;
    case config::SourceKind::Drum:
        if (const auto* source = std::get_if<DrumConfig>(&src))
        {
            RenderDrumSource(*source, voices, i, in, sampleRate, frame);
            return;
        }
        break;
    case config::SourceKind::Psg:
        if (const auto* source = std::get_if<PsgConfig>(&src))
        {
            RenderPsgSource(*source, voices, i, in, frame);
            return;
        }
        break;
    default:
        break;
    }

    RenderSourceFrame(src, voices, i, in, sampleRate, frame);
}

ChannelRenderContext BuildChannelRenderContext(const RenderState& state, const SoundData& sound, int ch)
{
    ChannelRenderContext ctx{};
    ctx.ch = ch;
    ctx.sampleRate = sound.fs;
    ctx.dt = 1.0 / sound.fs;
    ctx.mixGainL = state.channelMixGainL[ch];
    ctx.mixGainR = state.channelMixGainR[ch];
    ctx.ccGain = state.channelCcGain[ch];
    ctx.pitch = state.channelPitch[ch];
    ctx.modwheel = state.channelModwheel[ch];
    ctx.pressure = state.channelPressure[ch];
    ctx.brightness = state.channelBrightness[ch];
    ctx.resonance = state.channelResonance[ch];
    ctx.attackScale = state.channelAttackScale[ch];
    ctx.decayScale = state.channelDecayScale[ch];
    ctx.sustainAdd = state.channelSustainAdd[ch];
    ctx.releaseScale = state.channelReleaseScale[ch];
    ctx.brightnessCutoffScale = state.channelBrightnessCutoffScale[ch];
    ctx.resonanceScale = state.channelResonanceScale[ch];
    ctx.portamentoTimeSec = state.channelPortamentoTimeSec[ch];
    ctx.renderable = state.channelRenderable[ch];
    ctx.hasPolyPressure = state.channelHasPolyPressure[ch];
    ctx.portamentoOn = state.channelPortamentoOn[ch];
    return ctx;
}

bool RenderVoiceSampleToChannel(
    RenderState& state,
    const ChannelRenderContext& ctx,
    size_t i,
    int sourceKindIndex,
    StereoFrame& channelSum,
    DrumBusConfig& channelDrumBus,
    bool& channelHasDrumBus)
{
    auto& voices = state.voices;
    if (voices.pendingRemove[i] != 0 || voices.env[i].stage == ADSRStage::Off)
    {
        return false;
    }

    const double envGain = StepADSR(
        voices.env[i],
        ctx.dt,
        voices.attackSec[i] * ctx.attackScale,
        voices.decaySec[i] * ctx.decayScale,
        std::clamp(voices.sustainLevel[i] + ctx.sustainAdd, 0.0, 1.0),
        voices.releaseSec[i] * ctx.releaseScale);
    if (voices.pendingRemove[i] == 0 && voices.env[i].stage == ADSRStage::Off)
    {
        voices.pendingRemove[i] = 1;
        return true;
    }

    VoiceRenderInput in{};
    in.dt = ctx.dt;
    in.envGain = envGain;
    if (!ctx.renderable)
    {
        return false;
    }

    const uint8_t fastPathMask = voices.fastPathMask[i];
    in.mixGainL = ctx.mixGainL;
    in.mixGainR = ctx.mixGainR;
    in.pitchFactor = ctx.pitch;
    in.ccGain = ctx.ccGain;
    in.modwheel = ctx.modwheel;
    if (ctx.pressure > 0.0)
    {
        in.channelPressure = ctx.pressure;
    }
    if (ctx.hasPolyPressure)
    {
        const int note = std::clamp(voices.noteNumber[i], 0, 127);
        in.polyPressure = state.channelPolyPressure[ctx.ch][note];
    }
    if ((fastPathMask & kVoiceFastPathExpressionDisabled) != 0)
    {
        in.velocityNorm = voices.expressionDefaultVelocityNorm[i];
        in.expressionVelocity = in.velocityNorm;
        in.velGain = voices.runtimeDefaultAmpVelocity[i];
        in.brightness = ctx.brightness;
        in.brightnessCutoffScale = ctx.brightnessCutoffScale;
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
            ctx.brightness);
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
        in.brightness = std::clamp(ctx.brightness + expr.brightnessAdd, 0.0, 1.0);
        in.brightnessCutoffScale = RenderCutoffScaleFromBrightness(in.brightness);
    }
    in.resonance = ctx.resonance;
    in.resonanceScale = ctx.resonanceScale;

    if (ctx.portamentoOn)
    {
        const double effectivePortamentoTimeSec =
            ((fastPathMask & kVoiceFastPathPortamentoDisabled) != 0)
                ? ctx.portamentoTimeSec
                : (std::max)(voices.portamentoTimeSec[i], ctx.portamentoTimeSec);
        if (effectivePortamentoTimeSec > 0.0)
        {
            if (std::abs(voices.portamentoPitchHz[i] - voices.portamentoTargetHz[i]) > 0.01)
            {
                voices.portamentoPitchHz[i] +=
                    (voices.portamentoTargetHz[i] - voices.portamentoPitchHz[i]) *
                    (1.0 - std::exp(-in.dt / effectivePortamentoTimeSec));
            }
            if (voices.portamentoTargetHz[i] > 0.0)
            {
                in.pitchFactor *= voices.portamentoPitchHz[i] / voices.portamentoTargetHz[i];
            }
        }
    }

    SourceRenderFrame frame{};
    const config::SourceKind sourceKind = config::SourceKindFromIndex(sourceKindIndex);
    RenderSourceFrameByKind(sourceKind, voices.source[i], voices, i, in, ctx.sampleRate, frame);
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
        voices.runtimeAmp[i] *
        in.ccGain *
        in.velGain *
        in.envGain *
        frame.ampMul;
    const double stereoL = gain * (frame.sample + frame.stereoOffsetL);
    const double stereoR = gain * (frame.sample + frame.stereoOffsetR);
    channelSum.left += stereoL;
    channelSum.right += stereoR;
    if (voices.runtimeHasDrumBus[i] != 0)
    {
        channelDrumBus = voices.drumBus[i];
        channelHasDrumBus = true;
    }
    return false;
}

size_t MixChannelBlockToOutput(
    RenderState& state,
    const SoundData& sound,
    int ch,
    int frameCount,
    std::vector<StereoFrame>& outFrames,
    bool replaceOutput)
{
    size_t removedCount = 0;
    const ChannelRenderContext ctx = BuildChannelRenderContext(state, sound, ch);
    const auto& sourceBuckets = state.activeVoiceIndicesByChannelSource[ch];
    const auto& activeSourceKinds = state.activeSourceKindsByChannel[ch];
    for (int offset = 0; offset < frameCount; offset++)
    {
        StereoFrame channelSum{};
        DrumBusConfig channelDrumBus{};
        bool channelHasDrumBus = false;
        for (const int sourceKind : activeSourceKinds)
        {
            for (const size_t i : sourceBuckets[sourceKind])
            {
                if (RenderVoiceSampleToChannel(state, ctx, i, sourceKind, channelSum, channelDrumBus, channelHasDrumBus))
                {
                    removedCount++;
                }
            }
        }
        if (channelHasDrumBus)
        {
            channelSum = ApplyDrumBus(state.drumBusState[ch], channelDrumBus, channelSum, sound.fs);
        }
        StereoFrame& out = outFrames[static_cast<size_t>(offset)];
        const double left = channelSum.left * ctx.mixGainL;
        const double right = channelSum.right * ctx.mixGainR;
        if (replaceOutput)
        {
            out = StereoFrame{ left, right };
        }
        else
        {
            out.left += left;
            out.right += right;
        }
    }
    return removedCount;
}

size_t RenderChannelBlockToBuffer(
    RenderState& state,
    const SoundData& sound,
    int ch,
    int frameCount,
    std::vector<StereoFrame>& channelFrames)
{
    size_t removedCount = 0;
    channelFrames.resize(static_cast<size_t>(frameCount));
    const ChannelRenderContext ctx = BuildChannelRenderContext(state, sound, ch);
    const auto& sourceBuckets = state.activeVoiceIndicesByChannelSource[ch];
    const auto& activeSourceKinds = state.activeSourceKindsByChannel[ch];
    for (int offset = 0; offset < frameCount; offset++)
    {
        StereoFrame channelSum{};
        DrumBusConfig channelDrumBus{};
        bool channelHasDrumBus = false;
        for (const int sourceKind : activeSourceKinds)
        {
            for (const size_t i : sourceBuckets[sourceKind])
            {
                if (RenderVoiceSampleToChannel(state, ctx, i, sourceKind, channelSum, channelDrumBus, channelHasDrumBus))
                {
                    removedCount++;
                }
            }
        }
        if (channelHasDrumBus)
        {
            channelSum = ApplyDrumBus(state.drumBusState[ch], channelDrumBus, channelSum, sound.fs);
        }
        channelFrames[static_cast<size_t>(offset)] = StereoFrame{
            channelSum.left * ctx.mixGainL,
            channelSum.right * ctx.mixGainR
        };
    }
    return removedCount;
}

size_t ResolveRenderWorkerCount(size_t workerJobCount)
{
    const unsigned int hardwareThreads = std::thread::hardware_concurrency();
    if (hardwareThreads <= 1 || workerJobCount == 0)
    {
        return 0;
    }
    const size_t availableWorkers = static_cast<size_t>(hardwareThreads - 1);
    return std::min<size_t>({ availableWorkers, workerJobCount, 4 });
}

bool EnsureRenderWorkerPool(RenderState& state, size_t workerJobCount)
{
    if (state.renderParallelDisabled)
    {
        return false;
    }
    const size_t workerCount = ResolveRenderWorkerCount(workerJobCount);
    if (workerCount == 0)
    {
        return false;
    }
    if (state.renderWorkerPool && state.renderWorkerPool->WorkerCount() >= workerCount)
    {
        return true;
    }
    try
    {
        state.renderWorkerPool = std::make_unique<RenderWorkerPool>(workerCount);
    }
    catch (...)
    {
        state.renderWorkerPool.reset();
        state.renderParallelDisabled = true;
        return false;
    }
    return state.renderWorkerPool != nullptr && state.renderWorkerPool->WorkerCount() > 0;
}

bool HasActiveDrumBusVoice(const RenderState& state, const std::vector<int>& activeChannels)
{
    for (const int ch : activeChannels)
    {
        for (const size_t i : state.activeVoiceIndicesByChannel[ch])
        {
            if (state.voices.runtimeHasDrumBus[i] != 0)
            {
                return true;
            }
        }
    }
    return false;
}

bool RenderVoicesBlockParallel(
    RenderState& state,
    const SoundData& sound,
    int frameCount,
    std::vector<StereoFrame>& outFrames,
    const std::vector<int>& activeChannels,
    size_t& removedCount)
{
    const size_t workerJobCount = activeChannels.size() - 1;
    if (frameCount < 32 ||
        activeChannels.size() < 2 ||
        HasActiveDrumBusVoice(state, activeChannels) ||
        !EnsureRenderWorkerPool(state, workerJobCount))
    {
        return false;
    }

    for (const int ch : activeChannels)
    {
        state.renderChannelBlockFrames[ch].resize(static_cast<size_t>(frameCount));
    }

    std::array<size_t, 16> removedByChannel{};
    const bool ok = state.renderWorkerPool->RunWithCaller(activeChannels.size() - 1, [&](size_t jobIndex) {
        const int ch = activeChannels[jobIndex + 1];
        removedByChannel[ch] = RenderChannelBlockToBuffer(
            state,
            sound,
            ch,
            frameCount,
            state.renderChannelBlockFrames[ch]);
    }, [&]() {
        const int ch = activeChannels.front();
        removedByChannel[ch] = RenderChannelBlockToBuffer(
            state,
            sound,
            ch,
            frameCount,
            state.renderChannelBlockFrames[ch]);
    });
    if (!ok)
    {
        state.renderParallelDisabled = true;
        return false;
    }

    removedCount = 0;
    std::fill(outFrames.begin(), outFrames.end(), StereoFrame{});
    bool replaceOutput = true;
    for (const int ch : activeChannels)
    {
        removedCount += removedByChannel[ch];
        const auto& channelFrames = state.renderChannelBlockFrames[ch];
        for (int offset = 0; offset < frameCount; offset++)
        {
            StereoFrame& out = outFrames[static_cast<size_t>(offset)];
            const StereoFrame frame = channelFrames[static_cast<size_t>(offset)];
            if (replaceOutput)
            {
                out = frame;
            }
            else
            {
                out.left += frame.left;
                out.right += frame.right;
            }
        }
        replaceOutput = false;
    }
    return true;
}

} // namespace

void RenderVoicesBlock(RenderState& state, const SoundData& sound, int frameCount, std::vector<StereoFrame>& outFrames)
{
    if (frameCount <= 0)
    {
        outFrames.clear();
        return;
    }
    if (state.activeVoiceIndicesDirty)
    {
        RebuildActiveVoiceIndices(state);
    }

    outFrames.resize(static_cast<size_t>(frameCount));
    auto& activeChannels = state.renderActiveChannels;
    activeChannels.clear();
    activeChannels.reserve(16);
    for (int ch = 0; ch < 16; ch++)
    {
        if (!state.activeVoiceIndicesByChannel[ch].empty())
        {
            activeChannels.push_back(ch);
        }
    }

    if (activeChannels.empty())
    {
        std::fill(outFrames.begin(), outFrames.end(), StereoFrame{});
        return;
    }

    size_t removedCount = 0;
    if (!RenderVoicesBlockParallel(state, sound, frameCount, outFrames, activeChannels, removedCount))
    {
        bool replaceOutput = true;
        for (const int ch : activeChannels)
        {
            removedCount += MixChannelBlockToOutput(state, sound, ch, frameCount, outFrames, replaceOutput);
            replaceOutput = false;
        }
    }

    if (removedCount > 0)
    {
        state.pendingRemoveCount += removedCount;
        MarkActiveVoiceIndicesDirty(state);
    }
}
