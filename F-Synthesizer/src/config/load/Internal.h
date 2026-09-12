#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include "third_party/nlohmann/json.hpp"

#include "AppCore.h"
#include "config/SourceRegistry.h"

namespace config::internal::load
{
using Json = nlohmann::json;
// 目的: modulation オブジェクトを構文解析し、既定値へ上書きする。
// 前提: text は解析済みの JSON object。
bool ParseModulationObject(const Json& text, ModulationConfig& modulation, std::string& err);
bool ParseWaveformSmoothingObject(const Json& text, WaveformConfig::SmoothingConfig& smoothing);
bool ParseWaveformSmoothingObject(const Json& text, AnalogConfig::SmoothingConfig& smoothing);
// 目的: modulation の数値範囲と destination 許可範囲を検証する。
// 前提: allowFmIndexDestination=false のとき fm.index は拒否する。
bool ValidateModulation(
    const ModulationConfig& modulation,
    bool allowFmIndexDestination,
    const char* contextPrefix,
    std::string& err);
bool ValidateWaveformSmoothing(const WaveformConfig::SmoothingConfig& smoothing, std::string& err);
bool ValidateWaveformSmoothing(const AnalogConfig::SmoothingConfig& smoothing, std::string& err);

bool ParseSourceObject(const Json& sourceObjText, SourceConfig& outSource, std::string& err);
bool ParseWaveformSource(const Json& sourceObjText, SourceConfig& outSource, std::string& err);
bool ParseAnalogSource(const Json& sourceObjText, SourceConfig& outSource, std::string& err);
bool ParseNoiseSource(const Json& sourceObjText, SourceConfig& outSource, std::string& err);
bool ParseFmSource(const Json& sourceObjText, SourceConfig& outSource, std::string& err);
bool ParseDrumKitSource(const Json& sourceObjText, SourceConfig& outSource, std::string& err);
bool ParsePsgSource(const Json& sourceObjText, SourceConfig& outSource, std::string& err);

bool ValidateLifecycleContract(const Json& sourceObjText, SourceKind sourceKind, std::string& err);
bool ValidateSmoothingSupport(const Json& sourceObjText, SourceKind sourceKind, std::string& err);

bool ParseWaveformCommonFields(const Json& text, WaveformConfig& cfg, std::string& err);
bool ParseAnalogCommonFields(const Json& text, AnalogConfig& cfg, std::string& err);

bool ValidateNoiseBySchema(const NoiseConfig& noise, std::string& err);
bool ValidateWaveformBySchema(const WaveformConfig& wf, std::string& err);
bool ValidateAnalogBySchema(const AnalogConfig& analog, std::string& err);
bool ValidateFmBySchema(const FmConfig& fm, std::string& err);
bool ValidateDrumBySchema(const DrumConfig& drum, std::string& err);

bool ParseDrumConfigObject(const Json& text, DrumConfig& drum, std::string& err);
bool ExtractArrayForKey(const Json& text, const std::string& key, Json& outArray, bool& found, std::string& err);
bool ParseTopLevelArrayObjectEntries(
    const Json& arrText,
    const std::function<bool(size_t, const Json&)>& onEntry,
    std::string& err);
bool ParseTopLevelIntArrayElements(
    const Json& arrText,
    const std::function<bool(size_t, int)>& onElement,
    std::string& err);
bool ParseTopLevelDoubleArrayElements(
    const Json& arrText,
    const std::function<bool(size_t, double)>& onElement,
    std::string& err);

bool ParseInstrumentSoundObject(const Json& soundObjText, InstrumentSoundConfig& cfg, std::string& err);
bool ParseChannelMixObject(const Json& mixObjText, ChannelMixState& mix, std::string& err);

// 目的: 設定全文から ProjectModel を構築する。
// 前提: baseDir は相対パス解決の基準ディレクトリ。
// 副作用: model を更新し、失敗時は err を設定する。
bool LoadConfigFromJSON(
    const Json& text,
    const std::filesystem::path& baseDir,
    ProjectModel& model,
    std::string& err);
} // namespace config::internal::load
