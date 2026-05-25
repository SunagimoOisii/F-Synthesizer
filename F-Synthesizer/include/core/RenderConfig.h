#pragma once

#include <array>
#include <vector>

#include "midi/MIDIParser.h"
#include "midi/Sequencer.h"
#include "SynthEngine/InstrumentSoundConfig.h"
#include "SynthEngine/EffectsConfig.h"

// SynthEngine へ渡す実行用モデル。
// 保存形式やGUI状態ではなく、ProjectModel から展開された1回のrender入力だけを束ねる。
struct RenderConfig
{
    const std::vector<MIDIEvent>& events;
    const std::vector<TempoEvent>& tempoEvents;
    int ticksPerQuarter = 0;
    double renderStartSec = 0.0;
    const std::array<InstrumentSoundConfig, 16>& soundSlots;
    const std::array<ChannelMixState, 16>& channelMixStates;
    const MasterEffectConfig& effects;
};
