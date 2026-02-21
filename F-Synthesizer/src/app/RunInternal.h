#pragma once

#include <array>
#include <filesystem>
#include <string>
#include <vector>

#include "AppCore.h"
#include "midi/MidiPipeline.h"

namespace app::run
{
std::filesystem::path FindProjectRootInternal();
AppConfig BuildDefaultConfig();
std::shared_ptr<const std::array<ChannelConfig, 16>> BuildDefaultChannelConfigs();
std::shared_ptr<const std::array<ChannelMixState, 16>> BuildDefaultChannelMixStates();

void LogLine(IRunObserver* observer, const std::string& line);

void LogMidiTickSummary(
    IRunObserver* observer,
    const std::vector<MIDIEventTick>& ticks,
    const std::vector<TempoEvent>& tempoEvents,
    int ticksPerQuarter,
    const MIDIParseStatus& stats);

void LogSampleEventSummary(
    IRunObserver* observer,
    const std::vector<MIDIEvent>& events);

void LogRenderStats(IRunObserver* observer, const SoundData& sound);

int SaveRunOutput(
    const AppConfig& config,
    const RenderOptions& options,
    const SoundData& sound,
    IRunObserver* observer);

int RunMain(
    const AppConfig& config,
    const RenderOptions& options,
    IRunObserver* observer,
    SoundData* renderedSound);
} // namespace app::run
