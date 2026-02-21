#pragma once

#include <filesystem>
#include <string>

#include "AudioBuffer.h"

struct WavWriteError
{
    std::string code;
    std::string cause;
    std::string hint;
    std::filesystem::path path;
    int errnoValue = 0;
    unsigned long systemError = 0;
};

bool SaveWavFilePath(const SoundData& sound, const std::filesystem::path& filePath);
bool SaveWavFilePath(const SoundData& sound, const std::filesystem::path& filePath, WavWriteError* outError);
