#include "gui/GUIActions.h"

#include <algorithm>
#include <array>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "AppCore.h"
#include "config/SourceRegistry.h"
#include "gui/GUIRenderJob.h"
#include "gui/GUIProjectFacade.h"
#include "gui/PreviewAudio.h"
#include "project/ProjectModel.h"

namespace
{
bool ValidatePreviewOnlySettings(const GUIState& state, std::string& err)
{
    if (state.targetChannel < -1 || state.targetChannel > 15)
    {
        err = "Target Channel must be -1 or 0..15.";
        return false;
    }
    if (state.pianoRoll.displayChannel < 0 || state.pianoRoll.displayChannel > 15)
    {
        err = "Selected channel must be 0..15.";
        return false;
    }
    if (state.sampleRate <= 0)
    {
        err = "Sample Rate must be positive.";
        return false;
    }
    if (state.bits != 16)
    {
        err = "Bits must be 16 in current implementation.";
        return false;
    }
    return true;
}

} // namespace

namespace gui
{
void StartGUISoundTonePreview(GUIState& state)
{
    if (state.running) return;
    std::string validationError;
    if (!ValidatePreviewOnlySettings(state, validationError))
    {
        state.hasRun = true;
        state.lastRunExitCode = 1;
        AppendGUILog(state, "[GUI] Sound Tone Preview validation failed: " + validationError);
        RaiseGUIError(
            state,
            detail::BuildUserErrorMessage("Tone Preview を開始できません。Sound 設定を確認してください。", validationError),
            3,
            true);
        return;
    }
    // Device/COM lifetime belongs to the GUI thread, not this preview's
    // short-lived async render thread.
    if (!EnsurePreviewAudioDevice(state.playback, state.sampleRate, validationError))
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
    project.targetChannel = previewChannel;
    const int previewNote = ChooseAuditionNote(state, previewChannel);
    project.midiPath.clear();
    RenderRuntimeOverrides overrides{};
    PublishLiveRenderSettings(state);
    overrides.liveSettings = state.liveSettings;
    overrides.ticksPerQuarter = 480;
    auto notes = std::make_shared<std::vector<MIDIEventTick>>();
    auto addNote = [&](int tick, int pitch, int length, int velocity) {
        MIDIEventTick on{}, off{};
        on.type = off.type = MIDIEventType::Note;
        on.tick = tick; off.tick = tick + length;
        on.channel = off.channel = previewChannel;
        on.noteNumber = off.noteNumber = pitch;
        on.velocity = velocity; on.isNoteOn = true;
        on.noteInstanceID = off.noteInstanceID = static_cast<int>(notes->size() / 2 + 1);
        on.order = static_cast<int>(notes->size()); off.order = on.order + 1;
        notes->push_back(on); notes->push_back(off);
    };
    const bool drums = std::holds_alternative<DrumKitConfig>(AudibleInstrument(state, previewChannel).sound.source);
    if (drums)
    {
        for (int step = 0; step < 16; ++step)
        {
            addNote(step * 240, 42, 100, step % 2 ? 65 : 90);
            if (step % 4 == 0) addNote(step * 240, 36, 140, 110);
            if (step % 4 == 2) addNote(step * 240, 38, 140, 105);
        }
    }
    else addNote(0, previewNote, static_cast<int>(std::clamp(state.auditionLengthSec, .2f, 3.f) * 960), 100);
    overrides.noteTicks = notes;

    RenderOptions options = DefaultPreviewRenderOptions();
    options.writeWAV = false;
    options.startSec = 0.0;
    options.durationSec = drums ? 4.0 : state.auditionLengthSec + .5;
    project.extraReleaseSec = .5;

    state.previewRequestedStartTick = 0;
    state.previewRequestedDurationSec = options.durationSec;
    state.lastOutputPath = "[memory preview]";

    detail::LaunchRenderJob(state, {std::move(project), options, std::move(overrides), 0});
}
} // namespace gui
