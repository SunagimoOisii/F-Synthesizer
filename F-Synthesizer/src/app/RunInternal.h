#pragma once

#include <array>
#include <filesystem>
#include <string>
#include <vector>

#include "AppCore.h"
#include "midi/MIDIPipeline.h"

namespace app::run
{
// app層内部API:
// 公開Run APIを支える実行/保存/統計の分割実装を束ねる。
std::filesystem::path FindProjectRootInternal();

void LogLine(IRunObserver* observer, const std::string& line);

void LogMIDITickSummary(
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
    const ProjectModel& project,
    const RenderOptions& options,
    const SoundData& sound,
    IRunObserver* observer);

int RunMain(
    const ProjectModel& project,
    const RenderOptions& options,
    const RenderRuntimeOverrides& overrides,
    IRunObserver* observer,
    SoundData* renderedSound);

int RunExportRender(
    const ProjectModel& project,
    const RenderOptions& options,
    const RenderRuntimeOverrides& overrides,
    IRunObserver* observer,
    SoundData* renderedSound);

int RunPreviewRender(
    const ProjectModel& project,
    const RenderOptions& options,
    const RenderRuntimeOverrides& overrides,
    IRunObserver* observer,
    SoundData* renderedSound);

int RunPreviewStreamingInternal(
    const ProjectModel& project,
    const RenderOptions& options,
    const RenderRuntimeOverrides& overrides,
    IRunObserver* observer,
    IPreviewStreamSink& streamSink,
    bool loop);
} // namespace app::run

