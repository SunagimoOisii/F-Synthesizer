#pragma once

#include <array>
#include <vector>

#include "AudioBuffer.h"
#include "Envelope.h"
#include "Sequencer.h"

enum class SynthMode
{
    Basic,
    FM
};

enum class SourceType
{
    Waveform,
    Noise
};

struct Voice
{
    //識別, 状態
    SynthMode mode;
    SourceType source;
    WaveType type;
    NoiseType noiseType;
    int noteNumber;
    int velocity;
    int channel;
    bool released;

    //レベル, エンベロープ
    double amp;
    double attackSec;
    double decaySec;
    double sustainLevel;
    double releaseSec;
    ADSRState env;

    //基本波形位相
    double phase;

    //FM パラメータ
    double fmCarrierPhase;
    double fmModPhase;
    double fmCarrierRatio;
    double fmModRatio;
    double fmIndex;
    double fmOutLevel;
};

struct ChannelConfig
{
    //種類
    SynthMode mode;
    SourceType source;
    WaveType type;
    NoiseType noiseType;

    //レベル, エンベロープ
    double amp;
    double attackSec;
    double decaySec;
    double sustainLevel;
    double releaseSec;

    //FM パラメータ
    double fmCarrierRatio;
    double fmModRatio;
    double fmIndex;
    double fmOutLevel;
};

void RenderMIDIEvents(
    SoundData& sound,
    const std::vector<MIDIEvent>& events,
    const std::array<ChannelConfig, 16>& channelConfigs);
