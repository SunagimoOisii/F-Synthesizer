#include "gui/GUIStatePersistence.h"

#include <cstring>

#include "AppCore.h"
#include "gui/GUIStateModel.h"
#include "gui/GUIStateStorage.h"

namespace
{
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
    data.defaultWave = state.defaultWave;
    data.uiScaleIndex = state.uiScaleIndex;
    data.logPanelHeight = state.logPanelHeight;
    data.presetIndex = state.presetIndex;
    data.serialSave = state.serialSave;
    data.previewLoop = state.previewLoop;
    data.selectedChannel = state.selectedChannel;
    data.selectedDrumNote = state.selectedDrumNote;
    data.presetName = state.presetName;
    data.lastPresetPath = state.lastPresetPath;
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
    state.defaultWave = data.defaultWave;
    state.uiScaleIndex = data.uiScaleIndex;
    state.logPanelHeight = data.logPanelHeight;
    state.presetIndex = data.presetIndex;
    state.serialSave = data.serialSave;
    state.previewLoop = data.previewLoop;
    state.selectedChannel = data.selectedChannel;
    state.selectedDrumNote = data.selectedDrumNote;
    strncpy_s(state.presetName, sizeof(state.presetName), data.presetName.c_str(), _TRUNCATE);
    state.lastPresetPath = data.lastPresetPath;

    gui::EnsureChannelMixStates(state);
    for (int ch = 0; ch < 16; ch++)
    {
        (*state.channelMixStates)[ch] = data.channelMixStates[ch];
    }
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
    return true;
}

bool SaveGUIStateFile(const GUIState& state, std::string& err)
{
    const GUIStateStorageData data = BuildStateStorageData(state);
    return SaveGUIStateStorageFile(GUIStatePath(), data, err);
}
} // namespace gui
