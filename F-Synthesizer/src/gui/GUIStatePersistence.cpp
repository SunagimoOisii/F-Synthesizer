#include "gui/GUIStatePersistence.h"

#include <algorithm>
#include <cstring>

#include "AppCore.h"
#include "gui/GUIStateModel.h"
#include "gui/GUIStateStorage.h"

namespace
{
std::filesystem::path PianoRollProjectPath()
{
    return FindProjectRootPath() / "config" / "piano_roll_project.json";
}

GUIStateStorageData BuildStateStorageData(const GUIState& state)
{
    // GUI実行状態から、永続化対象だけを中間形式へ変換する。
    GUIStateStorageData data{};
    data.midiPath = state.midiPath;
    data.wavPath = state.wavPath;
    data.targetChannel = state.targetChannel;
    data.sampleRate = state.sampleRate;
    data.initialSeconds = state.initialSeconds;
    data.bits = state.bits;
    data.extraReleaseSec = state.extraReleaseSec;
    data.fxBitCrusherBits = state.masterEffects.bitCrusher.bits;
    data.fxSampleRateReducerRatio = static_cast<float>(state.masterEffects.sampleRateReducer.ratio);
    data.fxChorusEnabled = state.masterEffects.chorus.enabled;
    data.fxChorusMix = static_cast<float>(state.masterEffects.chorus.mix);
    data.fxChorusBaseDelayMs = static_cast<float>(state.masterEffects.chorus.baseDelayMs);
    data.fxChorusDepthMs = static_cast<float>(state.masterEffects.chorus.depthMs);
    data.fxChorusRateHz = static_cast<float>(state.masterEffects.chorus.rateHz);
    data.fxChorusFeedback = static_cast<float>(state.masterEffects.chorus.feedback);
    data.fxFlangerEnabled = state.masterEffects.flanger.enabled;
    data.fxFlangerMix = static_cast<float>(state.masterEffects.flanger.mix);
    data.fxFlangerBaseDelayMs = static_cast<float>(state.masterEffects.flanger.baseDelayMs);
    data.fxFlangerDepthMs = static_cast<float>(state.masterEffects.flanger.depthMs);
    data.fxFlangerRateHz = static_cast<float>(state.masterEffects.flanger.rateHz);
    data.fxFlangerFeedback = static_cast<float>(state.masterEffects.flanger.feedback);
    data.fxDelayEnabled = state.masterEffects.delay.enabled;
    data.fxDelayMix = static_cast<float>(state.masterEffects.delay.mix);
    data.fxDelayTimeSec = static_cast<float>(state.masterEffects.delay.timeSec);
    data.fxDelayFeedback = static_cast<float>(state.masterEffects.delay.feedback);
    data.fxDelayTempoSync = state.masterEffects.delay.tempoSync;
    data.fxDelaySyncBeats = static_cast<float>(state.masterEffects.delay.syncBeats);
    data.fxReverbEnabled = state.masterEffects.reverb.enabled;
    data.fxReverbMix = static_cast<float>(state.masterEffects.reverb.mix);
    data.fxReverbRoomSize = static_cast<float>(state.masterEffects.reverb.roomSize);
    data.fxReverbDamping = static_cast<float>(state.masterEffects.reverb.damping);
    data.UIScaleIndex = state.UIScaleIndex;
    data.UIModeTab = state.UIModeTab;
    data.UIThemeIndex = state.UIThemeIndex;
    data.themeFxScanline = state.themeFxScanline;
    data.themeFxDotMask = state.themeFxDotMask;
    data.themeFxVignette = state.themeFxVignette;
    data.logPanelHeight = state.logPanelHeight;
    data.presetIndex = state.presetIndex;
    data.serialSave = state.serialSave;
    data.previewLoop = state.previewLoop;
    data.autoTonePreviewEnabled = state.autoTonePreviewEnabled;
    data.selectedSoundSlot = state.selectedSoundSlot;
    data.selectedDrumNote = state.selectedDrumNote;
    data.tonePreviewNoteNumber = state.tonePreviewNoteNumber;
    data.chordModeEnabled = state.chordModeEnabled;
    data.chordType = state.chordType;
    data.presetName = state.presetName;
    data.lastPresetPath = state.lastPresetPath;
    data.prDisplayChannel = state.pianoRoll.displayChannel;
    data.prSnapIndex = state.pianoRoll.snapIndex;
    data.prPixelsPerQuarter = state.pianoRoll.pixelsPerQuarter;
    data.prTickOffset = state.pianoRoll.tickOffset;
    data.prNoteOffset = state.pianoRoll.noteOffset;
    data.prVisibleNoteCount = state.pianoRoll.visibleNoteCount;
    data.prDrumNameMode = state.pianoRoll.drumNameMode;
    data.prFollowPreviewPlayback = state.pianoRoll.followPreviewPlayback;
    data.prPreviewStartTick = state.pianoRoll.previewStartTick;
    data.stepSeqViewActive = state.stepSeq.viewActive;
    for (int r = 0; r < GUIStepSeqState::kRows; ++r)
    {
        uint16_t bits = 0;
        for (int s = 0; s < GUIStepSeqState::kSteps; ++s)
        {
            if (state.stepSeq.steps[r][s])
            {
                bits |= static_cast<uint16_t>(1u << s);
            }
        }
        data.stepSeqStepBits[r] = bits;
        data.stepSeqVelocity[r] = state.stepSeq.velocity[r];
    }
    data.channelAssignments = state.channelAssignments;
    data.drumChannelSpecialHandling = state.drumChannelSpecialHandling;
    data.prSelectedIndices.clear();
    for (int i = 0; i < static_cast<int>(state.pianoRoll.selected.size()); i++)
    {
        if (state.pianoRoll.selected[static_cast<size_t>(i)] != 0)
        {
            data.prSelectedIndices.push_back(i);
        }
    }
    for (int ch = 0; ch < 16; ch++)
    {
        data.channelMixStates[ch] = (state.channelMixStates != nullptr)
            ? (*state.channelMixStates)[ch]
            : ChannelMixState{};
    }
    return data;
}

void ApplyStateStorageData(GUIState& state, const GUIStateStorageData& data)
{
    // 復元は storage -> GUIState の一方向適用に限定し、初期化責務を混ぜない。
    strncpy_s(state.midiPath, sizeof(state.midiPath), data.midiPath.c_str(), _TRUNCATE);
    strncpy_s(state.wavPath, sizeof(state.wavPath), data.wavPath.c_str(), _TRUNCATE);
    state.targetChannel = data.targetChannel;
    state.sampleRate = data.sampleRate;
    state.initialSeconds = data.initialSeconds;
    state.bits = data.bits;
    state.extraReleaseSec = data.extraReleaseSec;
    state.masterEffects.bitCrusher.bits = data.fxBitCrusherBits;
    state.masterEffects.sampleRateReducer.ratio = data.fxSampleRateReducerRatio;
    state.masterEffects.chorus.enabled = data.fxChorusEnabled;
    state.masterEffects.chorus.mix = data.fxChorusMix;
    state.masterEffects.chorus.baseDelayMs = data.fxChorusBaseDelayMs;
    state.masterEffects.chorus.depthMs = data.fxChorusDepthMs;
    state.masterEffects.chorus.rateHz = data.fxChorusRateHz;
    state.masterEffects.chorus.feedback = data.fxChorusFeedback;
    state.masterEffects.flanger.enabled = data.fxFlangerEnabled;
    state.masterEffects.flanger.mix = data.fxFlangerMix;
    state.masterEffects.flanger.baseDelayMs = data.fxFlangerBaseDelayMs;
    state.masterEffects.flanger.depthMs = data.fxFlangerDepthMs;
    state.masterEffects.flanger.rateHz = data.fxFlangerRateHz;
    state.masterEffects.flanger.feedback = data.fxFlangerFeedback;
    state.masterEffects.delay.enabled = data.fxDelayEnabled;
    state.masterEffects.delay.mix = data.fxDelayMix;
    state.masterEffects.delay.timeSec = data.fxDelayTimeSec;
    state.masterEffects.delay.feedback = data.fxDelayFeedback;
    state.masterEffects.delay.tempoSync = data.fxDelayTempoSync;
    state.masterEffects.delay.syncBeats = data.fxDelaySyncBeats;
    state.masterEffects.reverb.enabled = data.fxReverbEnabled;
    state.masterEffects.reverb.mix = data.fxReverbMix;
    state.masterEffects.reverb.roomSize = data.fxReverbRoomSize;
    state.masterEffects.reverb.damping = data.fxReverbDamping;
    state.UIScaleIndex = data.UIScaleIndex;
    state.UIModeTab = data.UIModeTab;
    state.UIThemeIndex = data.UIThemeIndex;
    state.themeFxScanline = data.themeFxScanline;
    state.themeFxDotMask = data.themeFxDotMask;
    state.themeFxVignette = data.themeFxVignette;
    state.logPanelHeight = data.logPanelHeight;
    state.presetIndex = data.presetIndex;
    state.serialSave = data.serialSave;
    state.previewLoop = data.previewLoop;
    state.autoTonePreviewEnabled = data.autoTonePreviewEnabled;
    state.selectedSoundSlot = data.selectedSoundSlot;
    state.selectedDrumNote = data.selectedDrumNote;
    state.tonePreviewNoteNumber = std::clamp(data.tonePreviewNoteNumber, 0, 127);
    state.chordModeEnabled = data.chordModeEnabled;
    state.chordType = std::clamp(data.chordType, 0, 4);
    strncpy_s(state.presetName, sizeof(state.presetName), data.presetName.c_str(), _TRUNCATE);
    state.lastPresetPath = data.lastPresetPath;
    state.pianoRoll.displayChannel = data.prDisplayChannel;
    state.pianoRoll.snapIndex = data.prSnapIndex;
    state.pianoRoll.pixelsPerQuarter = data.prPixelsPerQuarter;
    state.pianoRoll.tickOffset = data.prTickOffset;
    state.pianoRoll.noteOffset = data.prNoteOffset;
    state.pianoRoll.visibleNoteCount = data.prVisibleNoteCount;
    state.pianoRoll.drumNameMode = data.prDrumNameMode;
    state.pianoRoll.followPreviewPlayback = data.prFollowPreviewPlayback;
    state.pianoRoll.previewStartTick = data.prPreviewStartTick;
    state.stepSeq.viewActive = data.stepSeqViewActive;
    for (int r = 0; r < GUIStepSeqState::kRows; ++r)
    {
        const uint16_t bits = data.stepSeqStepBits[r];
        for (int s = 0; s < GUIStepSeqState::kSteps; ++s)
        {
            state.stepSeq.steps[r][s] = ((bits >> s) & 1u) != 0;
        }
        state.stepSeq.velocity[r] = std::clamp(data.stepSeqVelocity[r], 1, 127);
    }
    state.pianoRoll.pendingSelectedIndices = data.prSelectedIndices;
    state.channelAssignments = data.channelAssignments;
    state.drumChannelSpecialHandling = data.drumChannelSpecialHandling;

    gui::EnsureChannelMixStates(state);
    for (int ch = 0; ch < 16; ch++)
    {
        (*state.channelMixStates)[ch] = data.channelMixStates[ch];
    }
}

PianoRollProjectStorageData BuildPianoRollProjectStorageData(const GUIState& state)
{
    PianoRollProjectStorageData data{};
    if (!state.pianoRoll.hasProjectData || state.pianoRoll.loadedMidiPath.empty() || state.pianoRoll.notes.empty())
    {
        return data;
    }

    data.midiPath = state.pianoRoll.loadedMidiPath.string();
    data.ticksPerQuarter = (state.pianoRoll.ticksPerQuarter > 0) ? state.pianoRoll.ticksPerQuarter : 480;
    data.notes.reserve(state.pianoRoll.notes.size());
    for (const auto& n : state.pianoRoll.notes)
    {
        PianoRollProjectStorageNote out{};
        out.startTick = n.startTick;
        out.endTick = n.endTick;
        out.note = n.note;
        out.channel = n.channel;
        out.velocity = n.velocity;
        data.notes.push_back(out);
    }
    return data;
}

void ApplyPianoRollProjectStorageData(GUIState& state, const PianoRollProjectStorageData& data)
{
    if (data.midiPath.empty() || data.notes.empty())
    {
        state.pianoRoll.hasProjectData = false;
        state.pianoRoll.projectMidiPath.clear();
        state.pianoRoll.projectNotes.clear();
        state.pianoRoll.projectTicksPerQuarter = 0;
        return;
    }

    state.pianoRoll.projectMidiPath = std::filesystem::path(data.midiPath);
    state.pianoRoll.projectTicksPerQuarter = (data.ticksPerQuarter > 0) ? data.ticksPerQuarter : 480;
    state.pianoRoll.projectNotes.clear();
    state.pianoRoll.projectNotes.reserve(data.notes.size());
    for (const auto& n : data.notes)
    {
        gui::PianoRollNote out{};
        out.startTick = n.startTick;
        out.endTick = n.endTick;
        out.note = n.note;
        out.channel = n.channel;
        out.velocity = n.velocity;
        state.pianoRoll.projectNotes.push_back(out);
    }
    state.pianoRoll.hasProjectData = true;
}
} // namespace

