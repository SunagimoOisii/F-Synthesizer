#pragma once

#include <array>
#include <atomic>
#include <memory>
#include "SynthEngine/InstrumentSoundConfig.h"
#include "SynthEngine/EffectsConfig.h"

// One producer, one UI reader. Atomic samples keep concurrent snapshots race-free.
// This tap is on the render thread, never on the device callback.
struct AudioScope
{
    static constexpr size_t capacity = 4096;
    std::atomic<int> channel{0};
    std::atomic<uint64_t> cursor{0};
    std::array<std::atomic<float>, capacity> part{}, mix{};
    void Push(float selected, float full)
    {
        const auto index = cursor.load(std::memory_order_relaxed);
        part[index % capacity].store(selected, std::memory_order_relaxed);
        mix[index % capacity].store(full, std::memory_order_relaxed);
        cursor.store(index + 1, std::memory_order_release);
    }
};

struct LiveRenderSettings
{
    std::array<InstrumentSoundConfig, 16> sounds{};
    std::array<ChannelMixState, 16> mixes{};
    MasterEffectConfig effects{};
    std::shared_ptr<AudioScope> scope;
};

// GUI publishes immutable values; only the producer thread reads this mailbox.
using LiveRenderMailbox = std::atomic<std::shared_ptr<const LiveRenderSettings>>;
