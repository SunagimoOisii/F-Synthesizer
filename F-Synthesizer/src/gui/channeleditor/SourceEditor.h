#pragma once
#include "SynthEngine/InstrumentSoundConfig.h"
#include <functional>
namespace gui::detail
{
using HoverHelpFn = std::function<void(const char *, const char *, const char *)>;
bool DrawSourceEditor(InstrumentSoundConfig &sound, int &selectedDrumNote, const HoverHelpFn &hoverHelp);
} // namespace gui::detail
