#pragma once

#include "AppCore.h"
#include "gui/GUIState.h"

namespace gui
{
void EnsureChannelConfigs(GUIState& state);
void EnsureChannelMixStates(GUIState& state);
AppConfig BuildConfigFromGUI(const GUIState& state);
} // namespace gui
