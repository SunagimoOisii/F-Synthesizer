#pragma once

#include <array>
#include <filesystem>
#include <string>
#include <vector>

#include "SynthEngine/ChannelConfig.h"

struct AppConfig;

namespace gui
{
struct GUIPresetSnapshot
{
    std::array<ChannelConfig, 16> channelConfigs{};
};

bool SavePresetDiffFile(const GUIPresetSnapshot& snapshot, const std::filesystem::path& presetPath, std::string& err);
std::vector<std::string> CollectPresetItems(const std::filesystem::path& projectRoot);
bool LoadPresetConfig(
    const std::filesystem::path& projectRoot,
    const std::string& presetName,
    AppConfig& cfg,
    std::string& err);
} // namespace gui
