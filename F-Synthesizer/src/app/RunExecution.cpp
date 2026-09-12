#include "RunInternal.h"

#include <algorithm>
#include <cmath>
#include <sstream>

#include "core/RenderConfigBuilder.h"
#include "core/RenderGateway.h"
#include "io/PlatformPaths.h"
#include "project/ProjectModel.h"

namespace app::run
{
namespace
{
bool PrepareRenderMIDI(const ProjectModel& project, const RenderOptions& options,
    const RenderRuntimeOverrides& overrides, IRunObserver* observer, MIDIBuildOutput& midi)
{
    std::string error;
    if (!BuildMIDIPipeline(project.midiPath, project.targetChannel, project.sampleRate,
        options.startSec, options.durationSec, overrides.noteTicks.get(), overrides.ticksPerQuarter, midi, error))
    {
        LogLine(observer, error == "no note events found" ? "No note events found." :
            "Failed to load MIDI: " + PathToUtf8(project.midiPath));
        return false;
    }
    if (midi.events.empty()) { LogLine(observer, "No note events found."); return false; }
    LogMIDITickSummary(observer, midi.ticks, midi.tempoEvents, midi.ticksPerQuarter, midi.stats);
    LogSampleEventSummary(observer, midi.events);
    return true;
}

int RenderLength(const ProjectModel& project, const RenderOptions& options, int lastSample)
{
    const bool preview = options.mode == RunMode::Preview;
    const int release = static_cast<int>(project.extraReleaseSec * project.sampleRate);
    int needed = lastSample + release + 1;
    if (preview && options.durationSec >= 0)
        needed = std::min(needed, static_cast<int>(options.durationSec * project.sampleRate) + release + 1);
    const int initial = project.initialSeconds * project.sampleRate;
    return needed > initial || (preview && needed > 0) ? needed : initial;
}

int RunRenderCommon(
    const ProjectModel& project,
    const RenderOptions& options,
    const RenderRuntimeOverrides& overrides,
    IRunObserver* observer,
    SoundData* renderedSound,
    bool saveOutput)
{
    const bool previewMode = (options.mode == RunMode::Preview);

    if (options.writeWAV)
    {
        std::string dirErr;
        if (!EnsureDirectoryForFile(project.wavPath, dirErr))
        {
            LogLine(observer, dirErr);
            return 1;
        }
    }

    std::error_code cwdEc;
    const std::filesystem::path cwd = std::filesystem::current_path(cwdEc);
    if (cwdEc)
    {
        LogLine(observer, "[IO] op=current working directory cause=\"" + cwdEc.message() + "\"");
    }
    else
    {
        LogLine(observer, "Working Directory: " + PathToUtf8(cwd));
    }
    LogLine(observer, "MIDI Path: " + PathToUtf8(project.midiPath));
    LogLine(observer, "Output Path: " + PathToUtf8(project.wavPath));
    LogLine(observer, std::string("Run Mode: ") + (previewMode ? "preview" : "export"));

    MIDIBuildOutput midiOut{};
    if (!PrepareRenderMIDI(project, options, overrides, observer, midiOut)) return 1;
    const auto& events = midiOut.events;

    const ResolvedRenderConfigInputs renderInputs = ResolveRenderConfigInputs(project);

    int soundLength = RenderLength(project, options, events.back().sample);
    SoundData sound(soundLength, project.bits, project.sampleRate, 2);

    {
        std::ostringstream oss;
        oss << "Events: " << events.size()
            << ", FirstSample: " << events.front().sample
            << ", LastSample: " << events.back().sample
            << ", Length: " << sound.length;
        LogLine(observer, oss.str());
    }

    const RenderConfig renderConfig = BuildRenderConfig(project, options, events, midiOut, renderInputs);

    bool canceled = false;
    const bool canCancel = options.allowCancel && observer != nullptr;
    if (canCancel)
    {
        auto shouldCancelObserver = [&]() -> bool { return observer->ShouldCancel(); };
        RenderWithEngine(sound, renderConfig, shouldCancelObserver, &canceled);
    }
    else
    {
        auto neverCancel = []() -> bool { return false; };
        RenderWithEngine(sound, renderConfig, neverCancel, &canceled);
    }
    if (canceled)
    {
        LogLine(observer, "[Run] Canceled by request.");
        return 2;
    }

    LogRenderStats(observer, sound);

    if (renderedSound != nullptr)
    {
        *renderedSound = sound;
    }

    if (!saveOutput)
    {
        LogLine(observer, "Preview render completed (memory only, no WAV write).");
        return 0;
    }
    return SaveRunOutput(project, options, sound, observer);
}
} // namespace

int RunExportRender(
    const ProjectModel& project,
    const RenderOptions& options,
    const RenderRuntimeOverrides& overrides,
    IRunObserver* observer,
    SoundData* renderedSound)
{
    return RunRenderCommon(project, options, overrides, observer, renderedSound, true);
}

int RunPreviewRender(
    const ProjectModel& project,
    const RenderOptions& options,
    const RenderRuntimeOverrides& overrides,
    IRunObserver* observer,
    SoundData* renderedSound)
{
    RenderOptions previewOptions = options;
    previewOptions.mode = RunMode::Preview;
    previewOptions.writeWAV = false;
    return RunRenderCommon(project, previewOptions, overrides, observer, renderedSound, false);
}

int RunPreviewStreamingInternal(
    const ProjectModel& project,
    const RenderOptions& options,
    const RenderRuntimeOverrides& overrides,
    IRunObserver* observer,
    IPreviewStreamSink& streamSink,
    bool loop)
{
    RenderOptions previewOptions = options;
    previewOptions.mode = RunMode::Preview;
    previewOptions.writeWAV = false;

    LogLine(observer, "Run Mode: preview streaming");

    MIDIBuildOutput midiOut{};
    if (!PrepareRenderMIDI(project, previewOptions, overrides, observer, midiOut)) return 1;
    const auto& events = midiOut.events;

    const ResolvedRenderConfigInputs renderInputs = ResolveRenderConfigInputs(project);

    int soundLength = RenderLength(project, previewOptions, events.back().sample);
    // A selected loop spans exactly the selected beats, without a release gap.
    if (loop && previewOptions.durationSec > 0.0)
        soundLength = std::max(1, static_cast<int>(std::lround(previewOptions.durationSec * project.sampleRate)));

    if (!streamSink.Begin(project.sampleRate, 2, soundLength, loop))
    {
        LogLine(observer, "[Preview] streaming sink failed to start.");
        return 1;
    }

    const RenderConfig renderConfig = BuildRenderConfig(project, previewOptions, events, midiOut, renderInputs);

    bool canceled = false;
    auto shouldCancelObserver = [&]() -> bool {
        return previewOptions.allowCancel && observer != nullptr && observer->ShouldCancel();
    };
    int skipFrames = static_cast<int>(std::max(0.0, previewOptions.previewSkipSec) * project.sampleRate);
    auto onFrames = [&](int, const double* interleavedStereo, int frameCount) -> bool {
        if (shouldCancelObserver())
        {
            return false;
        }
        const int skip = std::min(skipFrames, frameCount);
        skipFrames -= skip;
        return skip == frameCount || streamSink.WriteFrames(interleavedStereo + skip * 2, frameCount - skip);
    };
    do
    {
        RenderWithEngineFrameBlocks(
            soundLength, project.sampleRate, renderConfig, onFrames,
            shouldCancelObserver, &canceled, overrides.liveSettings);
    } while (loop && !canceled && !shouldCancelObserver());

    streamSink.Complete(canceled);
    if (canceled)
    {
        LogLine(observer, "[Run] Canceled by request.");
        return 2;
    }
    LogLine(observer, "Preview streaming render completed.");
    return 0;
}

int RunMain(
    const ProjectModel& project,
    const RenderOptions& options,
    const RenderRuntimeOverrides& overrides,
    IRunObserver* observer,
    SoundData* renderedSound)
{
    if (options.mode == RunMode::Preview)
    {
        return RunPreviewRender(project, options, overrides, observer, renderedSound);
    }
    return RunExportRender(project, options, overrides, observer, renderedSound);
}
} // namespace app::run
