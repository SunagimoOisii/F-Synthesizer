#pragma once

#include <filesystem>
#include <string>

#include "AppCore.h"

namespace config::internal::load
{
// 目的: modulation オブジェクトを構文解析し、既定値へ上書きする。
// 前提: text は { ... } 形式のオブジェクト文字列。
bool ParseModulationObject(const std::string& text, ModulationConfig& modulation, std::string& err);
bool ParseWaveformSmoothingObject(const std::string& text, WaveformConfig::SmoothingConfig& smoothing);
// 目的: modulation の数値範囲と destination 許可範囲を検証する。
// 前提: allowFmIndexDestination=false のとき fm.index は拒否する。
bool ValidateModulation(
    const ModulationConfig& modulation,
    bool allowFmIndexDestination,
    const char* contextPrefix,
    std::string& err);
bool ValidateWaveformSmoothing(const WaveformConfig::SmoothingConfig& smoothing, std::string& err);

bool ParseSourceObject(const std::string& sourceObjText, SourceConfig& outSource, std::string& err);

bool LoadChannelsDiff(const std::string& text, AppConfig& cfg, std::string& err);
bool LoadChannelMixDiff(const std::string& text, AppConfig& cfg, std::string& err);

// 目的: 設定全文から AppConfig を構築する。
// 前提: baseDir は相対パス解決の基準ディレクトリ。
// 副作用: cfg を更新し、失敗時は err を設定する。
bool LoadConfigFromText(
    const std::string& text,
    const std::filesystem::path& baseDir,
    AppConfig& cfg,
    std::string& err);
} // namespace config::internal::load
