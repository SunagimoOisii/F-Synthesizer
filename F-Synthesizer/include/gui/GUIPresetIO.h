#pragma once

#include <array>
#include <filesystem>
#include <string>
#include <vector>

#include "AppCore.h"

namespace gui
{
struct GUIPresetSnapshot
{
    std::string midiPathUtf8;
    std::string wavPathUtf8;
    std::array<ChannelConfig, 16> channelConfigs{};
    std::array<ChannelMixState, 16> channelMixStates{};
};

bool SavePresetDiffFile(const GUIPresetSnapshot& snapshot, const std::filesystem::path& presetPath, std::string& err);
std::vector<std::string> CollectPresetItems(const std::filesystem::path& projectRoot);
bool LoadPresetConfig(
    const std::filesystem::path& projectRoot,
    const std::string& presetName,
    AppConfig& cfg,
    std::string& err);
} // namespace gui
