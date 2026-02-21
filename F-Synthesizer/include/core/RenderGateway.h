#pragma once

#include <array>
#include <functional>
#include <vector>

#include "AppCore.h"
#include "Sequencer.h"

// app 層から core(SynthEngine) への実行境界。
// 呼び出し点を固定して依存方向を保つ。
void RenderWithEngine(
    SoundData& sound,
    const std::vector<MIDIEvent>& events,
    const std::array<ChannelConfig, 16>& channelConfigs,
    const std::array<ChannelMixState, 16>& channelMixStates,
    const std::function<bool()>& shouldCancel,
    bool* canceled);
