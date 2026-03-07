#pragma once

#include <filesystem>
#include <string>

#include "AppCore.h"

namespace config::internal::load
{
bool ParseModulationObject(const std::string& text, ModulationConfig& modulation, std::string& err);
bool ParseWaveformSmoothingObject(const std::string& text, WaveformConfig::SmoothingConfig& smoothing);
bool ValidateModulation(
    const ModulationConfig& modulation,
    bool allowFmIndexDestination,
    const char* contextPrefix,
    std::string& err);
bool ValidateWaveformSmoothing(const WaveformConfig::SmoothingConfig& smoothing, std::string& err);

bool ParseSourceObject(const std::string& sourceObjText, SourceConfig& outSource, std::string& err);

bool LoadChannelsDiff(const std::string& text, AppConfig& cfg, std::string& err);
bool LoadChannelMixDiff(const std::string& text, AppConfig& cfg, std::string& err);

bool LoadConfigFromText(
    const std::string& text,
    const std::filesystem::path& baseDir,
    AppConfig& cfg,
    std::string& err);
} // namespace config::internal::load
