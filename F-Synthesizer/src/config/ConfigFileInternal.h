#pragma once

#include <array>
#include <filesystem>
#include <functional>
#include <iosfwd>
#include <memory>
#include <optional>
#include <string>

#include "AppCore.h"
#include "project/ProjectModel.h"

namespace config::internal
{
// 目的: UTF-8テキストを読み込む。失敗時は空文字を返す。
std::string ReadTextFile(const std::filesystem::path& filePath);

// 目的: JSON object から top-level の単純キーを取り出す。
// 制約: key を含む JSON object 文字列が入力であること。
std::optional<std::string> ReadJSONString(const std::string& text, const std::string& key);
std::optional<int> ReadJSONInt(const std::string& text, const std::string& key);
std::optional<double> ReadJSONDouble(const std::string& text, const std::string& key);
std::optional<bool> ReadJSONBool(const std::string& text, const std::string& key);

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
std::string FilterModeToString(FilterMode mode);
std::string LfoWaveToString(LfoWave wave);
std::string ModSourceToString(ModSource source);
std::string ModDestinationToString(ModDestination destination);
std::string EscapeJSON(const std::string& src);

bool ExtractObjectAt(const std::string& text, size_t openBracePos, std::string& outObject, std::string& err);
bool ExtractObjectForKey(const std::string& text, const std::string& key, std::string& outObject, bool& found, std::string& err);
// 目的: {"k": {...}} の object を1段だけ走査して、各要素を onEntry へ渡す。
bool ParseTopLevelObjectEntries(
    const std::string& objText,
    const std::function<bool(const std::string&, const std::string&)>& onEntry,
    std::string& err);

bool LoadProjectModelFileInternal(const std::filesystem::path& configPath, ProjectModel& model, std::string& err);
bool SaveProjectModelFileInternal(const std::filesystem::path& configPath, const ProjectModel& model, std::string& err);

void WriteIndent(std::ostream& out, int indent);
void WriteDrumConfig(std::ostream& out, const DrumConfig& d, int indent);
void WriteSourceConfig(std::ostream& out, const SourceConfig& src, int indent);
void WriteInstrumentSoundConfig(std::ostream& out, int ch, const InstrumentSoundConfig& cfg, bool withComma);
void WriteChannelMixState(std::ostream& out, int ch, const ChannelMixState& mix, bool withComma);
} // namespace config::internal
