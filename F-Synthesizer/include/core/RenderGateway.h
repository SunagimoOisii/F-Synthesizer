#pragma once

#include <array>
#include <functional>
#include <vector>

#include "AppCore.h"
#include "Sequencer.h"

void RenderWithEngine(
    SoundData& sound,
    const std::vector<MIDIEvent>& events,
    const std::array<ChannelConfig, 16>& channelConfigs,
    const std::array<ChannelMixState, 16>& channelMixStates,
    const std::function<bool()>& shouldCancel,
    bool* canceled);
