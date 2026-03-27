#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <variant>
#include <vector>

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
    double baseFreq = 0.0;
    double pitchDrop = 1.0;
    double pitchDecaySec = 0.0;
    double noisePrev = 0.0;
    double hpPrev = 0.0;
    double hpAlpha = 0.0;
    double lpPrev = 0.0;
    double lpAlpha = 0.0;
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
    std::vector<ADSRState> env;

    std::vector<double> phase;
    std::vector<double> phaseInc;
    std::vector<double> portamentoPitchHz;  // 現在のスライド中ピッチ（Hz）
    std::vector<double> portamentoTargetHz; // ターゲットピッチ（Hz）
    std::vector<double> portamentoTimeSec;  // チャンネル設定からコピーした時定数
    std::vector<PerSourceVoiceState> sourceState;

    size_t size() const;
    bool empty() const;
    void reserve(size_t n);
    void clear();
    void AddVoice(const ChannelConfig& cfg, const MIDIEvent& e, int sampleRate);
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
    double ampMul = 1.0;
    double sourceGain = 1.0;
    double shaperCutoffHz = 0.0;
    double shaperResonanceMul = 1.0;
    double shaperDrive = 0.0;
    double shaperDriveNorm = 1.0;
    CommonShaperKind shaperKind = CommonShaperKind::None;
};

struct RenderState
{
    // RenderMIDIEvents の 1 実行スコープで共有される可変状態。
    Voice voices;
    size_t eventIndex = 0;
    size_t pendingRemoveCount = 0;
    std::array<double, 16> channelCc7{};
    std::array<double, 16> channelCc11{};
    std::array<double, 16> channelPitch{};
    std::array<double, 16> channelModwheel{};
    std::array<double, 16> channelPressure{};
    std::array<std::array<double, 128>, 16> channelPolyPressure{};
    std::array<bool, 16> channelSustain{};
    std::array<double, 16> channelBrightness{};
    std::array<double, 16> channelResonance{};
    std::array<ChannelAdsrOffset, 16> channelAdsrOffset{};
    std::array<double, 16> channelPortamentoTimeSec{};
    std::array<bool, 16> channelPortamentoOn{};
    std::array<double, 16> channelMixGainL{};
    std::array<double, 16> channelMixGainR{};
    std::array<bool, 16> channelMute{};
    std::array<bool, 16> channelSolo{};
    std::array<bool, 16> channelRenderable{};
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
};

inline int ClampChannel(int channel)
{
    return (channel >= 0 && channel < 16) ? channel : 0;
}

void ProcessEventsAtSample(const std::vector<MIDIEvent>& events,
    int sampleIndex,
    const std::array<ChannelConfig, 16>& channelConfigs,
    int sampleRate,
    RenderState& state);

struct StereoFrame
{
    double left = 0.0;
    double right = 0.0;
};

StereoFrame RenderVoices(RenderState& state, const SoundData& sound);
