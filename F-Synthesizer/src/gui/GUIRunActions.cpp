#include "gui/GUIActions.h"

#include <algorithm>
#include <chrono>
#include <future>
#include <mutex>
#include <vector>

#include "AppCore.h"
#include "gui/GUIActionsInternal.h"
#include "gui/GUIConfigUtils.h"
#include "gui/GUIRunHelpers.h"
#include "gui/GUIStateModel.h"
#include "gui/PreviewAudio.h"
#include "io/PlatformPaths.h"

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

void TrimPreviewSoundByDuration(SoundData& sound, double durationSec)
{
    if (durationSec <= 0.0 || sound.fs <= 0 || sound.length <= 0)
    {
        return;
    }
    const uint64_t keepSamples = static_cast<uint64_t>(durationSec * static_cast<double>(sound.fs));
    if (keepSamples == 0)
    {
        if (sound.channels >= 2)
        {
            sound.dataL.clear();
            sound.dataR.clear();
        }
        else
        {
            sound.data.clear();
        }
        sound.length = 0;
        return;
    }
    if (keepSamples < static_cast<uint64_t>(sound.length))
    {
        const size_t keep = static_cast<size_t>(keepSamples);
        if (sound.channels >= 2)
        {
            sound.dataL.resize(keep);
            sound.dataR.resize(keep);
        }
        else
        {
            sound.data.resize(keep);
        }
        sound.length = static_cast<int>(keep);
    }
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

    AppConfig cfg = BuildConfigFromGUI(state);
    if (previewSelected && state.UIModeTab == 0)
    {
        cfg.targetChannel = previewChannel;
        OverridePreviewChannelWithSelectedSoundSlot(state, previewChannel, cfg);
        AppendGUILog(state, "[GUI] Sound Preview route: PR Channel ch" + std::to_string(previewChannel) +
            " <= Selected Slot s" + std::to_string(std::clamp(state.selectedSoundSlot, 0, 15)));
    }
    int overrideTicksPerQuarter = 0;
    cfg.overrideNoteTicks = BuildOverrideNoteTicksFromPianoRoll(state, overrideTicksPerQuarter);
    cfg.overrideTicksPerQuarter = overrideTicksPerQuarter;
    RenderOptions options = previewSelected ? DefaultPreviewRenderOptions() : DefaultRenderOptions();
    if (!previewSelected && state.serialSave)
    {
        cfg.wavPath = BuildSerialWAVPath(cfg.wavPath);
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
        int startTick = state.pianoRoll.previewStartTick;
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
    state.lastOutputPath = previewSelected ? "[memory preview]" : PathToUtf8(cfg.wavPath);

    state.runLogTab = state.UIModeTab;
    state.observer.logs = &detail::LogsByTab(state, state.runLogTab);
    {
        std::lock_guard<std::mutex> lock(state.logMutex);
        detail::LogsByTab(state, state.runLogTab).clear();
    }
    state.lastPeak = 0.0;
    state.hasPeak = false;
    state.runOutputBuffer = previewSelected ? std::make_shared<SoundData>() : nullptr;
    state.runIsPreview = previewSelected;
    state.autoPlayPreviewOnRunComplete = previewSelected;
    detail::AppendGUILogToTab(state, state.runLogTab, previewSelected ? "[GUI] Preview Play started" : "[GUI] Export started");
    if (cfg.overrideNoteTicks != nullptr)
    {
        detail::AppendGUILogToTab(state, state.runLogTab, "[GUI] PianoRoll edited notes applied: count=" +
            std::to_string(cfg.overrideNoteTicks->size() / 2));
    }
    if (previewSelected)
    {
        if (state.pianoRoll.previewRangeEnabled)
        {
            const int a = (std::min)(state.pianoRoll.previewRangeStartTick, state.pianoRoll.previewRangeEndTick);
            const int b = (std::max)(state.pianoRoll.previewRangeStartTick, state.pianoRoll.previewRangeEndTick);
            detail::AppendGUILogToTab(state, state.runLogTab, "[GUI] Preview range tick=" + std::to_string(a) + "-" +
                std::to_string(b) + " secStart=" + std::to_string(options.startSec) +
                " secDuration=" + std::to_string(options.durationSec));
        }
        else
        {
            detail::AppendGUILogToTab(state, state.runLogTab, "[GUI] Preview start tick=" + std::to_string(state.pianoRoll.previewStartTick) +
                " sec=" + std::to_string(options.startSec));
        }
    }
    detail::AppendGUILogToTab(state, state.runLogTab, "[GUI] Effective Output: " + state.lastOutputPath);
    state.hasRun = false;
    state.stopRequested.store(false, std::memory_order_relaxed);
    state.running = true;
    state.runFuture = std::async(std::launch::async, [cfg, options, outBuffer = state.runOutputBuffer, &state]() {
        return Run(cfg, options, &state.observer, outBuffer.get());
        });
}

bool TryFinalizeCompletedRun(GUIState& state)
{
    if (!state.running ||
        !state.runFuture.valid() ||
        state.runFuture.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
    {
        return false;
    }

    state.lastRunExitCode = state.runFuture.get();
    state.hasRun = true;
    state.running = false;
    detail::AppendGUILogToTab(state, state.runLogTab, std::string("[GUI] Run finished: exit=") + std::to_string(state.lastRunExitCode));
    if (state.runIsPreview)
    {
        if (state.lastRunExitCode == 0 &&
            state.runOutputBuffer != nullptr &&
            state.runOutputBuffer->length > 0)
        {
            if (state.pianoRoll.previewRangeEnabled)
            {
                TrimPreviewSoundByDuration(*state.runOutputBuffer, state.previewRequestedDurationSec);
            }
            if (state.runOutputBuffer->length <= 0)
            {
                state.previewAudioReady = false;
                state.previewRenderedSound.reset();
                detail::AppendGUILogToTab(state, state.runLogTab, "[GUI] Preview range is too short: no audio");
                state.runOutputBuffer.reset();
                state.runIsPreview = false;
                state.autoPlayPreviewOnRunComplete = false;
                if (state.restorePreviewOnRunComplete)
                {
                    DeactivateSoloPreview(state);
                }
                return true;
            }
            state.previewRenderedSound = state.runOutputBuffer;
            state.previewAudioReady = true;
            detail::AppendGUILogToTab(state, state.runLogTab, "[GUI] Preview audio buffer ready");
            if (state.autoPlayPreviewOnRunComplete)
            {
                std::string playErr;
                state.playback.playStartTick.store(state.previewRequestedStartTick, std::memory_order_relaxed);
                if (PlayPreviewAudio(state.playback, *state.previewRenderedSound, state.previewLoop, playErr))
                {
                    detail::AppendGUILogToTab(state, state.runLogTab, "[GUI] Preview playback started");
                }
                else
                {
                    detail::AppendGUILogToTab(state, state.runLogTab, "[GUI] Preview playback failed: " + playErr);
                    RaiseGUIError(
                        state,
                        BuildUserErrorMessage("Preview 再生に失敗しました。設定を確認して再実行してください。", playErr),
                        0,
                        true);
                }
            }
        }
        else
        {
            state.previewAudioReady = false;
            state.previewRenderedSound.reset();
        }
        state.runOutputBuffer.reset();
        state.runIsPreview = false;
        state.autoPlayPreviewOnRunComplete = false;
    }
    if (state.restorePreviewOnRunComplete)
    {
        DeactivateSoloPreview(state);
    }
    return true;
}
} // namespace gui
