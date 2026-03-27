#pragma once

#include <filesystem>
#include <functional>
#include <string>

#include "AppCore.h"
#include "config/SourceRegistry.h"

namespace config::internal::load
{
// 目的: modulation オブジェクトを構文解析し、既定値へ上書きする。
// 前提: text は { ... } 形式のオブジェクト文字列。
bool ParseModulationObject(const std::string& text, ModulationConfig& modulation, std::string& err);
bool ParseWaveformSmoothingObject(const std::string& text, WaveformConfig::SmoothingConfig& smoothing);
bool ParseWaveformSmoothingObject(const std::string& text, AnalogConfig::SmoothingConfig& smoothing);
// 目的: modulation の数値範囲と destination 許可範囲を検証する。
// 前提: allowFmIndexDestination=false のとき fm.index は拒否する。
bool ValidateModulation(
    const ModulationConfig& modulation,
    bool allowFmIndexDestination,
    const char* contextPrefix,
    std::string& err);
bool ValidateWaveformSmoothing(const WaveformConfig::SmoothingConfig& smoothing, std::string& err);
bool ValidateWaveformSmoothing(const AnalogConfig::SmoothingConfig& smoothing, std::string& err);

bool ParseSourceObject(const std::string& sourceObjText, SourceConfig& outSource, std::string& err);
bool ParseWaveformSource(const std::string& sourceObjText, SourceConfig& outSource, std::string& err);
bool ParseAnalogSource(const std::string& sourceObjText, SourceConfig& outSource, std::string& err);
bool ParseNoiseSource(const std::string& sourceObjText, SourceConfig& outSource, std::string& err);
bool ParseFmSource(const std::string& sourceObjText, SourceConfig& outSource, std::string& err);
bool ParseDrumSource(const std::string& sourceObjText, SourceConfig& outSource, std::string& err);
bool ParseDrumKitSource(const std::string& sourceObjText, SourceConfig& outSource, std::string& err);
bool ParsePsgSource(const std::string& sourceObjText, SourceConfig& outSource, std::string& err);

bool ValidateLifecycleContract(const std::string& sourceObjText, SourceKind sourceKind, std::string& err);
bool ValidateSmoothingSupport(const std::string& sourceObjText, SourceKind sourceKind, std::string& err);

bool ParseWaveformCommonFields(const std::string& text, WaveformConfig& cfg, std::string& err);
bool ParseAnalogCommonFields(const std::string& text, AnalogConfig& cfg, std::string& err);

bool ValidateNoiseBySchema(const NoiseConfig& noise, std::string& err);
bool ValidateWaveformBySchema(const WaveformConfig& wf, std::string& err);
bool ValidateAnalogBySchema(const AnalogConfig& analog, std::string& err);
bool ValidateFmBySchema(const FmConfig& fm, std::string& err);
bool ValidateDrumBySchema(const DrumConfig& drum, std::string& err);

bool ParseDrumConfigObject(const std::string& text, DrumConfig& drum, std::string& err);
bool ExtractArrayForKey(const std::string& text, const std::string& key, std::string& outArray, bool& found, std::string& err);
bool ParseTopLevelArrayObjectEntries(
    const std::string& arrText,
    const std::function<bool(size_t, const std::string&)>& onEntry,
    std::string& err);
bool ParseTopLevelIntArrayElements(
    const std::string& arrText,
    const std::function<bool(size_t, int)>& onElement,
    std::string& err);

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
