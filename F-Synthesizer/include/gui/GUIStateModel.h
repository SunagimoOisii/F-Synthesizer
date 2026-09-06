#pragma once

#include <functional>

#include "gui/GUIState.h"

namespace gui
{
void InitializeGUIState(
    GUIState& state,
    const std::function<void(const std::string&)>& refreshPresetItems);
void RepairGUIStatePaths(
    GUIState& state,
    const std::function<void(const std::string&)>& refreshPresetItems,
    const std::function<void(const std::string&)>& appendLog);
} // namespace gui
