#include "gui/GUIStatePersistence.h"

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
    data.assetReferenceMode = state.assetReferenceMode;
    data.showReferenceAdvanced = state.showReferenceAdvanced;
    data.sampleRate = state.sampleRate;
    data.initialSeconds = state.initialSeconds;
    data.bits = state.bits;
    data.extraReleaseSec = state.extraReleaseSec;
    data.defaultWave = state.defaultWave;
    data.uiScaleIndex = state.uiScaleIndex;
    data.uiModeTab = state.uiModeTab;
    data.logPanelHeight = state.logPanelHeight;
    data.presetIndex = state.presetIndex;
    data.serialSave = state.serialSave;
    data.previewLoop = state.previewLoop;
    data.selectedChannel = state.selectedChannel;
    data.selectedDrumNote = state.selectedDrumNote;
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
    state.assetReferenceMode = data.assetReferenceMode;
    state.showReferenceAdvanced = data.showReferenceAdvanced;
    state.sampleRate = data.sampleRate;
    state.initialSeconds = data.initialSeconds;
    state.bits = data.bits;
    state.extraReleaseSec = data.extraReleaseSec;
    state.defaultWave = data.defaultWave;
    state.uiScaleIndex = data.uiScaleIndex;
    state.uiModeTab = data.uiModeTab;
    state.logPanelHeight = data.logPanelHeight;
    state.presetIndex = data.presetIndex;
    state.serialSave = data.serialSave;
    state.previewLoop = data.previewLoop;
    state.selectedChannel = data.selectedChannel;
    state.selectedDrumNote = data.selectedDrumNote;
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
