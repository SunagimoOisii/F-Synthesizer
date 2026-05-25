#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <condition_variable>
#include <exception>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>
#include <variant>
#include <vector>

#include "config/SourceRegistry.h"
#include "SynthEngine/Filter.h"
#include "SynthEngine/Smoothing.h"
#include "SynthEngine/SynthEngine.h"

struct WaveformVoiceState
{
    FilterInstance filter;
    ModulationRuntimeState modulation;
    SmoothedParam ampSmoothing;
    SmoothedParam pitchSmoothing;
    SmoothedParam filterCutoffSmoothing;
    double syncPhase = 0.0;
    double hardSyncFadeFromMain = 0.0;
    double hardSyncFadeFromL = 0.0;
    double hardSyncFadeFromR = 0.0;
    int hardSyncFadeRemaining = 0;
    double ringPhase = 0.0;
    int arpStep = 0;
    double arpElapsedSec = 0.0;
    // ノートオン時に確定する、ユニゾン各voiceのdetune比率キャッシュ。
    std::array<double, 8> unisonDetuneRatio{};
    // filter keytrack の固定比率（noteNumberとfilterKeytrackから算出）。
    double filterKeytrackRatio = 1.0;
    // drive正規化定数 1/tanh(k)（k=drive*20）。
    double driveNorm = 1.0;
};

struct AnalogVoiceState
{
    FilterInstance filter;
    ModulationRuntimeState modulation;
    SmoothedParam ampSmoothing;
    SmoothedParam pitchSmoothing;
    SmoothedParam filterCutoffSmoothing;
    // ドリフトLFO状態。
    double driftPhase = 0.0;
    // ノートオン時に確定するボイス固有のドリフト位相オフセット [0..1)。
    double driftPhaseOffset = 0.0;
    double syncPhase = 0.0;
    double hardSyncFadeFromMain = 0.0;
    double hardSyncFadeFromL = 0.0;
    double hardSyncFadeFromR = 0.0;
    int hardSyncFadeRemaining = 0;
    double ringPhase = 0.0;
    int arpStep = 0;
    double arpElapsedSec = 0.0;
    std::array<double, 8> unisonDetuneRatio{};
    double filterKeytrackRatio = 1.0;
    double driveNorm = 1.0;
};

struct FmVoiceState
{
    std::array<double, 4> opPhase{};
    std::array<ADSRState, 4> opLevelEnv{};
    std::array<ADSRState, 4> opIndexEnv{};
    double op0FeedbackSample = 0.0;
    ModulationRuntimeState modulation;
    FilterInstance filter;
    double driveNorm = 1.0;
};

struct NoiseVoiceState
{
    FilterInstance filter;
};

struct DrumVoiceState
{
    double time = 0.0;
    double bodyFreq = 0.0;
    double pitchStart = 1.0;
    double pitchDecaySec = 0.0;
    double transientPhase = 0.0;
    std::array<double, 4> metalPhase{};
    std::array<double, 4> burstDelaySec{};
    double noisePrev = 0.0;
    double hpPrev = 0.0;
    double hpAlpha = 0.0;
    double lpPrev = 0.0;
    double lpAlpha = 0.0;
    double pitchRatio = 1.0;
    double decayScale = 1.0;
    double velocityNorm = 1.0;
    uint32_t noiseState = 0xA5A5A5A5u;
};

struct DrumKitVoiceState : DrumVoiceState {};

struct PsgVoiceState
{
    double phase = 0.0;
    uint16_t lfsrState = 0xACE1u; // noise用LFSR初期値（非ゼロ必須）
};

using PerSourceVoiceState = std::variant<
    WaveformVoiceState,
    AnalogVoiceState,
    FmVoiceState,
    NoiseVoiceState,
    DrumVoiceState,
    DrumKitVoiceState,
    PsgVoiceState>;

struct ChannelAdsrOffset
{
    double attack = 0.0;
    double decay = 0.0;
    double sustain = 0.0;
    double release = 0.0;
};

