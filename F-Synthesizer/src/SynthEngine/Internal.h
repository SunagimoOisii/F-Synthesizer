#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "SynthEngine/SynthEngine.h"

struct VoicesSoA
{
    std::vector<SourceConfig> source;
    std::vector<int> noteNumber;
    std::vector<int> velocity;
    std::vector<int> channel;
    std::vector<int> channelIndex;
    std::vector<uint8_t> released;
    std::vector<uint8_t> pendingRemove;

    std::vector<double> amp;
    std::vector<double> attackSec;
    std::vector<double> decaySec;
    std::vector<double> sustainLevel;
    std::vector<double> releaseSec;
    std::vector<ADSRState> env;

    std::vector<double> phase;
    std::vector<double> phaseInc;
    std::vector<double> fmCarrierPhase;
    std::vector<double> fmModPhase;

    std::vector<double> drumTime;
    std::vector<double> drumBaseFreq;
    std::vector<double> drumPitchDrop;
    std::vector<double> drumPitchDecaySec;
    std::vector<double> drumNoisePrev;
    std::vector<double> drumHpPrev;
    std::vector<double> drumHpAlpha;
    std::vector<double> drumLpPrev;
    std::vector<double> drumLpAlpha;

    size_t size() const;
    bool empty() const;
    void reserve(size_t n);
    void clear();
    void AddVoice(const ChannelConfig& cfg, const MIDIEvent& e, int sampleRate);
    void MarkNoteOff(int channel, int noteNumber);
    size_t CleanupPending();
};

struct RenderState
{
    VoicesSoA voices;
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
