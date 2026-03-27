#include "gui/GUIActions.h"

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <vector>

#include "AppCore.h"
#include "gui/GUIActionsInternal.h"
#include "gui/GUIStateModel.h"
#include "gui/PreviewAudio.h"

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

bool ValidatePreviewOnlySettings(const GUIState& state, std::string& err)
{
    if (state.targetChannel < -1 || state.targetChannel > 15)
    {
        err = "Target Channel must be -1 or 0..15.";
        return false;
    }
    if (state.selectedSoundSlot < 0 || state.selectedSoundSlot > 15)
    {
        err = "Selected Sound Slot must be 0..15.";
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

std::shared_ptr<const std::vector<MIDIEventTick>> BuildOverrideNoteTicksForSoundTone(
    int channel,
    int noteNumber,
    int velocity,
    int ticksPerQuarter)
{
    auto ticks = std::make_shared<std::vector<MIDIEventTick>>();
    ticks->reserve(2);

    MIDIEventTick on{};
    on.type = MIDIEventType::Note;
    on.tick = 0;
    on.noteNumber = std::clamp(noteNumber, 0, 127);
    on.velocity = std::clamp(velocity, 1, 127);
    on.channel = std::clamp(channel, 0, 15);
    on.controller = 0;
    on.value = 0;
    on.noteInstanceID = 1;
    on.order = 0;
    on.isNoteOn = true;
    ticks->push_back(on);

    MIDIEventTick off{};
    off.type = MIDIEventType::Note;
    off.tick = (std::max)(1, ticksPerQuarter);
    off.noteNumber = on.noteNumber;
    off.velocity = 0;
    off.channel = on.channel;
    off.controller = 0;
    off.value = 0;
    off.noteInstanceID = on.noteInstanceID;
    off.order = 1;
    off.isNoteOn = false;
    ticks->push_back(off);

    return ticks;
}

int ResolveSoundTonePreviewNote(const GUIState& state, int slot)
{
    if (!state.channelConfigs)
    {
        return 60;
    }

    slot = std::clamp(slot, 0, 15);
    const SourceConfig& src = (*state.channelConfigs)[slot].source;
    if (!std::holds_alternative<DrumKitConfig>(src))
    {
        return std::clamp(state.tonePreviewNoteNumber, 0, 127);
    }
    if (const auto* kit = std::get_if<DrumKitConfig>(&src))
    {
        const int preferred = std::clamp(state.selectedDrumNote, 0, 127);
        if (kit->map[preferred].type != DrumType::None)
        {
            return preferred;
        }

        constexpr int kPreferredNotes[] = { 36, 38, 42 };
        for (int n : kPreferredNotes)
        {
            if (kit->map[n].type != DrumType::None)
            {
                return n;
            }
        }
        for (int n = 0; n < 128; n++)
        {
            if (kit->map[n].type != DrumType::None)
            {
                return n;
            }
        }
        return 36;
    }
    return std::clamp(state.tonePreviewNoteNumber, 0, 127);
}

void OverridePreviewChannelWithSelectedSoundSlot(const GUIState& state, int previewChannel, AppConfig& cfg)
{
    if (!state.channelConfigs)
    {
        return;
    }

    auto previewConfigs = std::make_shared<std::array<ChannelConfig, 16>>();
    if (cfg.channelConfigs)
    {
        *previewConfigs = *cfg.channelConfigs;
    }
    else
    {
        const AppConfig def = DefaultConfig();
        if (def.channelConfigs)
        {
            *previewConfigs = *def.channelConfigs;
        }
    }

    previewChannel = std::clamp(previewChannel, 0, 15);
    const int slot = std::clamp(state.selectedSoundSlot, 0, 15);
    (*previewConfigs)[previewChannel] = (*state.channelConfigs)[slot];
    cfg.channelConfigs = std::static_pointer_cast<const std::array<ChannelConfig, 16>>(previewConfigs);
}
} // namespace

namespace gui
{
void ActivateSoloPreview(GUIState& state, int channel)
{
    EnsureChannelMixStates(state);
    channel = std::clamp(channel, 0, 15);
    if (!state.soloPreviewActive)
    {
        state.soloPreviewBackup = *state.channelMixStates;
    }
    for (int ch = 0; ch < 16; ch++)
    {
        ChannelMixState& mix = (*state.channelMixStates)[ch];
        mix.solo = (ch == channel);
        if (ch == channel)
        {
            mix.mute = false;
        }
    }
    state.soloPreviewChannel = channel;
    state.soloPreviewActive = true;
    AppendGUILog(state, "[GUI] Solo Preview ON: ch" + std::to_string(channel));
}

void DeactivateSoloPreview(GUIState& state)
{
    if (!state.soloPreviewActive || !state.channelMixStates)
    {
        return;
    }
    *state.channelMixStates = state.soloPreviewBackup;
    AppendGUILog(state, "[GUI] Solo Preview OFF: restore previous mix state");
    state.soloPreviewActive = false;
    state.restorePreviewOnRunComplete = false;
}

void StartGUISoundTonePreview(GUIState& state)
{
    std::string validationError;
    if (!ValidatePreviewOnlySettings(state, validationError))
    {
        state.hasRun = true;
        state.lastRunExitCode = 1;
        AppendGUILog(state, "[GUI] Sound Tone Preview validation failed: " + validationError);
        RaiseGUIError(
            state,
            BuildUserErrorMessage("Tone Preview を開始できません。Sound 設定を確認してください。", validationError),
            3,
            true);
        return;
    }
    ClearGUIError(state);

    const int previewChannel = std::clamp(state.selectedSoundSlot, 0, 15);
    ActivateSoloPreview(state, previewChannel);
    if (state.playback.playing.load(std::memory_order_relaxed))
    {
        StopPreviewAudio(state.playback);
        AppendGUILog(state, "[GUI] Previous preview playback stopped for new run");
    }

    AppConfig cfg = BuildConfigFromGUI(state);
    cfg.targetChannel = previewChannel;
    OverridePreviewChannelWithSelectedSoundSlot(state, previewChannel, cfg);
    const int previewNote = ResolveSoundTonePreviewNote(state, previewChannel);
    cfg.midiPath.clear();
    cfg.overrideTicksPerQuarter = 480;
    cfg.overrideNoteTicks = BuildOverrideNoteTicksForSoundTone(previewChannel, previewNote, 110, cfg.overrideTicksPerQuarter);

    RenderOptions options = DefaultPreviewRenderOptions();
    options.writeWAV = false;
    options.startSec = 0.0;
    options.durationSec = 1.5;

    state.restorePreviewOnRunComplete = true;
    state.previewRequestedStartTick = 0;
    state.previewRequestedDurationSec = options.durationSec;
    state.lastOutputPath = "[memory preview]";

    state.runLogTab = state.UIModeTab;
    state.observer.logs = &detail::LogsByTab(state, state.runLogTab);
    {
        std::lock_guard<std::mutex> lock(state.logMutex);
        detail::LogsByTab(state, state.runLogTab).clear();
    }
    state.lastPeak = 0.0;
    state.hasPeak = false;
    state.runOutputBuffer = std::make_shared<SoundData>();
    state.runIsPreview = true;
    state.autoPlayPreviewOnRunComplete = true;
    detail::AppendGUILogToTab(state, state.runLogTab, "[GUI] Sound Tone Preview started (note=" + std::to_string(previewNote) + ")");
    detail::AppendGUILogToTab(state, state.runLogTab, "[GUI] Effective Output: " + state.lastOutputPath);
    state.hasRun = false;
    state.stopRequested.store(false, std::memory_order_relaxed);
    state.running = true;
    state.runFuture = std::async(std::launch::async, [cfg, options, outBuffer = state.runOutputBuffer, &state]() {
        return Run(cfg, options, &state.observer, outBuffer.get());
        });
}

void StopGUIRunAndPreview(GUIState& state)
{
    state.autoPlayPreviewOnRunComplete = false;

    if (state.playback.playing.load(std::memory_order_relaxed))
    {
        StopPreviewAudio(state.playback);
        AppendGUILog(state, "[GUI] Preview playback stopped");
    }
    if (state.running)
    {
        state.stopRequested.store(true, std::memory_order_relaxed);
        AppendGUILog(state, "[GUI] Stop requested (render cancellation signal sent)");
    }
}
} // namespace gui
