#pragma once

#include <array>
#include <atomic>
#include <memory>
#include "SynthEngine/InstrumentSoundConfig.h"
#include "SynthEngine/EffectsConfig.h"

struct LiveRenderSettings
{
    std::array<InstrumentSoundConfig, 16> sounds{};
    std::array<ChannelMixState, 16> mixes{};
    MasterEffectConfig effects{};
};

// GUI publishes immutable values; only the producer thread reads this mailbox.
using LiveRenderMailbox = std::atomic<std::shared_ptr<const LiveRenderSettings>>;
