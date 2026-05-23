#pragma once

#include <array>
#include <functional>
#include <vector>

#include "SynthEngine/ChannelConfig.h"
#include "SynthEngine/EffectsConfig.h"
#include "core/AudioBuffer.h"
#include "midi/Sequencer.h"

void RenderMIDIEvents(
    SoundData& sound,
    const std::vector<MIDIEvent>& events,
    const std::array<ChannelConfig, 16>& channelConfigs,
    const std::array<ChannelMixState, 16>& channelMixStates,
    const MasterEffectConfig& effects = MasterEffectConfig{},
    const std::vector<TempoEvent>* tempoEvents = nullptr,
    int ticksPerQuarter = 0,
    double renderStartSec = 0.0,
    // shouldCancel が true を返した時点でレンダを中断する。
    // canceled が null でない場合は、中断時のみ true を書き戻す。
    const std::function<bool()>& shouldCancel = {},
    bool* canceled = nullptr);
