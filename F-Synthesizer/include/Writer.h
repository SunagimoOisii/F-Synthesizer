#pragma once

#include "AudioBuffer.h"

bool SaveWavFile(const SoundData& sound, const char* filePath);
bool SaveCsvFile(const SoundData& sound, const char* filePath);
