#include "gui/GUIActions.h"

#include <algorithm>
#include <chrono>
#include <exception>
#include <future>
#include <map>
#include <mutex>
#include <system_error>
#include <vector>

#include "AppCore.h"
#include "gui/GUIActionsInternal.h"
#include "gui/GUIConfigUtils.h"
#include "gui/GUIProjectFacade.h"
#include "gui/GUIRunHelpers.h"
#include "gui/GUIStateModel.h"
#include "gui/PreviewAudio.h"
#include "io/PlatformPaths.h"
#include "project/ProjectModel.h"

namespace
{
std::string BuildUserErrorMessage(const std::string& summary, const std::string& detail)
{
    if (detail.empty())
    {
        return summary;
    }
    return summary + " (" + detail + ")";
}

double SafeBpm(double bpm)
{
    return (bpm > 1e-3) ? bpm : 120.0;
}

double SecondsAtTickForPreview(const std::vector<TempoEvent>& tempoEvents, int ticksPerQuarter, int targetTick)
{
    if (targetTick <= 0 || ticksPerQuarter <= 0)
    {
        return 0.0;
    }

    std::vector<TempoEvent> sorted = tempoEvents;
    std::sort(sorted.begin(), sorted.end(), [](const TempoEvent& a, const TempoEvent& b) {
        return a.tick < b.tick;
    });
    if (sorted.empty() || sorted.front().tick != 0)
    {
        TempoEvent te{};
        te.tick = 0;
        te.bpm = 120.0;
        sorted.insert(sorted.begin(), te);
    }

    double seconds = 0.0;
    int cursorTick = 0;
    double cursorBpm = SafeBpm(sorted.front().bpm);
    size_t idx = 1;
    while (idx < sorted.size() && sorted[idx].tick <= targetTick)
    {
        const int nextTick = sorted[idx].tick;
        const int deltaTick = nextTick - cursorTick;
        const double secPerTick = (60.0 / SafeBpm(cursorBpm)) / static_cast<double>(ticksPerQuarter);
        seconds += secPerTick * static_cast<double>(deltaTick);
        cursorTick = nextTick;
        cursorBpm = SafeBpm(sorted[idx].bpm);
        idx++;
    }
    if (targetTick > cursorTick)
    {
        const int deltaTick = targetTick - cursorTick;
        const double secPerTick = (60.0 / SafeBpm(cursorBpm)) / static_cast<double>(ticksPerQuarter);
        seconds += secPerTick * static_cast<double>(deltaTick);
    }
    return seconds;
}

double PreviewRangeDurationSec(const GUIState& state)
{
    const auto& pr = state.pianoRoll;
    if (!pr.previewRangeEnabled || pr.ticksPerQuarter <= 0)
    {
        return 0.0;
    }
    const int rangeStartTick = (std::min)(pr.previewRangeStartTick, pr.previewRangeEndTick);
    const int rangeEndTick = (std::max)(pr.previewRangeStartTick, pr.previewRangeEndTick);
    if (rangeEndTick <= rangeStartTick)
    {
        return 0.0;
    }
    const double startSec = SecondsAtTickForPreview(pr.tempoEvents, pr.ticksPerQuarter, rangeStartTick);
    const double endSec = SecondsAtTickForPreview(pr.tempoEvents, pr.ticksPerQuarter, rangeEndTick);
    return (std::max)(0.0, endSec - startSec);
}

std::shared_ptr<const std::vector<MIDIEventTick>> BuildOverrideNoteTicksFromPianoRoll(
    const GUIState& state,
    int& outTicksPerQuarter)
{
    outTicksPerQuarter = 0;
    const auto& pr = state.pianoRoll;
    if (pr.hasLoadError || pr.notes.empty() || pr.ticksPerQuarter <= 0)
    {
        return nullptr;
    }
    const std::filesystem::path currentMidiPath = Utf8ToPath(state.midiPath);
    if (currentMidiPath.empty() || pr.loadedMidiPath != currentMidiPath)
    {
        return nullptr;
    }

    auto ticks = std::make_shared<std::vector<MIDIEventTick>>();
    ticks->reserve(pr.notes.size() * 2);

    int order = 0;
    int noteInstanceID = 1;
    for (const auto& n : pr.notes)
    {
        const int startTick = (std::max)(0, n.startTick);
        const int endTick = (std::max)(startTick + 1, n.endTick);
        const int channel = std::clamp(n.channel, 0, 15);
        const int note = std::clamp(n.note, 0, 127);
        const int velocity = std::clamp(n.velocity, 1, 127);

        MIDIEventTick on{};
        on.type = MIDIEventType::Note;
        on.tick = startTick;
        on.noteNumber = note;
        on.velocity = velocity;
        on.channel = channel;
        on.controller = 0;
        on.value = 0;
        on.noteInstanceID = noteInstanceID;
        on.order = order++;
        on.isNoteOn = true;
        ticks->push_back(on);

        MIDIEventTick off{};
        off.type = MIDIEventType::Note;
        off.tick = endTick;
        off.noteNumber = note;
        off.velocity = 0;
        off.channel = channel;
        off.controller = 0;
        off.value = 0;
        off.noteInstanceID = noteInstanceID;
        off.order = order++;
        off.isNoteOn = false;
        ticks->push_back(off);
        noteInstanceID++;
    }

    outTicksPerQuarter = pr.ticksPerQuarter;
    return ticks;
}

bool ValidateBeforeRun(const GUIState& state, std::string& err)
{
    return gui::ValidateRunSettings(
        state.midiPath,
        state.wavPath,
        state.targetChannel,
        state.sampleRate,
        state.initialSeconds,
        state.bits,
        err);
}

std::string RuntimeInstrumentId(int ch)
{
    return "gui_runtime__ch" + std::to_string(ch);
}

ProjectModel BuildRuntimeProjectFromGUI(GUIState& state)
{
    ProjectModel project = gui::BuildProjectModelFromGUI(state);

    auto instruments = std::make_shared<std::map<std::string, InstrumentConfig>>();
    auto projectChannels = std::make_shared<std::array<ProjectChannelConfig, 16>>();
    const auto& channelConfigs = state.channelConfigs ? *state.channelConfigs : gui::ReadChannelConfigs(state);
    const auto& channelMixStates = state.channelMixStates ? *state.channelMixStates : gui::ReadChannelMixStates(state);

    for (int ch = 0; ch < 16; ch++)
    {
        const int src = std::clamp(state.channelAssignments[ch], 0, 15);
        const std::string id = RuntimeInstrumentId(ch);
        InstrumentConfig instrument{};
        instrument.sound = channelConfigs[src];
        instruments->emplace(id, instrument);

        ProjectChannelConfig projectChannel{};
        projectChannel.enabled = true;
        projectChannel.instrumentId = id;
        projectChannel.mix = channelMixStates[ch];
        (*projectChannels)[ch] = projectChannel;
    }

    project.channelConfigs = std::make_shared<const std::array<ChannelConfig, 16>>(channelConfigs);
    project.channelMixStates = std::make_shared<const std::array<ChannelMixState, 16>>(channelMixStates);
    project.instruments = instruments;
    project.projectChannels = projectChannels;
    return project;
}

void OverridePreviewChannelWithSelectedSoundSlot(const GUIState& state, int previewChannel, ProjectModel& project)
{
    if (!state.channelConfigs || !project.instruments || !project.projectChannels)
    {
        return;
    }

    previewChannel = std::clamp(previewChannel, 0, 15);
    const int slot = std::clamp(state.selectedSoundSlot, 0, 15);
    const std::string instrumentId = (*project.projectChannels)[previewChannel].instrumentId;
    auto mutableInstruments = std::make_shared<std::map<std::string, InstrumentConfig>>(*project.instruments);
    auto it = mutableInstruments->find(instrumentId);
    if (it != mutableInstruments->end())
    {
        it->second.sound = (*state.channelConfigs)[slot];
    }
    project.instruments = mutableInstruments;
}

std::string FormatRunException(const std::exception& ex)
{
    if (const auto* systemError = dynamic_cast<const std::system_error*>(&ex))
    {
        return std::string(systemError->what()) +
            " code=" + std::to_string(systemError->code().value()) +
            " category=" + systemError->code().category().name();
    }
    return ex.what();
}

class GUIPreviewStreamSink final : public IPreviewStreamSink
{
public:
    GUIPreviewStreamSink(PreviewPlaybackState& playback, bool loop, int startTick)
        : playback_(playback)
        , loop_(loop)
        , startTick_(startTick)
    {
    }

