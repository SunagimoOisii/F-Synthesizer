#pragma once

#include <array>
#include <functional>
#include <variant>
#include <vector>

#include "SynthEngine/Filter.h"
#include "AudioBuffer.h"
#include "Envelope.h"
#include "Sequencer.h"

struct WaveformConfig
{
    // 位相サンプルで生成する基本波形。
    WaveType wave;
    // Unison voice 数（1 で無効）。
    int unisonVoices = 1;
    // Unison の最大デチューン幅（cent）。
    double unisonDetuneCents = 0.0;
    // Unison ボイス間の位相拡散量（0..1、モノラルでは位相分散として扱う）。
    double unisonSpread = 0.0;
    // Sub-osc (-1 octave) の混合量。
    double subOscLevel = 0.0;
    // 共通Filterレイヤー（Waveform向け初期接続）。
    FilterMode filterMode = FilterMode::LowPass;
    double filterCutoffHz = 8000.0;
    double filterResonance = 0.707;
};

struct NoiseConfig
{
    // ノイズ音源種別。
    NoiseType noise;
};

struct FmConfig
{
    // 2オペレータFM用の最小パラメータ集合。
    WaveType carrierWave;
    WaveType modWave;
    double carrierRatio;
    double modRatio;
    double index;
    double outLevel;
};

enum class DrumType
{
    None,
    Kick,
    Snare,
    Hat
};

struct DrumConfig
{
    // DrumType ごとに参照する項目が異なるため、未使用値は 0/負値で未指定を表す。
    DrumType type = DrumType::None;
    double gain = 1.0;
    double baseFreq = 0.0;
    double pitchDrop = 0.0;
    double pitchDecaySec = 0.0;
    double toneFreq = 0.0;
    double toneLevel = 0.0;
    double noiseLevel = 0.0;
    double hpCut = 0.0;
    double lpCut = 0.0;
    int toneWave = -1;
    int noiseType = -1;
};

struct DrumKitConfig
{
    // GM想定の note(0..127) -> DrumConfig マップ。
    std::array<DrumConfig, 128> map;
};

using SourceConfig = std::variant<WaveformConfig, NoiseConfig, FmConfig, DrumConfig, DrumKitConfig>;

struct Voice
{
    // 旧AoS互換の Voice 定義。実レンダは SoA 側を使用する。
    // 識別, 状態
    SourceConfig source;
    int noteNumber;
    int velocity;
    int channel;
    int channelIndex;
    bool released;
    bool pendingRemove;

    //レベル, エンベロープ
    double amp;
    double attackSec;
    double decaySec;
    double sustainLevel;
    double releaseSec;
    ADSRState env;

    //基本波形位相
    double phase;
    double phaseInc;

    //FM パラメータ
    double fmCarrierPhase;
    double fmModPhase;

    //Drum パラメータ
    double drumTime;
    double drumBaseFreq;
    double drumPitchDrop;
    double drumPitchDecaySec;
    double drumNoisePrev;
    double drumHpPrev;
    double drumHpAlpha;
    double drumLpPrev;
    double drumLpAlpha;
};

struct ChannelConfig
{
    // 1チャンネルの合成設定（音源 + ADSR + 振幅）
    // 音源
    SourceConfig source;

    //レベル, エンベロープ
    double amp;
    double attackSec;
    double decaySec;
    double sustainLevel;
    double releaseSec;
};

struct ChannelMixState
{
    // GUI/CLI 共通のミックス状態。solo は hasAnySolo 判定と組み合わせて使う。
    bool mute = false;
    bool solo = false;
    double level = 1.0;
    double pan = 0.0;
    double gain = 1.0;
};

void RenderMIDIEvents(
    SoundData& sound,
    const std::vector<MIDIEvent>& events,
    const std::array<ChannelConfig, 16>& channelConfigs,
    const std::array<ChannelMixState, 16>& channelMixStates,
    const std::function<bool()>& shouldCancel = {},
    bool* canceled = nullptr);
