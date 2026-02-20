#pragma once

#include <filesystem>

#include "AudioBuffer.h"

bool SaveWavFilePath(const SoundData& sound, const std::filesystem::path& filePath);
