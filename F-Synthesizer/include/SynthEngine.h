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
    SynthMode mode;
    SourceType source;
    WaveType type;
    NoiseType noiseType;
    int noteNumber;
    int velocity;
    int channel;
    bool released;

    double amp;
    double attackSec;
    double decaySec;
    double sustainLevel;
    double releaseSec;
    ADSRState env;

    double phase;

    double fmCarrierPhase;
    double fmModPhase;
    double fmCarrierRatio;
    double fmModRatio;
    double fmIndex;
    double fmOutLevel;
};

struct ChannelConfig
{
    SynthMode mode;
    SourceType source;
    WaveType type;
    NoiseType noiseType;

    double amp;
    double attackSec;
    double decaySec;
    double sustainLevel;
    double releaseSec;

    double fmCarrierRatio;
    double fmModRatio;
    double fmIndex;
    double fmOutLevel;
};

void RenderMIDIEvents(
    SoundData& sound,
    const std::vector<MIDIEvent>& events,
    const std::array<ChannelConfig, 16>& channelConfigs);