struct DrumBusRuntimeState
{
    double envFast = 0.0;
    double envSlow = 0.0;
    double presenceLpL = 0.0;
    double presenceLpR = 0.0;
    double lowLpL = 0.0;
    double lowLpR = 0.0;
    double roomL = 0.0;
    double roomR = 0.0;
    double roomDiffL = 0.0;
    double roomDiffR = 0.0;
};

enum VoiceLayerMask : uint32_t
{
    kVoiceLayerAttack = 1u << 0,
    kVoiceLayerBass = 1u << 1,
    kVoiceLayerLead = 1u << 2,
    kVoiceLayerChord = 1u << 3,
    kVoiceLayerPad = 1u << 4,
    kVoiceLayerPluck = 1u << 5,
    kVoiceLayerString = 1u << 6,
    kVoiceLayerBody = 1u << 7,
    kVoiceLayerHarmonic = 1u << 8,
    kVoiceLayerPowerChord = 1u << 9,
    kVoiceLayerChug = 1u << 10,
    kVoiceLayerAmpCab = 1u << 11
};

enum VoiceFastPathMask : uint8_t
{
    kVoiceFastPathExpressionDisabled = 1u << 0,
    kVoiceFastPathPortamentoDisabled = 1u << 1
};

struct Voice
{
    // SoA 構造:
    // 目的: sample ループで必要な属性を列単位に連続配置し、キャッシュ効率を上げる。
    // 前提: すべての配列は同じ index が同じ Voice を指す。
    // トレードオフ: デバッグ時に 1 Voice の全体像が追いにくくなる。
    std::vector<SourceConfig> source;
    std::vector<int> noteNumber;
    std::vector<int> velocity;
    std::vector<int> channel;
    std::vector<int> channelIndex;
    std::vector<int> noteInstanceID;
    std::vector<uint8_t> released;
    std::vector<uint8_t> pendingRemove;
    std::vector<uint8_t> sustainedPendingOff;

    std::vector<double> amp;
    std::vector<double> attackSec;
    std::vector<double> decaySec;
    std::vector<double> sustainLevel;
    std::vector<double> releaseSec;
    std::vector<AttackLayerConfig> attackLayer;
    std::vector<BassLayerConfig> bassLayer;
    std::vector<LeadLayerConfig> leadLayer;
    std::vector<ChordLayerConfig> chordLayer;
    std::vector<PadLayerConfig> padLayer;
    std::vector<PluckLayerConfig> pluckLayer;
    std::vector<StringLayerConfig> stringLayer;
    std::vector<BodyLayerConfig> bodyLayer;
    std::vector<HarmonicLayerConfig> harmonicLayer;
    std::vector<PowerChordLayerConfig> powerChordLayer;
    std::vector<ChugLayerConfig> chugLayer;
    std::vector<AmpCabLayerConfig> ampCabLayer;
    std::vector<DrumBusConfig> drumBus;
    std::vector<ExpressionMapConfig> expressionMap;
    std::vector<double> expressionDefaultVelocityNorm;
    std::vector<double> expressionDefaultAmpVelocity;
    std::vector<uint32_t> layerMask;
    std::vector<uint8_t> fastPathMask;
    std::vector<double> runtimeAmp;
    std::vector<double> runtimeDefaultAmpVelocity;
    std::vector<uint8_t> runtimeHasDrumBus;
    std::vector<uint8_t> runtimeSourceKind;
    std::vector<uint8_t> runtimeSourceHasModTargets;
    std::vector<uint8_t> runtimeSourceIsOneShot;
    std::vector<ADSRState> env;