    bool Begin(int sampleRate, int channels, int, bool loop) override
    {
        std::string err;
        session_ = StartStreamingPreviewAudio(
            playback_,
            sampleRate,
            static_cast<ma_uint32>(channels),
            loop_ || loop,
            err);
        playback_.playStartTick.store(startTick_, std::memory_order_relaxed);
        return session_ != 0;
    }

    bool WriteFrame(double left, double right) override
    {
        return WriteStreamingPreviewFrame(playback_, session_, left, right);
    }

    bool WriteFrames(const double* interleavedStereo, int frameCount) override
    {
        return WriteStreamingPreviewFrames(playback_, session_, interleavedStereo, frameCount);
    }

    void Complete(bool canceled) override
    {
        CompleteStreamingPreviewAudio(playback_, session_, canceled);
    }

private:
    PreviewPlaybackState& playback_;
    bool loop_ = false;
    int startTick_ = 0;
    uint64_t session_ = 0;
};

int RunSafely(
    ProjectModel project,
    RenderOptions options,
    RenderRuntimeOverrides overrides,
    GUIState& state)
{
    try
    {
        if (options.mode == RunMode::Preview)
        {
            GUIPreviewStreamSink sink(state.playback, state.previewLoop, state.previewRequestedStartTick);
            return RunPreviewStreaming(project, options, overrides, &state.observer, sink, state.previewLoop);
        }
        return Run(project, options, overrides, &state.observer, nullptr);
    }
    catch (const std::exception& ex)
    {
        gui::detail::AppendGUILogToTab(state, state.runLogTab, "[GUI] Run exception: " + FormatRunException(ex));
        return 1;
    }
    catch (...)
    {
        gui::detail::AppendGUILogToTab(state, state.runLogTab, "[GUI] Run exception: unknown exception");
        return 1;
    }
}

void MarkRunStartFailed(GUIState& state, const std::string& detail)
{
    state.running = false;
    state.hasRun = true;
    state.lastRunExitCode = 1;
    state.runIsPreview = false;
    gui::AppendGUILog(state, "[GUI] Run start failed: " + detail);
    gui::RaiseGUIError(
        state,
        BuildUserErrorMessage("Export/Preview を開始できません。実行環境を確認してください。", detail),
        0,
        true);
}
} // namespace

