#pragma once

#include <functional>

#include "gui/GUIState.h"

namespace gui
{
bool DrawChannelEditor(
    GUIState& state,
    bool showSourceTypeSelector = true,
    const std::function<void(const char* what, const char* impact, const char* caution)>& updateHoverHelp = {});
} // namespace gui
