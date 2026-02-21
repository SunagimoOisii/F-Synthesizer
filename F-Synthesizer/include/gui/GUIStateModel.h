#pragma once

#include <functional>

#include "AppCore.h"
#include "gui/GUIState.h"

namespace gui
{
void EnsureChannelConfigs(GUIState& state);
void EnsureChannelMixStates(GUIState& state);
AppConfig BuildConfigFromGUI(const GUIState& state);
void InitializeGUIState(
    GUIState& state,
    const std::function<void(const std::string&)>& refreshPresetItems);
void RepairGUIStatePaths(
    GUIState& state,
    const std::function<void(const std::string&)>& refreshPresetItems,
    const std::function<void(const std::string&)>& appendLog);
} // namespace gui
