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
    struct ArpeggioConfig
    {
        bool enabled = false;
        double rateHz = 8.0;
        int steps = 1;
        std::array<int, 8> semitones{};
    };

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
    // Square 波のパルス幅（0.05..0.95）。
    double pulseWidth = 0.5;
    bool hardSyncEnabled = false;
    double hardSyncRatio = 2.0;
    bool ringModEnabled = false;
    double ringModRatio = 2.0;
    double ringModMix = 1.0;
    // 共通Filterレイヤー（Waveform向け初期接続）。
    FilterMode filterMode = FilterMode::Bypass;
    double filterCutoffHz = 8000.0;
    double filterResonance = 0.707;
    // キートラック量（0.0=なし, 1.0=フルトラック）。基準ノートは C4(60)。
    double filterKeytrack = 0.0;
    // ウェーブシェーパーのドライブ量（0.0=バイパス, 1.0=最大クリップ）。
    double drive = 0.0;
    ArpeggioConfig arpeggio{};
    // 共通Smoothingレイヤー（amp/pitch/filterCutoff）。
    SmoothingConfig smoothing{};
    // 共通Modulationレイヤー（LFO/Env2/Matrix）。
    ModulationConfig modulation{};
};

// アナログ模倣発振方式の設定集合。
// WaveformConfig と同等のオシレータ基盤に、オシレータドリフトを加えた方式。
// smoothing は waveform と同一契約で適用する。
struct AnalogConfig
{
    using ArpeggioConfig = WaveformConfig::ArpeggioConfig;

    struct SmoothingConfig
    {
        bool enabled = true;
        bool pitchEnabled = false;
        double ampTimeMs = 4.0;
        double pitchTimeMs = 2.0;
        double filterCutoffTimeMs = 8.0;
    };

    WaveType wave;
    int unisonVoices = 1;
    double unisonDetuneCents = 0.0;
    double unisonSpread = 0.0;
    double subOscLevel = 0.0;
    double pulseWidth = 0.5;
    bool hardSyncEnabled = false;
    double hardSyncRatio = 2.0;
    bool ringModEnabled = false;
    double ringModRatio = 2.0;
    double ringModMix = 1.0;
    FilterMode filterMode = FilterMode::Bypass;
    double filterCutoffHz = 8000.0;
    double filterResonance = 0.707;
    double filterKeytrack = 0.0;
    double drive = 0.0;
    // アナログ固有: ボイスごとのピッチドリフト。
    double driftDepthCents = 0.0;
    double driftRateHz = 0.3;
    ArpeggioConfig arpeggio{};
    SmoothingConfig smoothing{};
    ModulationConfig modulation{};
};

// ノイズ発振方式の最小設定。
struct NoiseConfig
{
    // ノイズ音源種別。
    // smoothing は非対応（契約上 waveform 専用）。
    NoiseType noise;
    // 共通Filterレイヤー（Waveform/Fm/Analog と同一契約）。
    FilterMode filterMode = FilterMode::Bypass;
    double filterCutoffHz = 8000.0;
    double filterResonance = 0.707;
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
    // オペレータ出力レベルに掛ける個別エンベロープ。
    ModEnvelopeConfig levelEnv{};
    // オペレータ変調深さに掛ける個別エンベロープ。
    ModEnvelopeConfig indexEnv{};
};

// FM 発振方式の設定集合（4オペレータ）。
// algorithm によってオペレータ間の接続トポロジが変わる。
// smoothing は非対応（契約上 waveform 専用）。
struct FmConfig
{
    // 接続アルゴリズム（0-7）。
    //   0: ops[0]→ops[1]（1変調→1キャリア。ops[2]/ops[3] は無視）
    //   1: [ops[0]→ops[1]] + [ops[2]→ops[3]]（2ペア並列）
    //   2: ops[0]→[ops[1]+ops[2]+ops[3]]（1変調→3キャリア）
    //   3: ops[0]→ops[1]→ops[2]→ops[3]（チェーン）
    //   4: [ops[0]→ops[1]] + [ops[2]→ops[3]]（2ペア並列キャリア）
    //   5: ops[0]→[ops[1]+ops[2]+ops[3]]（3並列キャリア）
    //   6: [ops[0]→ops[1]] + ops[2] + ops[3]（1ペア + 2独立）
    //   7: ops[0] + ops[1] + ops[2] + ops[3]（全独立キャリア）
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
    Hat,
    Tom,
    Rim,
    Clap,
    Crash,
    Ride
};

