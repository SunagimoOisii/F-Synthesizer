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

// basePath の親ディレクトリに "name_YYYYMMDD_HHMMSS[_n].ext" を生成する。
std::filesystem::path BuildSerialWAVPath(const std::filesystem::path& basePath);
// Preview保存時の一時命名規則。channel は呼び出し側で 0..15 に丸めて渡す前提。
std::filesystem::path BuildPreviewWAVPath(const std::filesystem::path& basePath, int channel);
} // namespace gui
