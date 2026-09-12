#include "gui/GUIActions.h"

#include <algorithm>
#include <map>
#include <vector>

#include "AppCore.h"
#include "midi/TempoMap.h"
#include "gui/GUIRenderJob.h"
#include "gui/GUIConfigUtils.h"
#include "gui/GUIPianoRoll.h"
#include "gui/GUIProjectFacade.h"
#include "gui/GUIRunHelpers.h"
#include "gui/PreviewAudio.h"
#include "io/PlatformPaths.h"
#include "project/ProjectModel.h"

namespace
{
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
    const midi::TempoMap tempo(pr.tempoEvents, pr.ticksPerQuarter);
    const double startSec = tempo.SecondsAtTick(rangeStartTick);
    const double endSec = tempo.SecondsAtTick(rangeEndTick);
    return (std::max)(0.0, endSec - startSec);
}

std::shared_ptr<const std::vector<MIDIEventTick>> BuildOverrideNoteTicksFromPianoRoll(
    const GUIState& state,
    int& outTicksPerQuarter)
{
    outTicksPerQuarter = 0;
    const auto& pr = state.pianoRoll;
    if (pr.hasLoadError || pr.ticksPerQuarter <= 0)
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

} // namespace

namespace gui
{
void StartGUIRun(GUIState& state, bool previewSelected, bool selectedChannelOnly)
{
    if (state.running) return;
    if (!previewSelected && PendingToneCount(state))
    { RaiseGUIError(state, "音色の変更を採用してから書き出してください。", 0, true); return; }
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
        RaiseGUIError(state, detail::BuildUserErrorMessage(summary, validationError), actionHint, true);
        return;
    }
    // Workspace notes must be restored even when playback/export starts before
    // the compose tab has drawn its piano roll.
    if (!LoadPianoRollMIDI(state.pianoRoll, Utf8ToPath(state.midiPath)))
    {
        RaiseGUIError(state, "MIDI を読み込めません。" + state.pianoRoll.lastError, 1, true);
        return;
    }
    // miniaudio's context owns thread-local COM initialization. Create/recreate
    // it here, on the same GUI thread that eventually destroys the device.
    if (previewSelected && !EnsurePreviewAudioDevice(state.playback, state.sampleRate, validationError))
    {
        detail::ReportRunStartFailure(state, validationError);
        return;
    }
    ClearGUIError(state);

    const int previewChannel = std::clamp(state.pianoRoll.displayChannel, 0, 15);
    if (state.playback.playing.load(std::memory_order_relaxed))
    {
        StopPreviewAudio(state.playback);
        AppendGUILog(state, "[GUI] Previous preview playback stopped for new run");
    }

    ProjectModel project = BuildProjectModelFromGUI(state);
    if (previewSelected)
    {
        project.targetChannel = selectedChannelOnly ? previewChannel : -1;
    }
    int overrideTicksPerQuarter = 0;
    RenderRuntimeOverrides overrides{};
    if (previewSelected)
    {
        PublishLiveRenderSettings(state);
        overrides.liveSettings = state.liveSettings;
    }
    overrides.noteTicks = BuildOverrideNoteTicksFromPianoRoll(state, overrideTicksPerQuarter);
    overrides.ticksPerQuarter = overrideTicksPerQuarter;
    RenderOptions options = previewSelected ? DefaultPreviewRenderOptions() : DefaultRenderOptions();
    if (!previewSelected && state.serialSave)
    {
        project.wavPath = BuildSerialWAVPath(project.wavPath);
    }
    if (previewSelected)
    {
        options.writeWAV = false;
        const midi::TempoMap tempo(state.pianoRoll.tempoEvents, state.pianoRoll.ticksPerQuarter);
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
        int startTick = state.songCursorTick;
        if (state.pianoRoll.previewRangeEnabled)
        {
            startTick = (std::min)(state.pianoRoll.previewRangeStartTick, state.pianoRoll.previewRangeEndTick);
        }
        state.previewFrameOffset = 0;
        if (state.pianoRoll.previewRangeEnabled && state.songCursorTick > startTick && state.songCursorTick < state.pianoRoll.previewRangeEndTick)
        {
            options.previewSkipSec = tempo.SecondsAtTick(state.songCursorTick) - tempo.SecondsAtTick(startTick);
            state.previewFrameOffset = static_cast<uint64_t>(options.previewSkipSec * state.sampleRate);
        }
        if (startTick > 0 && state.pianoRoll.ticksPerQuarter > 0)
        {
            options.startSec = tempo.SecondsAtTick(startTick);
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

    detail::LaunchRenderJob(state, {std::move(project), options, std::move(overrides),
        previewSelected ? 1 : 2, state.previewRequestedStartTick, state.previewFrameOffset, state.previewLoop});
}
} // namespace gui
