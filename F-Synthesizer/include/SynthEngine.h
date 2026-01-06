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

using SourceConfig = std::variant<WaveformConfig, NoiseConfig, FmConfig>;

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

void RenderMIDIEvents(
    SoundData& sound,
    const std::vector<MIDIEvent>& events,
    const std::array<ChannelConfig, 16>& channelConfigs);
