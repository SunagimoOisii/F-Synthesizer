#pragma once

#include <array>
#include <filesystem>
#include <functional>
#include <iosfwd>
#include <memory>
#include <optional>
#include <string>
#include "third_party/nlohmann/json.hpp"

#include "AppCore.h"
#include "project/ProjectModel.h"

namespace config::internal
{
using Json = nlohmann::json;
// 目的: UTF-8テキストを読み込む。失敗時は空文字を返す。
std::string ReadTextFile(const std::filesystem::path& filePath);

// 目的: JSON object から top-level の単純キーを取り出す。
// 制約: 解析済みの JSON object を渡すこと。
std::optional<std::string> ReadJSONString(const Json& text, const std::string& key);
std::optional<int> ReadJSONInt(const Json& text, const std::string& key);
std::optional<double> ReadJSONDouble(const Json& text, const std::string& key);
std::optional<bool> ReadJSONBool(const Json& text, const std::string& key);

bool TryParseWaveType(const std::string& name, WaveType& outWave);
bool TryParseNoiseType(const std::string& name, NoiseType& outNoise);
bool TryParseDrumType(const std::string& name, DrumType& outType);
bool TryParseFilterMode(const std::string& name, FilterMode& outMode);
bool TryParseLfoWave(const std::string& name, LfoWave& outWave);
bool TryParseModSource(const std::string& name, ModSource& outSource);
bool TryParseModDestination(const std::string& name, ModDestination& outDestination);

std::string WaveTypeToString(WaveType w);
std::string NoiseTypeToString(NoiseType n);
std::string DrumTypeToString(DrumType d);
std::string PsgWaveTypeToString(PsgWaveType w);
std::string AttackLayerTypeToString(AttackLayerType type);
std::string BassLayerTypeToString(BassLayerType type);
std::string LeadLayerTypeToString(LeadLayerType type);
std::string BodyLayerModeToString(BodyLayerConfig::Mode mode);
std::string FilterModeToString(FilterMode mode);
std::string LfoWaveToString(LfoWave wave);
std::string ModSourceToString(ModSource source);
std::string ModDestinationToString(ModDestination destination);

bool ExtractObjectForKey(const Json& text, const std::string& key, Json& outObject, bool& found, std::string& err);
// 目的: {"k": {...}} の object を1段だけ走査して、各要素を onEntry へ渡す。
bool ParseTopLevelObjectEntries(
    const Json& objText,
    const std::function<bool(const std::string&, const Json&)>& onEntry,
    std::string& err);

bool LoadProjectModelFileInternal(const std::filesystem::path& configPath, ProjectModel& model, std::string& err);
bool SaveProjectModelFileInternal(const std::filesystem::path& configPath, const ProjectModel& model, std::string& err);

} // namespace config::internal
