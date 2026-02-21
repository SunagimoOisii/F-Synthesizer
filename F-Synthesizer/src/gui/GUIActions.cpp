#include "gui/GUIActions.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <future>
#include <vector>

#include "AppCore.h"
#include "gui/GUIConfigUtils.h"
#include "gui/GUIPlatform.h"
#include "gui/GUIPresetIO.h"
#include "gui/GUIRunHelpers.h"
#include "gui/GUIStateModel.h"
#include "gui/PreviewAudio.h"
#include "io/PlatformPaths.h"

namespace
{
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
    if (durationSec <= 0.0 || sound.fs <= 0 || sound.data.empty())
    {
        return;
    }
    const uint64_t keepSamples = static_cast<uint64_t>(durationSec * static_cast<double>(sound.fs));
    if (keepSamples == 0)
    {
        sound.data.clear();
        sound.length = 0;
        return;
    }
    if (keepSamples < sound.data.size())
    {
        sound.data.resize(static_cast<size_t>(keepSamples));
        sound.length = static_cast<int>(sound.data.size());
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
        off.order = order++;
        off.isNoteOn = false;
        ticks->push_back(off);
    }

    outTicksPerQuarter = pr.ticksPerQuarter;
    return ticks;
}

int FindPresetIndex(const GUIState& state, const std::string& name)
{
    for (int i = 0; i < static_cast<int>(state.presetItems.size()); i++)
    {
        if (state.presetItems[i] == name)
        {
            return i;
        }
    }
    return -1;
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
namespace
{
std::vector<std::string>& LogsByTab(GUIState& state, int tab)
{
    return (tab == 1) ? state.musicLogs : state.soundLogs;
}

const std::vector<std::string>& LogsByTab(const GUIState& state, int tab)
{
    return (tab == 1) ? state.musicLogs : state.soundLogs;
}

void AppendGUILogToTab(GUIState& state, int tab, const std::string& line)
{
    std::lock_guard<std::mutex> lock(state.logMutex);
    LogsByTab(state, tab).push_back(line);
}
} // namespace

void AppendGUILog(GUIState& state, const std::string& line)
{
    AppendGUILogToTab(state, state.uiModeTab, line);
}

void RefreshPresetItems(GUIState& state, const std::string& preferName)
{
    // preset一覧は毎回ファイル一覧を読み直し、追加/削除を再起動なしで反映する。
    state.presetItems = CollectPresetItems(FindProjectRootPath());
    if (state.presetItems.empty())
    {
        state.presetItems.push_back("basic_wave");
    }

    int idx = FindPresetIndex(state, preferName);
    if (idx < 0)
    {
        idx = FindPresetIndex(state, "basic_wave");
    }
    state.presetIndex = (idx >= 0) ? idx : 0;
}

bool ApplySelectedPresetPaths(GUIState& state, std::string& err)
{
    if (state.presetItems.empty())
    {
        err = "preset list is empty";
        return false;
    }
    if (state.presetIndex < 0 || state.presetIndex >= static_cast<int>(state.presetItems.size()))
    {
        err = "invalid preset index";
        return false;
    }

    const std::string& presetName = state.presetItems[state.presetIndex];
    strncpy_s(state.presetName, sizeof(state.presetName), presetName.c_str(), _TRUNCATE);
    // GUIの編集状態をいったんAppConfig形式にそろえてから反映し、CLI経路と整合させる。
    AppConfig cfg{};
    if (!LoadPresetConfig(FindProjectRootPath(), presetName, cfg, err))
    {
        return false;
    }

    CopyPath(state.midiPath, sizeof(state.midiPath), cfg.midiPath);
    CopyPath(state.wavPath, sizeof(state.wavPath), cfg.wavPath);
    state.targetChannel = cfg.targetChannel;
    state.sampleRate = cfg.sampleRate;
    state.initialSeconds = cfg.initialSeconds;
    state.bits = cfg.bits;
    state.extraReleaseSec = static_cast<float>(cfg.extraReleaseSec);
    state.defaultWave = WaveToIndex(cfg.defaultWave);

    EnsureChannelConfigs(state);
    EnsureChannelMixStates(state);
    if (cfg.channelConfigs)
    {
        *state.channelConfigs = *cfg.channelConfigs;
    }
    if (cfg.channelMixStates)
    {
        *state.channelMixStates = *cfg.channelMixStates;
    }
    return true;
}

bool SavePresetDiffFromState(const GUIState& state, const std::filesystem::path& presetPath, std::string& err)
{
    if (!state.channelConfigs || !state.channelMixStates)
    {
        err = "channel configs or mix states are not initialized";
        return false;
    }
    GUIPresetSnapshot snapshot{};
    snapshot.midiPathUtf8 = state.midiPath;
    snapshot.wavPathUtf8 = state.wavPath;
    snapshot.channelConfigs = *state.channelConfigs;
    snapshot.channelMixStates = *state.channelMixStates;
    return SavePresetDiffFile(snapshot, presetPath, err);
}

void AnalyzeRenderPeakFromLogs(GUIState& state)
{
    std::lock_guard<std::mutex> lock(state.logMutex);
    const std::vector<std::string>& logs = LogsByTab(state, state.uiModeTab);
    for (auto it = logs.rbegin(); it != logs.rend(); ++it)
    {
        const std::string& line = *it;
        const std::string key = "[RenderStats] peak=";
        const size_t pos = line.find(key);
        if (pos == std::string::npos)
        {
            continue;
        }
        const size_t start = pos + key.size();
        size_t end = start;
        while (end < line.size() && (std::isdigit(static_cast<unsigned char>(line[end])) || line[end] == '.' || line[end] == '-'))
        {
            end++;
        }
        if (end <= start)
        {
            break;
        }
        try
        {
            state.lastPeak = std::stod(line.substr(start, end - start));
            state.hasPeak = true;
        }
        catch (...)
        {
            state.hasPeak = false;
        }
        return;
    }
}

void ActivateSoloPreview(GUIState& state, int channel)
{
    EnsureChannelMixStates(state);
    channel = std::clamp(channel, 0, 15);
    if (!state.soloPreviewActive)
    {
        state.soloPreviewBackup = *state.channelMixStates;
    }
    // Preview時は対象chのみsolo化し、完了時にbackupから復元する。
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

void StartGUIRun(GUIState& state, bool previewSelected)
{
    std::string validationError;
    if (!ValidateBeforeRun(state, validationError))
    {
        state.hasRun = true;
        state.lastRunExitCode = 1;
        AppendGUILog(state, "[GUI] Validation failed: " + validationError);
        return;
    }

    if (previewSelected)
    {
        ActivateSoloPreview(state, state.selectedChannel);
    }
    if (state.playback.playing.load(std::memory_order_relaxed))
    {
        StopPreviewAudio(state.playback);
        AppendGUILog(state, "[GUI] Previous preview playback stopped for new run");
    }

    // GUI編集値 -> AppConfig 変換をここに集約し、実行コア側でGUI依存を持たせない。
    AppConfig cfg = BuildConfigFromGUI(state);
    int overrideTicksPerQuarter = 0;
    cfg.overrideNoteTicks = BuildOverrideNoteTicksFromPianoRoll(state, overrideTicksPerQuarter);
    cfg.overrideTicksPerQuarter = overrideTicksPerQuarter;
    RenderOptions options = previewSelected ? DefaultPreviewRenderOptions() : DefaultRenderOptions();
    if (!previewSelected && state.serialSave)
    {
        cfg.wavPath = BuildSerialWavPath(cfg.wavPath);
    }
    if (previewSelected)
    {
        state.restorePreviewOnRunComplete = true;
        options.writeWav = false;
        const double rangeDurationSec = PreviewRangeDurationSec(state);
        if (state.pianoRoll.previewRangeEnabled && rangeDurationSec > 0.0)
        {
            // 範囲指定時は既定8秒上限ではなく範囲長を使う。
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

    state.runLogTab = state.uiModeTab;
    state.observer.logs = &LogsByTab(state, state.runLogTab);
    {
        std::lock_guard<std::mutex> lock(state.logMutex);
        LogsByTab(state, state.runLogTab).clear();
    }
    state.lastPeak = 0.0;
    state.hasPeak = false;
    state.runOutputBuffer = previewSelected ? std::make_shared<SoundData>() : nullptr;
    state.runIsPreview = previewSelected;
    state.autoPlayPreviewOnRunComplete = previewSelected;
    AppendGUILogToTab(state, state.runLogTab, previewSelected ? "[GUI] Preview Play started" : "[GUI] Export started");
    if (cfg.overrideNoteTicks != nullptr)
    {
        AppendGUILogToTab(state, state.runLogTab, "[GUI] PianoRoll edited notes applied: count=" +
            std::to_string(cfg.overrideNoteTicks->size() / 2));
    }
    if (previewSelected)
    {
        if (state.pianoRoll.previewRangeEnabled)
        {
            const int a = (std::min)(state.pianoRoll.previewRangeStartTick, state.pianoRoll.previewRangeEndTick);
            const int b = (std::max)(state.pianoRoll.previewRangeStartTick, state.pianoRoll.previewRangeEndTick);
            AppendGUILogToTab(state, state.runLogTab, "[GUI] Preview range tick=" + std::to_string(a) + "-" +
                std::to_string(b) + " secStart=" + std::to_string(options.startSec) +
                " secDuration=" + std::to_string(options.durationSec));
        }
        else
        {
            AppendGUILogToTab(state, state.runLogTab, "[GUI] Preview start tick=" + std::to_string(state.pianoRoll.previewStartTick) +
                " sec=" + std::to_string(options.startSec));
        }
    }
    AppendGUILogToTab(state, state.runLogTab, "[GUI] Effective Output: " + state.lastOutputPath);
    state.hasRun = false;
    state.stopRequested.store(false, std::memory_order_relaxed);
    state.running = true;
    state.runFuture = std::async(std::launch::async, [cfg, options, outBuffer = state.runOutputBuffer, &state]() {
        return Run(cfg, options, &state.observer, outBuffer.get());
        });
}

void StopGUIRunAndPreview(GUIState& state)
{
    // Stop後にRun完了通知が到着しても、自動再生を再開しないようにする。
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
    AppendGUILogToTab(state, state.runLogTab, std::string("[GUI] Run finished: exit=") + std::to_string(state.lastRunExitCode));
    if (state.runIsPreview)
    {
        // Preview成功時のみメモリバッファを再生可能状態に切り替える。
        if (state.lastRunExitCode == 0 &&
            state.runOutputBuffer != nullptr &&
            !state.runOutputBuffer->data.empty())
        {
            if (state.pianoRoll.previewRangeEnabled)
            {
                TrimPreviewSoundByDuration(*state.runOutputBuffer, state.previewRequestedDurationSec);
            }
            if (state.runOutputBuffer->data.empty())
            {
                state.previewAudioReady = false;
                state.previewRenderedSound.reset();
                AppendGUILogToTab(state, state.runLogTab, "[GUI] Preview range is too short: no audio");
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
            AppendGUILogToTab(state, state.runLogTab, "[GUI] Preview audio buffer ready");
            if (state.autoPlayPreviewOnRunComplete)
            {
                std::string playErr;
                state.playback.playStartTick.store(state.previewRequestedStartTick, std::memory_order_relaxed);
                if (PlayPreviewAudio(state.playback, *state.previewRenderedSound, state.previewLoop, playErr))
                {
                    AppendGUILogToTab(state, state.runLogTab, "[GUI] Preview playback started");
                }
                else
                {
                    AppendGUILogToTab(state, state.runLogTab, "[GUI] Preview playback failed: " + playErr);
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
