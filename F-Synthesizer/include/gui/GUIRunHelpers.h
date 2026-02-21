#pragma once

#include <filesystem>
#include <string>

namespace gui
{
bool ValidateRunSettings(
    const std::string& midiPathUtf8,
    const std::string& wavPathUtf8,
    int targetChannel,
    int sampleRate,
    int initialSeconds,
    int bits,
    std::string& err);

std::filesystem::path BuildSerialWavPath(const std::filesystem::path& basePath);
std::filesystem::path BuildPreviewWavPath(const std::filesystem::path& basePath, int channel);
} // namespace gui