    std::vector<double> phase;
    std::vector<double> phaseInc;
    std::vector<double> ageSec;
    std::vector<double> attackPhase;
    std::vector<uint32_t> attackNoiseState;
    std::vector<double> bassPhase;
    std::vector<double> bassFocusPhase;
    std::vector<double> bassLpState;
    std::vector<double> leadPhase;
    std::vector<double> leadDetunePhase;
    std::vector<std::array<double, 4>> chordPhase;
    std::vector<double> chordLpState;
    std::vector<double> padPhase;
    std::vector<double> padDetunePhase;
    std::vector<double> padMotionPhase;
    std::vector<double> padLpState;
    std::vector<double> pluckPhase;
    std::vector<double> pluckLpState;
    std::vector<double> stringPhaseA;
    std::vector<double> stringPhaseB;
    std::vector<double> stringMotionPhase;
    std::vector<std::array<double, 5>> bodyStateL;
    std::vector<std::array<double, 5>> bodyStateR;
    std::vector<std::array<double, 5>> bodyPhase;
    std::vector<std::array<double, 8>> harmonicPhase;
    std::vector<std::array<double, 8>> harmonicPhaseR;
    std::vector<std::array<double, 3>> powerChordPhase;
    std::vector<std::array<double, 3>> powerChordPhaseR;
    std::vector<double> chugPhase;
    std::vector<double> chugBodyPhase;
    std::vector<double> chugLpState;
    std::vector<double> ampCabLpStateL;
    std::vector<double> ampCabLpStateR;
    std::vector<double> ampCabHpStateL;
    std::vector<double> ampCabHpStateR;
    std::vector<double> portamentoPitchHz;  // 現在のスライド中ピッチ（Hz）
    std::vector<double> portamentoTargetHz; // ターゲットピッチ（Hz）
    std::vector<double> portamentoTimeSec;  // チャンネル設定からコピーした時定数
    std::vector<PerSourceVoiceState> sourceState;

    size_t size() const;
    bool empty() const;
    void reserve(size_t n);
    void clear();
    void AddVoice(const InstrumentSoundConfig& cfg, const MIDIEvent& e, int sampleRate);
    void MarkNoteOff(int channel, int noteNumber, int noteInstanceID, bool holdBySustain);
    void ReleaseSustained(int channel);
    size_t CleanupPending(std::vector<uint8_t>& keepScratch);
};

enum class CommonShaperKind
{
    None,
    BiquadFilter
};

// source render -> common shaper -> modulation apply の受け渡し最小単位。
// 方式追加時はこの構造を介して段階的に共通レイヤーへ接続する。
struct SourceRenderFrame
{
    double sample = 0.0;
    double stereoOffsetL = 0.0;
    double stereoOffsetR = 0.0;
    double ampMul = 1.0;
    double sourceGain = 1.0;
    double shaperCutoffHz = 0.0;
    double shaperResonanceMul = 1.0;
    double shaperDrive = 0.0;
    double shaperDriveNorm = 1.0;
    double shaperFilterDrive = 0.0;
    CommonShaperKind shaperKind = CommonShaperKind::None;
};

struct StereoFrame
{
    double left = 0.0;
    double right = 0.0;
};

class RenderFrameSink
{
public:
    virtual ~RenderFrameSink() = default;
    virtual int Length() const = 0;
    virtual int SampleRate() const = 0;
    virtual void Begin() = 0;
    virtual bool WriteFrame(int sampleIndex, StereoFrame frame) = 0;
    virtual bool WriteFrames(int sampleIndex, const StereoFrame* frames, int frameCount)
    {
        if (frames == nullptr && frameCount > 0)
        {
            return false;
        }
        for (int offset = 0; offset < frameCount; offset++)
        {
            if (!WriteFrame(sampleIndex + offset, frames[offset]))
            {
                return false;
            }
        }
        return true;
    }
    virtual bool SkipSilentFrames(int sampleIndex, int frameCount)
    {
        for (int offset = 0; offset < frameCount; offset++)
        {
            if (!WriteFrame(sampleIndex + offset, StereoFrame{}))
            {
                return false;
            }
        }
        return true;
    }
    virtual bool IsCanceled() const { return false; }
};

class RenderWorkerPool
{
public:
    explicit RenderWorkerPool(size_t workerCount);
    ~RenderWorkerPool();

    RenderWorkerPool(const RenderWorkerPool&) = delete;
    RenderWorkerPool& operator=(const RenderWorkerPool&) = delete;

