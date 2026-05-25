#include "gui/GUIStateModel.h"

#include <algorithm>
#include <cstring>

#include "gui/GUIConfigUtils.h"
#include "gui/GUIPlatform.h"
#include "gui/GUIProjectFacade.h"
#include "gui/PreviewAudio.h"
#include "io/PlatformPaths.h"

namespace gui
{
void EnsureSoundSlots(GUIState& state)
{
    (void)MutableSoundSlots(state);
}

void EnsureChannelMixStates(GUIState& state)
{
    (void)MutableChannelMixStates(state);
}

void InitializeGUIState(
    GUIState& state,
    const std::function<void(const std::string&)>& refreshPresetItems)
{
    StopPreviewAudio(state.playback);
    ApplyProjectModelToGUI(state, DefaultProjectModel());
    state.UIScaleIndex = 1;
    state.UIModeTab = 1;
    state.UIThemeIndex = 0;
    state.logPanelHeight = 240.0f;
    state.presetIndex = 0;
    state.selectedSoundSlot = 0;
    state.selectedDrumNote = 36;
    state.tonePreviewNoteNumber = 60;
    state.channelAssignments = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    state.drumChannelSpecialHandling = true;
    strncpy_s(state.presetName, sizeof(state.presetName), "sound_lead_blade", _TRUNCATE);
    state.running = false;
    state.stopRequested.store(false, std::memory_order_relaxed);
    state.hasRun = false;
    state.lastRunExitCode = 0;
    state.serialSave = false;
    state.lastOutputPath.clear();
    state.lastPresetPath.clear();
    state.soundLogs.clear();
    state.musicLogs.clear();
    state.exportLogs.clear();
    state.recentWavPaths.clear();
    state.runLogTab = state.UIModeTab;
    state.lastPeak = 0.0;
    state.hasPeak = false;
    state.soloPreviewActive = false;
    state.restorePreviewOnRunComplete = false;
    state.soloPreviewChannel = 0;
    state.previewLoop = false;
    state.autoTonePreviewEnabled = false;
    state.autoTonePreviewPending = false;
    state.autoTonePreviewLastEditSec = 0.0;
    state.previewAudioReady = false;
    state.runIsPreview = false;
    state.pianoRoll = gui::PianoRollState{};
    state.stepSeq = GUIStepSeqState{};
    state.playEditingChannel = -1;
    state.observer.logMutex = &state.logMutex;
    state.observer.logs = &state.soundLogs;
    state.observer.cancelRequested = &state.stopRequested;
    if (refreshPresetItems)
    {
        refreshPresetItems(state.presetName);
    }

    state.soloPreviewBackup = MutableChannelMixStates(state);
}

void RepairGUIStatePaths(
    GUIState& state,
    const std::function<void(const std::string&)>& refreshPresetItems,
    const std::function<void(const std::string&)>& appendLog)
{
    const ProjectModel def = DefaultProjectModel();
    const std::filesystem::path midi = Utf8ToPath(state.midiPath);
    const std::filesystem::path wav = Utf8ToPath(state.wavPath);

    // 保存状態の破損/旧形式を想定し、実行可能な範囲に丸めて復旧する。
    bool repaired = false;
    std::error_code midiEc;
    const bool midiExists = std::filesystem::exists(midi, midiEc);
    if (!midiExists || midiEc)
    {
        if (midiEc && appendLog)
        {
            appendLog("[GUI] Failed to inspect restored MIDI path: " + midiEc.message());
        }
        CopyPath(state.midiPath, sizeof(state.midiPath), def.midiPath);
        repaired = true;
    }
    std::error_code wavDirEc;
    const bool wavIsDirectory = std::filesystem::is_directory(wav, wavDirEc);
    if (wav.extension().empty() || wavIsDirectory || wavDirEc)
    {
        if (wavDirEc && appendLog)
        {
            appendLog("[GUI] Failed to inspect restored WAV path: " + wavDirEc.message());
        }
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
    auto& fx = state.masterEffects;
    const int clampedCrusherBits = std::clamp(fx.bitCrusher.bits, 1, 16);
    if (fx.bitCrusher.bits != clampedCrusherBits)
    {
        fx.bitCrusher.bits = clampedCrusherBits;
        repaired = true;
    }
    const double clampedSampleRateRatio = std::clamp(fx.sampleRateReducer.ratio, 0.0, 1.0);
    if (fx.sampleRateReducer.ratio != clampedSampleRateRatio)
    {
        fx.sampleRateReducer.ratio = clampedSampleRateRatio;
        repaired = true;
    }
    const double clampedChorusMix = std::clamp(fx.chorus.mix, 0.0, 1.0);
    if (fx.chorus.mix != clampedChorusMix)
    {
        fx.chorus.mix = clampedChorusMix;
        repaired = true;
    }
    const double clampedChorusBaseDelayMs = std::clamp(fx.chorus.baseDelayMs, 2.0, 40.0);
    if (fx.chorus.baseDelayMs != clampedChorusBaseDelayMs)
    {
        fx.chorus.baseDelayMs = clampedChorusBaseDelayMs;
        repaired = true;
    }
    const double clampedChorusDepthMs = std::clamp(fx.chorus.depthMs, 0.0, 20.0);
    if (fx.chorus.depthMs != clampedChorusDepthMs)
    {
        fx.chorus.depthMs = clampedChorusDepthMs;
        repaired = true;
    }
    const double clampedChorusRateHz = std::clamp(fx.chorus.rateHz, 0.05, 8.0);
    if (fx.chorus.rateHz != clampedChorusRateHz)
    {
        fx.chorus.rateHz = clampedChorusRateHz;
        repaired = true;
    }
    const double clampedChorusFeedback = std::clamp(fx.chorus.feedback, 0.0, 0.9);
    if (fx.chorus.feedback != clampedChorusFeedback)
    {
        fx.chorus.feedback = clampedChorusFeedback;
        repaired = true;
    }
    const double clampedFlangerMix = std::clamp(fx.flanger.mix, 0.0, 1.0);
    if (fx.flanger.mix != clampedFlangerMix)
    {
        fx.flanger.mix = clampedFlangerMix;
        repaired = true;
    }
    const double clampedFlangerBaseDelayMs = std::clamp(fx.flanger.baseDelayMs, 0.1, 8.0);
    if (fx.flanger.baseDelayMs != clampedFlangerBaseDelayMs)
    {
        fx.flanger.baseDelayMs = clampedFlangerBaseDelayMs;
        repaired = true;
    }
    const double clampedFlangerDepthMs = std::clamp(fx.flanger.depthMs, 0.0, 5.0);
    if (fx.flanger.depthMs != clampedFlangerDepthMs)
    {
        fx.flanger.depthMs = clampedFlangerDepthMs;
        repaired = true;
    }
    const double clampedFlangerRateHz = std::clamp(fx.flanger.rateHz, 0.05, 8.0);
    if (fx.flanger.rateHz != clampedFlangerRateHz)
    {
        fx.flanger.rateHz = clampedFlangerRateHz;
        repaired = true;
    }
    const double clampedFlangerFeedback = std::clamp(fx.flanger.feedback, 0.0, 0.95);
    if (fx.flanger.feedback != clampedFlangerFeedback)
    {
        fx.flanger.feedback = clampedFlangerFeedback;
        repaired = true;
    }
    const double clampedDelayMix = std::clamp(fx.delay.mix, 0.0, 1.0);
    if (fx.delay.mix != clampedDelayMix)
    {
        fx.delay.mix = clampedDelayMix;
        repaired = true;
    }
    const double clampedDelayTimeSec = std::clamp(fx.delay.timeSec, 0.01, 2.0);
    if (fx.delay.timeSec != clampedDelayTimeSec)
    {
        fx.delay.timeSec = clampedDelayTimeSec;
        repaired = true;
    }
    const double clampedDelayFeedback = std::clamp(fx.delay.feedback, 0.0, 0.95);
    if (fx.delay.feedback != clampedDelayFeedback)
    {
        fx.delay.feedback = clampedDelayFeedback;
        repaired = true;
    }
    const double clampedDelaySyncBeats = std::clamp(fx.delay.syncBeats, 0.125, 4.0);
    if (fx.delay.syncBeats != clampedDelaySyncBeats)
    {
        fx.delay.syncBeats = clampedDelaySyncBeats;
        repaired = true;
    }
    const double clampedReverbMix = std::clamp(fx.reverb.mix, 0.0, 1.0);
    if (fx.reverb.mix != clampedReverbMix)
    {
        fx.reverb.mix = clampedReverbMix;
        repaired = true;
    }
    const double clampedReverbRoomSize = std::clamp(fx.reverb.roomSize, 0.1, 1.0);
    if (fx.reverb.roomSize != clampedReverbRoomSize)
    {
        fx.reverb.roomSize = clampedReverbRoomSize;
        repaired = true;
    }
    const double clampedReverbDamping = std::clamp(fx.reverb.damping, 0.0, 1.0);
    if (fx.reverb.damping != clampedReverbDamping)
    {
        fx.reverb.damping = clampedReverbDamping;
        repaired = true;
    }
    if (state.UIScaleIndex < 0 || state.UIScaleIndex > 2)
    {
        state.UIScaleIndex = 1;
        repaired = true;
    }
    if (state.UIModeTab < 0 || state.UIModeTab > 3)
    {
        state.UIModeTab = 1;
        repaired = true;
    }
    if (state.UIThemeIndex < 0 || state.UIThemeIndex > 1)
    {
        state.UIThemeIndex = 0;
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
    for (int ch = 0; ch < 16; ch++)
    {
        if (state.channelAssignments[ch] < 0 || state.channelAssignments[ch] > 15)
        {
            SetChannelAssignment(state, ch, state.channelAssignments[ch]);
            repaired = true;
        }
    }

    EnsureChannelMixStates(state);
    for (int ch = 0; ch < 16; ch++)
    {
        ChannelMixState& mix = MutableChannelMix(state, ch);
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
