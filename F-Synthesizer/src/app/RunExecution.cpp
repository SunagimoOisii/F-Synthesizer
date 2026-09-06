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
    std::string midiErr;
    if (!BuildMIDIPipeline(
        project.midiPath,
        project.targetChannel,
        project.sampleRate,
        options.startSec,
        options.durationSec,
        overrides.noteTicks.get(),
        overrides.ticksPerQuarter,
        midiOut,
        midiErr))
    {
        if (midiErr == "no note events found")
        {
            LogLine(observer, "No note events found.");
        }
        else
        {
            LogLine(observer, "Failed to load MIDI: " + PathToUtf8(project.midiPath));
        }
        return 1;
    }

    LogMIDITickSummary(observer, midiOut.ticks, midiOut.tempoEvents, midiOut.ticksPerQuarter, midiOut.stats);

    std::vector<MIDIEvent> events = std::move(midiOut.events);
    LogSampleEventSummary(observer, events);

    if (events.empty())
    {
        if (midiErr.empty())
        {
            LogLine(observer, "No note events found.");
        }
        return 1;
    }

    const ResolvedRenderConfigInputs renderInputs = ResolveRenderConfigInputs(project);

    int lastSample = events.back().sample;
    int extraRelease = static_cast<int>(project.extraReleaseSec * project.sampleRate);
    int neededSamples = lastSample + extraRelease + 1;
    if (previewMode && options.durationSec >= 0.0)
    {
        const double durSec = (options.durationSec > 0.0) ? options.durationSec : 0.0;
        const int previewMax = static_cast<int>(durSec * project.sampleRate) + extraRelease + 1;
        if (neededSamples > previewMax)
        {
            neededSamples = previewMax;
        }
    }
    int soundLength = project.initialSeconds * project.sampleRate;
    if (neededSamples > soundLength)
    {
        soundLength = neededSamples;
    }
    else if (previewMode && neededSamples > 0 && neededSamples < soundLength)
    {
        soundLength = neededSamples;
    }
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
    std::string midiErr;
    if (!BuildMIDIPipeline(
        project.midiPath,
        project.targetChannel,
        project.sampleRate,
        previewOptions.startSec,
        previewOptions.durationSec,
        overrides.noteTicks.get(),
        overrides.ticksPerQuarter,
        midiOut,
        midiErr))
    {
        LogLine(observer, (midiErr == "no note events found") ? "No note events found." : "Failed to load MIDI: " + PathToUtf8(project.midiPath));
        return 1;
    }

    std::vector<MIDIEvent> events = std::move(midiOut.events);
    LogSampleEventSummary(observer, events);
    if (events.empty())
    {
        LogLine(observer, "No note events found.");
        return 1;
    }

    const ResolvedRenderConfigInputs renderInputs = ResolveRenderConfigInputs(project);

    int lastSample = events.back().sample;
    int extraRelease = static_cast<int>(project.extraReleaseSec * project.sampleRate);
    int neededSamples = lastSample + extraRelease + 1;
    if (previewOptions.durationSec >= 0.0)
    {
        const double durSec = (previewOptions.durationSec > 0.0) ? previewOptions.durationSec : 0.0;
        const int previewMax = static_cast<int>(durSec * project.sampleRate) + extraRelease + 1;
        neededSamples = std::min(neededSamples, previewMax);
    }
    int soundLength = project.initialSeconds * project.sampleRate;
    if (neededSamples > soundLength)
    {
        soundLength = neededSamples;
    }
    else if (neededSamples > 0 && neededSamples < soundLength)
    {
        soundLength = neededSamples;
    }

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
    auto onFrames = [&](int, const double* interleavedStereo, int frameCount) -> bool {
        if (shouldCancelObserver())
        {
            return false;
        }
        return streamSink.WriteFrames(interleavedStereo, frameCount);
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
