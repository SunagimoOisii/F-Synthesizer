#pragma once

#include <functional>

#include "gui/GUIState.h"

namespace gui
{
bool DrawChannelEditor(
    GUIState& state,
    const std::function<void(const char* what, const char* impact, const char* caution)>& updateHoverHelp = {});
} // namespace gui
