#pragma once

#include <array>
#include <variant>
#include <vector>

#include "AudioBuffer.h"
#include "Envelope.h"
#include "Sequencer.h"

struct WaveformConfig
{
    WaveType wave;
};

struct NoiseConfig
{
    NoiseType noise;
};

struct FmConfig
{
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
    std::array<DrumConfig, 128> map;
};

using SourceConfig = std::variant<WaveformConfig, NoiseConfig, FmConfig, DrumConfig, DrumKitConfig>;

struct Voice
{
    //識別, 状態
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
    //音源
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
    const std::array<ChannelMixState, 16>& channelMixStates);
