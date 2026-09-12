#pragma once

#include <functional>

#include "SynthEngine/InstrumentSoundConfig.h"

namespace gui
{
bool DrawChannelEditor(
    InstrumentSoundConfig& sound,
    int channel,
    int& selectedDrumNote,
    bool showSourceTypeSelector = true,
    const std::function<void(const char* what, const char* impact, const char* caution)>& updateHoverHelp = {});
} // namespace gui