    size_t WorkerCount() const noexcept { return workers_.size(); }
    bool Run(size_t jobCount, const std::function<void(size_t)>& job);
    bool RunWithCaller(
        size_t jobCount,
        const std::function<void(size_t)>& job,
        const std::function<void()>& callerJob);

private:
    void WorkerLoop();

    std::vector<std::thread> workers_{};
    std::mutex mutex_{};
    std::condition_variable workCv_{};
    std::condition_variable doneCv_{};
    std::function<void(size_t)> job_{};
    size_t jobCount_ = 0;
    size_t nextJob_ = 0;
    size_t completedJobs_ = 0;
    size_t generation_ = 0;
    bool stopping_ = false;
    std::exception_ptr exception_{};
};

struct RenderState
{
    // RenderMIDIEvents の 1 実行スコープで共有される可変状態。
    Voice voices;
    size_t eventIndex = 0;
    size_t pendingRemoveCount = 0;
    std::array<double, 16> channelCc7{};
    std::array<double, 16> channelCc11{};
    std::array<double, 16> channelCcGain{};
    std::array<double, 16> channelPitch{};
    std::array<double, 16> channelModwheel{};
    std::array<double, 16> channelPressure{};
    std::array<std::array<double, 128>, 16> channelPolyPressure{};
    std::array<bool, 16> channelHasPolyPressure{};
    std::array<bool, 16> channelSustain{};
    std::array<double, 16> channelBrightness{};
    std::array<double, 16> channelResonance{};
    std::array<ChannelAdsrOffset, 16> channelAdsrOffset{};
    std::array<double, 16> channelAttackScale{};
    std::array<double, 16> channelDecayScale{};
    std::array<double, 16> channelSustainAdd{};
    std::array<double, 16> channelReleaseScale{};
    std::array<double, 16> channelBrightnessCutoffScale{};
    std::array<double, 16> channelResonanceScale{};
    std::array<double, 16> channelPortamentoTimeSec{};
    std::array<bool, 16> channelPortamentoOn{};
    std::array<double, 16> channelMixGainL{};
    std::array<double, 16> channelMixGainR{};
    std::array<DrumBusRuntimeState, 16> drumBusState{};
    std::array<bool, 16> channelMute{};
    std::array<bool, 16> channelSolo{};
    std::array<bool, 16> channelRenderable{};
    std::array<std::vector<size_t>, 16> activeVoiceIndicesByChannel{};
    std::array<std::array<std::vector<size_t>, config::kSourceKindCount>, 16> activeVoiceIndicesByChannelSource{};
    std::array<std::vector<int>, 16> activeSourceKindsByChannel{};
    std::vector<StereoFrame> renderBlockFrames{};
    std::vector<int> renderActiveChannels{};
    std::array<std::vector<StereoFrame>, 16> renderChannelBlockFrames{};
    std::unique_ptr<RenderWorkerPool> renderWorkerPool{};
    bool renderParallelDisabled = false;
    std::array<double, 16> channelPitchBendNorm{};
    std::array<double, 16> channelPitchBendRangeSemis{};
    std::array<int, 16> channelRpnMsb{};
    std::array<int, 16> channelRpnLsb{};
    MasterEffectConfig effects{};
    std::vector<int> tempoChangeSamples{};
    std::vector<double> tempoChangeBpms{};
    size_t tempoChangeIndex = 0;
    double currentBpm = 120.0;
    std::vector<double> delayBufferL{};
    std::vector<double> delayBufferR{};
    size_t delayWrite = 0;
    std::vector<double> reverbCombL[4];
    std::vector<double> reverbCombR[4];
    std::array<size_t, 4> reverbCombWrite{};
    std::vector<double> reverbAllpassL[2];
    std::vector<double> reverbAllpassR[2];
    std::array<size_t, 2> reverbAllpassWrite{};
    std::vector<double> chorusBufferL{};
    std::vector<double> chorusBufferR{};
    size_t chorusWrite = 0;
    double chorusPhase = 0.0;
    std::vector<double> flangerBufferL{};
    std::vector<double> flangerBufferR{};
    size_t flangerWrite = 0;
    double flangerPhase = 0.0;
    int sampleRateReduceCounter = 0;
    double sampleRateReduceHoldL = 0.0;
    double sampleRateReduceHoldR = 0.0;
    bool hasAnySolo = false;
    std::vector<uint8_t> cleanupKeepScratch{};
    std::vector<size_t> activeVoiceIndices{};
    bool activeVoiceIndicesDirty = true;
};