namespace gui
{
void StartGUIRun(GUIState& state, bool previewSelected)
{
    std::string validationError;
    if (!ValidateBeforeRun(state, validationError))
    {
        state.hasRun = true;
        state.lastRunExitCode = 1;
        AppendGUILog(state, "[GUI] Validation failed: " + validationError);
        const int actionHint = (validationError.find("MIDI") != std::string::npos) ? 1 :
            ((validationError.find("Output") != std::string::npos) ? 2 : 0);
        const std::string summary = (actionHint == 1)
            ? "Export/Preview を開始できません。MIDI 設定を確認してください。"
            : ((actionHint == 2)
                ? "Export/Preview を開始できません。出力先設定を確認してください。"
                : "Export/Preview を開始できません。入力値を確認してください。");
        RaiseGUIError(state, BuildUserErrorMessage(summary, validationError), actionHint, true);
        return;
    }
    ClearGUIError(state);

    const int previewChannel = previewSelected
        ? std::clamp(state.pianoRoll.displayChannel, 0, 15)
        : std::clamp(state.selectedSoundSlot, 0, 15);
    if (previewSelected)
    {
        ActivateSoloPreview(state, previewChannel);
    }
    if (state.playback.playing.load(std::memory_order_relaxed))
    {
        StopPreviewAudio(state.playback);
        AppendGUILog(state, "[GUI] Previous preview playback stopped for new run");
    }

    ProjectModel project = BuildRuntimeProjectFromGUI(state);
    if (previewSelected)
    {
        project.targetChannel = previewChannel;
    }
    if (previewSelected && state.UIModeTab == 0)
    {
        OverridePreviewChannelWithSelectedSoundSlot(state, previewChannel, project);
        AppendGUILog(state, "[GUI] Sound Preview route: PR Channel ch" + std::to_string(previewChannel) +
            " <= Selected Slot s" + std::to_string(std::clamp(state.selectedSoundSlot, 0, 15)));
    }
    int overrideTicksPerQuarter = 0;
    RenderRuntimeOverrides overrides{};
    overrides.noteTicks = BuildOverrideNoteTicksFromPianoRoll(state, overrideTicksPerQuarter);
    overrides.ticksPerQuarter = overrideTicksPerQuarter;
    RenderOptions options = previewSelected ? DefaultPreviewRenderOptions() : DefaultRenderOptions();
    if (!previewSelected && state.serialSave)
    {
        project.wavPath = BuildSerialWAVPath(project.wavPath);
    }
    if (previewSelected)
    {
        state.restorePreviewOnRunComplete = true;
        options.writeWAV = false;
        const double rangeDurationSec = PreviewRangeDurationSec(state);
        if (state.pianoRoll.previewRangeEnabled && rangeDurationSec > 0.0)
        {
            options.durationSec = rangeDurationSec;
        }
        else if (!state.pianoRoll.previewRangeEnabled)
        {
            options.startSec = 0.0;
            options.durationSec = -1.0;
        }
        int startTick = 0;
        if (state.pianoRoll.previewRangeEnabled)
        {
            startTick = (std::min)(state.pianoRoll.previewRangeStartTick, state.pianoRoll.previewRangeEndTick);
        }
        if (startTick > 0 && state.pianoRoll.ticksPerQuarter > 0)
        {
            options.startSec = SecondsAtTickForPreview(
                state.pianoRoll.tempoEvents,
                state.pianoRoll.ticksPerQuarter,
                startTick);
        }
        state.previewRequestedStartTick = startTick;
        state.previewRequestedDurationSec = rangeDurationSec;
    }
    else
    {
        state.previewRequestedStartTick = 0;
        state.previewRequestedDurationSec = 0.0;
    }
    state.lastOutputPath = previewSelected ? "[memory preview]" : PathToUtf8(project.wavPath);

    state.runLogTab = state.UIModeTab;
    state.observer.logs = &detail::LogsByTab(state, state.runLogTab);
    {
        std::lock_guard<std::mutex> lock(state.logMutex);
        detail::LogsByTab(state, state.runLogTab).clear();
    }
    state.lastPeak = 0.0;
    state.hasPeak = false;
    state.runIsPreview = previewSelected;
    detail::AppendGUILogToTab(state, state.runLogTab, previewSelected ? "[GUI] Preview Play started" : "[GUI] Export started");
    if (overrides.noteTicks != nullptr)
    {
        detail::AppendGUILogToTab(state, state.runLogTab, "[GUI] PianoRoll edited notes applied: count=" +
            std::to_string(overrides.noteTicks->size() / 2));
    }
    if (previewSelected)
    {
        if (state.pianoRoll.previewRangeEnabled)
        {
            const int a = (std::min)(state.pianoRoll.previewRangeStartTick, state.pianoRoll.previewRangeEndTick);
            const int b = (std::max)(state.pianoRoll.previewRangeStartTick, state.pianoRoll.previewRangeEndTick);
            detail::AppendGUILogToTab(state, state.runLogTab, "[GUI] Preview optional range tick=" + std::to_string(a) + "-" +
                std::to_string(b) + " secStart=" + std::to_string(options.startSec) +
                " secDuration=" + std::to_string(options.durationSec));
        }
        else
        {
            detail::AppendGUILogToTab(state, state.runLogTab, "[GUI] Preview full range secStart=0 secDuration=full");
        }
    }
    detail::AppendGUILogToTab(state, state.runLogTab, "[GUI] Effective Output: " + state.lastOutputPath);
    state.hasRun = false;
    state.stopRequested.store(false, std::memory_order_relaxed);
    state.running = true;
    try
    {
        state.runFuture = std::async(std::launch::async, [project, options, overrides, &state]() {
            return RunSafely(project, options, overrides, state);
            });
    }
    catch (const std::exception& ex)
    {
        MarkRunStartFailed(state, FormatRunException(ex));
    }
    catch (...)
    {
        MarkRunStartFailed(state, "unknown exception");
    }
}

bool TryFinalizeCompletedRun(GUIState& state)
{
    if (!state.running ||
        !state.runFuture.valid() ||
        state.runFuture.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
    {
        return false;
    }

    try
    {
        state.lastRunExitCode = state.runFuture.get();
    }
    catch (const std::exception& ex)
    {
        state.lastRunExitCode = 1;
        detail::AppendGUILogToTab(state, state.runLogTab, "[GUI] Run future exception: " + FormatRunException(ex));
        RaiseGUIError(
            state,
            BuildUserErrorMessage("Export/Preview 実行中に例外が発生しました。ログを確認してください。", FormatRunException(ex)),
            0,
            true);
    }
    catch (...)
    {
        state.lastRunExitCode = 1;
        detail::AppendGUILogToTab(state, state.runLogTab, "[GUI] Run future exception: unknown exception");
        RaiseGUIError(
            state,
            "Export/Preview 実行中に不明な例外が発生しました。ログを確認してください。",
            0,
            true);
    }
    state.hasRun = true;
    state.running = false;
    detail::AppendGUILogToTab(state, state.runLogTab, std::string("[GUI] Run finished: exit=") + std::to_string(state.lastRunExitCode));
    const bool finishedPreview = state.runIsPreview;
    if (state.runIsPreview)
    {
        if (state.lastRunExitCode == 0)
        {
            state.previewAudioReady = true;
            detail::AppendGUILogToTab(state, state.runLogTab, "[GUI] Preview streaming completed");
        }
        else
        {
            state.previewAudioReady = false;
        }
        state.runIsPreview = false;
    }
    if (!finishedPreview &&
        state.lastRunExitCode == 0 &&
        !state.lastOutputPath.empty() &&
        state.lastOutputPath != "[memory preview]")
    {
        state.recentWavPaths.insert(state.recentWavPaths.begin(), state.lastOutputPath);
        if (state.recentWavPaths.size() > 5)
        {
            state.recentWavPaths.resize(5);
        }
    }
    if (state.restorePreviewOnRunComplete)
    {
        DeactivateSoloPreview(state);
    }
    return true;
}
} // namespace gui
