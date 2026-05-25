#pragma once

#include <array>
#include <filesystem>
#include <string>
#include <vector>

#include "SynthEngine/InstrumentSoundConfig.h"

namespace gui
{
struct GUIPresetSnapshot
{
    std::array<InstrumentSoundConfig, 16> soundSlots{};
};

bool SavePresetDiffFile(const GUIPresetSnapshot& snapshot, const std::filesystem::path& presetPath, std::string& err);
std::vector<std::string> CollectPresetItems(const std::filesystem::path& projectRoot);
} // namespace gui