namespace gui
{
std::filesystem::path GUIStatePath()
{
    return FindProjectRootPath() / "config" / "gui_state.json";
}

bool LoadGUIStateFile(GUIState& state, std::string& err)
{
    // 読込は現在値を初期値として渡し、不足キーは既定値を維持する。
    GUIStateStorageData data = BuildStateStorageData(state);
    if (!LoadGUIStateStorageFile(GUIStatePath(), data, err))
    {
        return false;
    }
    ApplyStateStorageData(state, data);

    PianoRollProjectStorageData projectData{};
    std::string projectErr;
    if (!LoadPianoRollProjectStorageFile(PianoRollProjectPath(), projectData, projectErr))
    {
        // GUI本体とPianoRollプロジェクトの片側だけ復元された状態を避けるため失敗で返す。
        err = projectErr;
        return false;
    }
    ApplyPianoRollProjectStorageData(state, projectData);
    return true;
}

bool SaveGUIStateFile(const GUIState& state, std::string& err)
{
    const GUIStateStorageData data = BuildStateStorageData(state);
    if (!SaveGUIStateStorageFile(GUIStatePath(), data, err))
    {
        return false;
    }

    const PianoRollProjectStorageData projectData = BuildPianoRollProjectStorageData(state);
    if (!SavePianoRollProjectStorageFile(PianoRollProjectPath(), projectData, err))
    {
        return false;
    }
    return true;
}
} // namespace gui
