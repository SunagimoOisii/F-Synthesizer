#pragma once

#include <array>
#include <functional>
#include <variant>
#include <vector>

#include "SynthEngine/Filter.h"
#include "SynthEngine/Modulation.h"
#include "core/AudioBuffer.h"
#include "synth/Envelope.h"
#include "synth/Oscillator.h"
#include "midi/Sequencer.h"

// 波形発振方式の設定集合。
// ConfigLoad / GUI編集 / Renderer の共通入力として使う。
struct WaveformConfig
{
    struct SmoothingConfig
    {
        bool enabled = true;
        bool pitchEnabled = false;
        double ampTimeMs = 4.0;
        double pitchTimeMs = 2.0;
        double filterCutoffTimeMs = 8.0;
    };

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
    FilterMode filterMode = FilterMode::Bypass;
    double filterCutoffHz = 8000.0;
    double filterResonance = 0.707;
    // 共通Smoothingレイヤー（amp/pitch/filterCutoff）。
    SmoothingConfig smoothing{};
    // 共通Modulationレイヤー（LFO/Env2/Matrix）。
    ModulationConfig modulation{};
};

// ノイズ発振方式の最小設定。
struct NoiseConfig
{
    // ノイズ音源種別。
    // smoothing は非対応（契約上 waveform 専用）。
    NoiseType noise;
};

// FM発振方式の設定集合。
// modulation は FM専用 destination(fm.index) を含めて扱う。
struct FmConfig
{
    // 2オペレータFM用の最小パラメータ集合。
    WaveType carrierWave;
    WaveType modWave;
    double carrierRatio;
    double modRatio;
    double index;
    double outLevel;
    FilterMode filterMode = FilterMode::Bypass;
    double filterCutoffHz = 8000.0;
    double filterResonance = 0.707;
    // 共通Modulationレイヤー（Phase1: fm.index / pitchMul / amp をサポート）。
    // smoothing は非対応（契約上 waveform 専用）。
    ModulationConfig modulation{};
};

enum class DrumType
{
    None,
    Kick,
    Snare,
    Hat
};

// 単発ドラム音源の設定。
// type別に使う項目が異なり、0/負値は「内部既定値を使う」意味を持つ。
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
    int toneWave = 0;
    int noiseType = 0;
    // one-shot アタック保護のため smoothing は非対応（契約上 waveform 専用）。
};

// DrumKit 用の note(0..127) マップ。
// 各ノートは DrumConfig を持ち、None で未割り当てを表す。
struct DrumKitConfig
{
    // GM想定の note(0..127) -> DrumConfig マップ。
    std::array<DrumConfig, 128> map;
};

using SourceConfig = std::variant<WaveformConfig, NoiseConfig, FmConfig, DrumConfig, DrumKitConfig>;

// 1チャンネル分の音色設定。
// source + ADSR + amp を1セットで保持する。
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
    // shouldCancel が true を返した時点でレンダを中断する。
    // canceled が null でない場合は、中断時のみ true を書き戻す。
    const std::function<bool()>& shouldCancel = {},
    bool* canceled = nullptr);
