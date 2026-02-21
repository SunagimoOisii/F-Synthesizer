#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include "SynthEngine/SynthEngine.h"

struct RenderState
{
    std::vector<Voice> voices;
    size_t eventIndex = 0;
    size_t pendingRemoveCount = 0;
    std::array<double, 16> channelCc7{};
    std::array<double, 16> channelCc11{};
    std::array<double, 16> channelPitch{};
    std::array<double, 16> channelMixGain{};
    std::array<bool, 16> channelMute{};
    std::array<bool, 16> channelSolo{};
    bool hasAnySolo = false;
};

inline int ClampChannel(int channel)
{
    return (channel >= 0 && channel < 16) ? channel : 0;
}

void ProcessEventsAtSample(const std::vector<MIDIEvent>& events,
    int sampleIndex,
    const std::array<ChannelConfig, 16>& channelConfigs,
    int sampleRate,
    RenderState& state);

double RenderVoices(RenderState& state, const SoundData& sound);

Voice MakeVoiceFromConfig(const ChannelConfig& cfg, const MIDIEvent& e, int sampleRate);