// 単発ドラム音源の設定。
// type別に使う項目が異なり、0/負値は「内部既定値を使う」意味を持つ。
struct DrumConfig
{
    // DrumType ごとに参照する項目が異なるため、未使用値は 0/負値で未指定を表す。
    DrumType type = DrumType::None;
    double gain = 1.0;
    double bodyFreq = 0.0;
    double bodyLevel = 0.0;
    double bodyDecaySec = 0.0;
    double pitchStart = 0.0;
    double pitchDecaySec = 0.0;
    double transientLevel = 0.0;
    double transientDecaySec = 0.0;
    double noiseLevel = 0.0;
    double snapLevel = 0.0;
    double snapDecaySec = 0.0;
    double metalLevel = 0.0;
    double airLevel = 0.0;
    double decaySec = 0.0;
    double hpCut = 0.0;
    double lpCut = 0.0;
    double drive = 0.0;
    int noiseColor = 0;
    double velocityToTone = 0.0;
    double velocityToDecay = 0.0;
    double humanizePitchCents = 0.0;
    double humanizeDecayPct = 0.0;
    // one-shot アタック保護のため smoothing は非対応（契約上 waveform 専用）。
};

// DrumKit 用の note(0..127) マップ。
// 各ノートは DrumConfig を持ち、None で未割り当てを表す。
struct DrumKitConfig
{
    // GM想定の note(0..127) -> DrumConfig マップ。
    std::array<DrumConfig, 128> map;
};

using SourceConfig = std::variant<WaveformConfig, AnalogConfig, NoiseConfig, FmConfig, DrumConfig, DrumKitConfig, PsgConfig>;

enum class AttackLayerType
{
    Pick,
    Brass,
    Metal
};

struct AttackLayerConfig
{
    bool enabled = false;
    AttackLayerType type = AttackLayerType::Pick;
    double level = 0.0;
    double decaySec = 0.035;
    double brightness = 0.5;
    double bodyMix = 0.5;
    double pitchOffsetSemis = 0.0;
    double drive = 0.0;
};

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
    // NoteOn直後だけ鳴る共通アタック補助レイヤー。
    AttackLayerConfig attackLayer{};
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

struct ReverbEffectConfig
{
    bool enabled = false;
    double mix = 0.0;
    double roomSize = 0.45;
    double damping = 0.30;
};

struct DelayEffectConfig
{
    bool enabled = false;
    double mix = 0.0;
    double timeSec = 0.25;
    double feedback = 0.25;
    bool tempoSync = false;
    // 1.0=4分音符, 0.5=8分音符。
    double syncBeats = 1.0;
};

struct ChorusEffectConfig
{
    bool enabled = false;
    double mix = 0.0;
    double baseDelayMs = 14.0;
    double depthMs = 6.0;
    double rateHz = 0.35;
    double feedback = 0.08;
};

struct FlangerEffectConfig
{
    bool enabled = false;
    double mix = 0.0;
    double baseDelayMs = 1.5;
    double depthMs = 1.0;
    double rateHz = 0.25;
    double feedback = 0.15;
};

struct BitCrusherEffectConfig
{
    // 1..16（16でバイパス）
    int bits = 16;
};

struct SampleRateReducerEffectConfig
{
    // 0..1（1.0でバイパス）
    double ratio = 1.0;
};

struct MasterEffectConfig
{
    ReverbEffectConfig reverb{};
    DelayEffectConfig delay{};
    ChorusEffectConfig chorus{};
    FlangerEffectConfig flanger{};
    BitCrusherEffectConfig bitCrusher{};
    SampleRateReducerEffectConfig sampleRateReducer{};
};

void RenderMIDIEvents(
    SoundData& sound,
    const std::vector<MIDIEvent>& events,
    const std::array<ChannelConfig, 16>& channelConfigs,
    const std::array<ChannelMixState, 16>& channelMixStates,
    const MasterEffectConfig& effects = MasterEffectConfig{},
    const std::vector<TempoEvent>* tempoEvents = nullptr,
    int ticksPerQuarter = 0,
    double renderStartSec = 0.0,
    // shouldCancel が true を返した時点でレンダを中断する。
    // canceled が null でない場合は、中断時のみ true を書き戻す。
    const std::function<bool()>& shouldCancel = {},
    bool* canceled = nullptr);
