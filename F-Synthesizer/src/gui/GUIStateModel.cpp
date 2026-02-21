#include "gui/GUIStateModel.h"

#include <algorithm>
#include <cstring>

#include "gui/GUIConfigUtils.h"
#include "gui/GUIPlatform.h"
#include "gui/PreviewAudio.h"
#include "io/PlatformPaths.h"

namespace gui
{
void EnsureChannelConfigs(GUIState& state)
{
    if (state.channelConfigs)
    {
        return;
    }
    // GUI状態が欠けているときは既定Configから埋め、nullの分岐を残さない。
    AppConfig cfg = DefaultConfig();
    state.channelConfigs = std::make_shared<std::array<ChannelConfig, 16>>();
    if (cfg.channelConfigs)
    {
        *state.channelConfigs = *cfg.channelConfigs;
    }
}

void EnsureChannelMixStates(GUIState& state)
{
    if (state.channelMixStates)
    {
        return;
    }
    AppConfig cfg = DefaultConfig();
    state.channelMixStates = std::make_shared<std::array<ChannelMixState, 16>>();
    if (cfg.channelMixStates)
    {
        *state.channelMixStates = *cfg.channelMixStates;
    }
}

AppConfig BuildConfigFromGUI(const GUIState& state)
{
    // GUI入力をRun境界のAppConfig形式にそろえる唯一の変換点。
    AppConfig cfg = DefaultConfig();
    cfg.midiPath = Utf8ToPath(state.midiPath);
    cfg.wavPath = Utf8ToPath(state.wavPath);
    cfg.targetChannel = state.targetChannel;
    cfg.sampleRate = state.sampleRate;
    cfg.initialSeconds = state.initialSeconds;
    cfg.bits = state.bits;
    cfg.extraReleaseSec = state.extraReleaseSec;
    cfg.defaultWave = WaveFromIndex(state.defaultWave);
    if (state.channelConfigs)
    {
        cfg.channelConfigs = std::static_pointer_cast<const std::array<ChannelConfig, 16>>(state.channelConfigs);
    }
    if (state.channelMixStates)
    {
        cfg.channelMixStates = std::static_pointer_cast<const std::array<ChannelMixState, 16>>(state.channelMixStates);
    }
    return cfg;
}

void InitializeGUIState(
    GUIState& state,
    const std::function<void(const std::string&)>& refreshPresetItems)
{
    StopPreviewAudio(state.playback);
    AppConfig cfg = DefaultConfig();
    CopyPath(state.midiPath, sizeof(state.midiPath), cfg.midiPath);
    CopyPath(state.wavPath, sizeof(state.wavPath), cfg.wavPath);
    state.targetChannel = cfg.targetChannel;
    state.sampleRate = cfg.sampleRate;
    state.initialSeconds = cfg.initialSeconds;
    state.bits = cfg.bits;
    state.extraReleaseSec = static_cast<float>(cfg.extraReleaseSec);
    state.defaultWave = 2;
    state.uiScaleIndex = 1;
    state.logPanelHeight = 240.0f;
    state.presetIndex = 0;
    state.selectedChannel = 0;
    state.selectedDrumNote = 36;
    strncpy_s(state.presetName, sizeof(state.presetName), "basic_wave", _TRUNCATE);
    state.running = false;
    state.stopRequested.store(false, std::memory_order_relaxed);
    state.hasRun = false;
    state.lastRunExitCode = 0;
    state.serialSave = false;
    state.lastOutputPath.clear();
    state.lastPresetPath.clear();
    state.logs.clear();
    state.lastPeak = 0.0;
    state.hasPeak = false;
    state.soloPreviewActive = false;
    state.restorePreviewOnRunComplete = false;
    state.soloPreviewChannel = 0;
    state.previewLoop = false;
    state.previewAudioReady = false;
    state.runIsPreview = false;
    state.autoPlayPreviewOnRunComplete = false;
    state.previewRenderedSound.reset();
    state.runOutputBuffer.reset();
    state.pianoRoll = gui::PianoRollState{};
    state.observer.logMutex = &state.logMutex;
    state.observer.logs = &state.logs;
    state.observer.cancelRequested = &state.stopRequested;
    if (refreshPresetItems)
    {
        refreshPresetItems(state.presetName);
    }

    state.channelConfigs = std::make_shared<std::array<ChannelConfig, 16>>();
    if (cfg.channelConfigs)
    {
        *state.channelConfigs = *cfg.channelConfigs;
    }
    state.channelMixStates = std::make_shared<std::array<ChannelMixState, 16>>();
    if (cfg.channelMixStates)
    {
        *state.channelMixStates = *cfg.channelMixStates;
    }
    state.soloPreviewBackup = *state.channelMixStates;
}

void RepairGUIStatePaths(
    GUIState& state,
    const std::function<void(const std::string&)>& refreshPresetItems,
    const std::function<void(const std::string&)>& appendLog)
{
    const AppConfig def = DefaultConfig();
    const std::filesystem::path midi = Utf8ToPath(state.midiPath);
    const std::filesystem::path wav = Utf8ToPath(state.wavPath);

    // 保存状態の破損/旧形式を想定し、実行可能な範囲に丸めて復旧する。
    bool repaired = false;
    if (!std::filesystem::exists(midi))
    {
        CopyPath(state.midiPath, sizeof(state.midiPath), def.midiPath);
        repaired = true;
    }
    if (wav.extension().empty() || std::filesystem::is_directory(wav))
    {
        CopyPath(state.wavPath, sizeof(state.wavPath), def.wavPath);
        repaired = true;
    }
    if (state.targetChannel < -1 || state.targetChannel > 15)
    {
        state.targetChannel = def.targetChannel;
        repaired = true;
    }
    if (state.presetItems.empty())
    {
        if (refreshPresetItems)
        {
            refreshPresetItems(state.presetName);
        }
    }
    if (state.presetIndex < 0 || state.presetIndex >= static_cast<int>(state.presetItems.size()))
    {
        if (refreshPresetItems)
        {
            refreshPresetItems(state.presetName);
        }
        repaired = true;
    }
    if (state.sampleRate <= 0)
    {
        state.sampleRate = def.sampleRate;
        repaired = true;
    }
    if (state.initialSeconds <= 0)
    {
        state.initialSeconds = def.initialSeconds;
        repaired = true;
    }
    if (state.bits != 16)
    {
        state.bits = 16;
        repaired = true;
    }
    if (state.uiScaleIndex < 0 || state.uiScaleIndex > 2)
    {
        state.uiScaleIndex = 1;
        repaired = true;
    }
    if (state.logPanelHeight < 140.0f || state.logPanelHeight > 520.0f)
    {
        state.logPanelHeight = std::clamp(state.logPanelHeight, 140.0f, 520.0f);
        repaired = true;
    }
    if (state.pianoRoll.displayChannel < 0 || state.pianoRoll.displayChannel > 15)
    {
        state.pianoRoll.displayChannel = std::clamp(state.pianoRoll.displayChannel, 0, 15);
        repaired = true;
    }
    if (state.pianoRoll.snapIndex < 0 || state.pianoRoll.snapIndex > 4)
    {
        state.pianoRoll.snapIndex = std::clamp(state.pianoRoll.snapIndex, 0, 4);
        repaired = true;
    }
    if (state.pianoRoll.pixelsPerQuarter < 16.0f || state.pianoRoll.pixelsPerQuarter > 240.0f)
    {
        state.pianoRoll.pixelsPerQuarter = std::clamp(state.pianoRoll.pixelsPerQuarter, 16.0f, 240.0f);
        repaired = true;
    }
    if (state.pianoRoll.tickOffset < 0)
    {
        state.pianoRoll.tickOffset = 0;
        repaired = true;
    }
    if (state.pianoRoll.noteOffset < 0 || state.pianoRoll.noteOffset > 116)
    {
        state.pianoRoll.noteOffset = std::clamp(state.pianoRoll.noteOffset, 0, 116);
        repaired = true;
    }
    if (state.pianoRoll.visibleNoteCount < 12 || state.pianoRoll.visibleNoteCount > 72)
    {
        state.pianoRoll.visibleNoteCount = std::clamp(state.pianoRoll.visibleNoteCount, 12, 72);
        repaired = true;
    }
    if (state.pianoRoll.previewStartTick < 0)
    {
        state.pianoRoll.previewStartTick = 0;
        repaired = true;
    }

    EnsureChannelMixStates(state);
    for (int ch = 0; ch < 16; ch++)
    {
        ChannelMixState& mix = (*state.channelMixStates)[ch];
        bool mixRepaired = false;
        if (mix.level < 0.0 || mix.level > 2.0)
        {
            mix.level = std::clamp(mix.level, 0.0, 2.0);
            mixRepaired = true;
        }
        if (mix.pan < -1.0 || mix.pan > 1.0)
        {
            mix.pan = std::clamp(mix.pan, -1.0, 1.0);
            mixRepaired = true;
        }
        if (mix.gain < 0.0 || mix.gain > 4.0)
        {
            mix.gain = std::clamp(mix.gain, 0.0, 4.0);
            mixRepaired = true;
        }
        if (mixRepaired)
        {
            repaired = true;
            if (appendLog)
            {
                appendLog("[GUI] Invalid mix state detected and clamped: ch" + std::to_string(ch));
            }
        }
    }

    if (repaired && appendLog)
    {
        appendLog("[GUI] Detected invalid saved state. Recovered to safe defaults.");
    }
}
} // namespace gui
