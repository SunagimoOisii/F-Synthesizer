#pragma once

#include <string>
#include <vector>

#include "gui/GUIState.h"

namespace gui::detail
{
std::vector<std::string>& LogsByTab(GUIState& state, int tab);
const std::vector<std::string>& LogsByTab(const GUIState& state, int tab);
void AppendGUILogToTab(GUIState& state, int tab, const std::string& line);
} // namespace gui::detail
