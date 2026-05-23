#pragma once

#include <array>

#include "SynthEngine/SourceConfig.h"

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

enum class BassLayerType
{
    Sub,
    Drive,
    Grit
};

struct BassLayerConfig
{
    bool enabled = false;
    BassLayerType type = BassLayerType::Drive;
    double level = 0.0;
    double subLevel = 0.45;
    double bodyLevel = 0.35;
    double gritLevel = 0.20;
    double cutoffHz = 900.0;
    double drive = 0.0;
    double pitchOffsetSemis = -12.0;
    double velocityToDrive = 0.0;
    double focusHz = 180.0;
    double focusLevel = 0.0;
    double bodySaturation = 0.0;
    double gritTone = 0.5;
    double attackBoost = 0.0;
    double attackDecaySec = 0.045;
};

enum class LeadLayerType
{
    Blade,
    Brass,
    Edge
};

struct LeadLayerConfig
{
    bool enabled = false;
    LeadLayerType type = LeadLayerType::Blade;
    double level = 0.0;
    double edgeLevel = 0.35;
    double bodyLevel = 0.30;
    double detuneCents = 4.0;
    double pitchBendSemis = 0.0;
    double bendDecaySec = 0.045;
    double attackBoost = 0.0;
    double attackDecaySec = 0.035;
    double drive = 0.0;
    double characterLevel = 0.0;
    double characterTone = 0.5;
    double biteLevel = 0.0;
    double biteDecaySec = 0.025;
    double wobbleDepthCents = 0.0;
    double wobbleRateHz = 4.0;
};

struct ChordLayerConfig
{
    bool enabled = false;
    double level = 0.0;
    std::array<int, 4> intervalsSemis{ 0, 7, 12, 0 };
    std::array<double, 4> voiceLevels{ 1.0, 0.72, 0.50, 0.0 };
    double detuneCents = 3.0;
    double spread = 0.25;
    double cutoffHz = 2600.0;
    double drive = 0.0;
};

struct PadLayerConfig
{
    bool enabled = false;
    double level = 0.0;
    double octaveLevel = 0.20;
    double detuneCents = 5.0;
    double spread = 0.35;
    double fadeInSec = 0.12;
    double brightness = 0.35;
    double motionDepth = 0.08;
    double motionRateHz = 0.22;
    double cutoffHz = 1800.0;
    double drive = 0.0;
};

struct PluckLayerConfig
{
    bool enabled = false;
    double level = 0.0;
    double decaySec = 0.18;
    double brightness = 0.55;
    double noiseMix = 0.35;
    double pitchOffsetSemis = 0.0;
    double bodySend = 0.35;
    double drive = 0.0;
};

struct StringLayerConfig
{
    bool enabled = false;
    double level = 0.0;
    double bowLevel = 0.25;
    double detuneCents = 7.0;
    double spread = 0.45;
    double fadeInSec = 0.08;
    double brightness = 0.45;
    double motionDepth = 0.12;
    double motionRateHz = 0.7;
    double bodySend = 0.40;
    double drive = 0.0;
};

struct BodyLayerConfig
{
    enum class Mode
    {
        Harmonic,
        Box,
        Metal
    };

    bool enabled = false;
    Mode mode = Mode::Box;
    double mix = 0.0;
    double size = 0.5;
    double tone = 0.45;
    double damping = 0.35;
    double stereo = 0.25;
    double drive = 0.0;
};

struct ExpressionMapConfig
{
    bool enabled = false;
    double velocityCurve = 1.0;
    double velocityToAmp = 1.0;
    double velocityToBrightness = 0.0;
    double velocityToFmIndex = 0.0;
    double velocityToAttack = 0.0;
    double velocityToBass = 0.0;
    double velocityToLead = 0.0;
    double velocityToChord = 0.0;
    double velocityToPad = 0.0;
    double velocityToPluck = 0.0;
    double velocityToString = 0.0;
    double velocityToBody = 0.0;
    double modWheelToBrightness = 0.0;
    double modWheelToPad = 0.0;
    double modWheelToString = 0.0;
    double pressureToDrive = 0.0;
    double pressureToFilterDrive = 0.0;
    double cc74ToBrightness = 0.0;
    double cc74ToPadBrightness = 0.0;
    double cc74ToStringBrightness = 0.0;
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
    // ベース向けに持続する低域と歪み成分を重ねる共通補助レイヤー。
    BassLayerConfig bassLayer{};
    // 主旋律向けに硬いアタック、薄い倍音、デチューン感を足す共通補助レイヤー。
    LeadLayerConfig leadLayer{};
    // 和音向けに固定voicingの追加音を足す共通補助レイヤー。
    ChordLayerConfig chordLayer{};
    // 背景向けに暗い厚みと揺れを足す共通補助レイヤー。
    PadLayerConfig padLayer{};
    PluckLayerConfig pluckLayer{};
    StringLayerConfig stringLayer{};
    BodyLayerConfig bodyLayer{};
    DrumBusConfig drumBus{};
    // MIDI velocity/CC/pressure を音量以外の音色変化へ写像する共通表情マップ。
    ExpressionMapConfig expressionMap{};
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
