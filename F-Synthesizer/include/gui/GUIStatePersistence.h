#pragma once

#include <filesystem>
#include <string>

#include "gui/GUIState.h"

namespace gui
{
std::filesystem::path GUIStatePath();
bool LoadGUIStateFile(GUIState& state, std::string& err);
bool SaveGUIStateFile(const GUIState& state, std::string& err);
} // namespace gui
