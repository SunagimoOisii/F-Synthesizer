#pragma once

#include <array>
#include <vector>

#include "midi/MIDIParser.h"
#include "midi/Sequencer.h"
#include "SynthEngine/ChannelConfig.h"
#include "SynthEngine/EffectsConfig.h"

// SynthEngine へ渡す実行用モデル。
// 保存形式やGUI状態ではなく、1回のrenderに必要な入力だけを束ねる。
// AppConfig から既定値を解決した後に作る。ProjectModel 直変換は Phase 3 で扱う。
struct RenderConfig
{
    const std::vector<MIDIEvent>& events;
    const std::vector<TempoEvent>& tempoEvents;
    int ticksPerQuarter = 0;
    double renderStartSec = 0.0;
    const std::array<ChannelConfig, 16>& channelConfigs;
    const std::array<ChannelMixState, 16>& channelMixStates;
    const MasterEffectConfig& effects;
};
