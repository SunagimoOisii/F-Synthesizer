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

struct Voice
{
    SynthMode mode;
    WaveType type;
    int noteNumber;
    int velocity;
    int channel;
    double amp;
    double attackSec;
    double decaySec;
    double sustainLevel;
    double releaseSec;
    double phase;
    double fmCarrierPhase;
    double fmModPhase;
    double fmCarrierRatio;
    double fmModRatio;
    double fmIndex;
    double fmOutLevel;
    ADSRState env;
    bool released;
};

struct ChannelConfig
{
    SynthMode mode;
    WaveType type;
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

void RenderMidiEvents(
    SoundData& sound,
    const std::vector<MidiEvent>& events,
    const std::array<ChannelConfig, 16>& channelConfigs);