inline void MarkActiveVoiceIndicesDirty(RenderState& state)
{
    state.activeVoiceIndicesDirty = true;
}

inline void RebuildActiveVoiceIndices(RenderState& state)
{
    state.activeVoiceIndices.clear();
    state.activeVoiceIndices.reserve(state.voices.size());
    for (auto& bucket : state.activeVoiceIndicesByChannel)
    {
        bucket.clear();
    }
    for (auto& channelBuckets : state.activeVoiceIndicesByChannelSource)
    {
        for (auto& bucket : channelBuckets)
        {
            bucket.clear();
        }
    }
    for (auto& sourceKinds : state.activeSourceKindsByChannel)
    {
        sourceKinds.clear();
    }
    for (size_t i = 0; i < state.voices.size(); i++)
    {
        if (state.voices.pendingRemove[i] == 0 && state.voices.env[i].stage != ADSRStage::Off)
        {
            state.activeVoiceIndices.push_back(i);
            const int channel = state.voices.channelIndex[i];
            const int ch = (channel >= 0 && channel < 16) ? channel : 0;
            const int sourceKind = std::clamp<int>(state.voices.runtimeSourceKind[i], 0, config::kSourceKindCount - 1);
            state.activeVoiceIndicesByChannel[ch].push_back(i);
            auto& sourceBucket = state.activeVoiceIndicesByChannelSource[ch][sourceKind];
            if (sourceBucket.empty())
            {
                state.activeSourceKindsByChannel[ch].push_back(sourceKind);
            }
            sourceBucket.push_back(i);
        }
    }
    state.activeVoiceIndicesDirty = false;
}

inline int ClampChannel(int channel)
{
    return (channel >= 0 && channel < 16) ? channel : 0;
}

inline double RenderTimeScaleFromOffset(double offset)
{
    return std::exp2(std::clamp(offset, -1.0, 1.0) * 2.0);
}

inline double RenderCutoffScaleFromBrightness(double brightness)
{
    return std::exp2((std::clamp(brightness, 0.0, 1.0) - 0.5) * 4.0);
}

inline double RenderResonanceScaleFromCc(double resonance)
{
    return std::exp2((std::clamp(resonance, 0.0, 1.0) - 0.5) * 2.0);
}

inline void RecomputeChannelCcGain(RenderState& state, int ch)
{
    ch = ClampChannel(ch);
    state.channelCcGain[ch] = state.channelCc7[ch] * state.channelCc11[ch];
}

inline void RecomputeChannelAdsrCache(RenderState& state, int ch)
{
    ch = ClampChannel(ch);
    const ChannelAdsrOffset& offset = state.channelAdsrOffset[ch];
    state.channelAttackScale[ch] = RenderTimeScaleFromOffset(offset.attack);
    state.channelDecayScale[ch] = RenderTimeScaleFromOffset(offset.decay);
    state.channelSustainAdd[ch] = offset.sustain * 0.5;
    state.channelReleaseScale[ch] = RenderTimeScaleFromOffset(offset.release);
}

inline void RecomputeChannelToneCache(RenderState& state, int ch)
{
    ch = ClampChannel(ch);
    state.channelBrightnessCutoffScale[ch] = RenderCutoffScaleFromBrightness(state.channelBrightness[ch]);
    state.channelResonanceScale[ch] = RenderResonanceScaleFromCc(state.channelResonance[ch]);
}

void ProcessEventsAtSample(const std::vector<MIDIEvent>& events,
    int sampleIndex,
    const std::array<InstrumentSoundConfig, 16>& soundSlots,
    int sampleRate,
    RenderState& state);

void RenderVoicesBlock(RenderState& state, const SoundData& sound, int frameCount, std::vector<StereoFrame>& outFrames);
