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
    // キートラック量（0.0=なし, 1.0=フルトラック）。基準ノートは C4(60)。
    double filterKeytrack = 0.0;
    // ウェーブシェーパーのドライブ量（0.0=バイパス, 1.0=最大クリップ）。
    double drive = 0.0;
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

// FM オペレータ1個分の設定。
struct FmOperator
{
    WaveType wave = WaveType::Sine;
    // 基音に対する周波数比（0.0 < ratio <= 32.0）。
    double ratio = 1.0;
    // 出力レベル（0.0..1.0）。キャリアは音量、モジュレータは変調量への寄与。
    double level = 1.0;
    // 変調深さ（0.0..32.0）。モジュレータとして使われるときのみ有効。
    double index = 0.0;
};

// FM 発振方式の設定集合（4オペレータ）。
// algorithm によってオペレータ間の接続トポロジが変わる。
// smoothing は非対応（契約上 waveform 専用）。
struct FmConfig
{
    // 接続アルゴリズム（0-3）。
    //   0: ops[0]→ops[1]（旧2オペ互換。ops[2]/ops[3] は無視）
    //   1: [ops[0]→ops[1]] + [ops[2]→ops[3]]（2ペア並列）
    //   2: ops[0]→[ops[1]+ops[2]+ops[3]]（1変調→3キャリア）
    //   3: ops[0]→ops[1]→ops[2]→ops[3]（チェーン）
    int algorithm = 0;
    // ops[0] の自己フィードバック量（0.0=なし, 1.0=最大）。
    double feedback = 0.0;
    FmOperator ops[4];
    FilterMode filterMode = FilterMode::Bypass;
    double filterCutoffHz = 8000.0;
    double filterResonance = 0.707;
    // ウェーブシェーパーのドライブ量（0.0=バイパス, 1.0=最大クリップ）。
    double drive = 0.0;
    // 共通 Modulation レイヤー（fm.index / pitchMul / amp をサポート）。
    ModulationConfig modulation{};
};

// PSG波形種別。
enum class PsgWaveType
{
    Square,    // デューティ固定 50%
    Pulse,     // デューティ可変（duty パラメータで制御）
    Triangle,
    Noise      // 16bit LFSR 白ノイズ
};

// PSG発振方式の設定集合。
// チップ制約（離散ボリューム・デューティサイクル・ボイス数上限）を再現する。
// smoothing は非対応（契約上 waveform 専用）。
struct PsgConfig
{
    PsgWaveType wave = PsgWaveType::Square;
    // パルス幅（0-7, 1/8刻み。0=12.5%, 4=50%, 7=87.5%）。wave=Pulse のみ有効。
    int duty = 4;
    // 4bit音量量子化ステップ（0-15。15=最大、0=無音）。
    int volumeSteps = 15;
    // ポリフォニー上限（1-8。チップ制約の再現用）。
    int maxVoices = 3;
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

using SourceConfig = std::variant<WaveformConfig, NoiseConfig, FmConfig, DrumConfig, DrumKitConfig, PsgConfig>;

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
    // ポルタメント時定数（0.0=オフ, 秒単位）。0 より大きい場合に前ノートからスライドする。
    double portamentoTimeSec = 0.0;
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
