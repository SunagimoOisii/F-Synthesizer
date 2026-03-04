#pragma once

#include <array>
#include <filesystem>
#include <functional>
#include <iosfwd>
#include <memory>
#include <optional>
#include <string>

#include "AppCore.h"

namespace config::internal
{
std::string ReadTextFile(const std::filesystem::path& filePath);

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
bool ParseTopLevelObjectEntries(
    const std::string& objText,
    const std::function<bool(const std::string&, const std::string&)>& onEntry,
    std::string& err);

std::shared_ptr<std::array<ChannelConfig, 16>> MakeMutableChannelConfigs(const AppConfig& cfg);
std::shared_ptr<std::array<ChannelMixState, 16>> MakeMutableChannelMixStates(const AppConfig& cfg);

bool LoadConfigFileInternal(const std::filesystem::path& configPath, AppConfig& cfg, std::string& err);
bool SaveConfigFileInternal(const std::filesystem::path& configPath, const AppConfig& config, std::string& err);

void WriteIndent(std::ostream& out, int indent);
void WriteDrumConfig(std::ostream& out, const DrumConfig& d, int indent);
void WriteSourceConfig(std::ostream& out, const SourceConfig& src, int indent);
void WriteChannelConfig(std::ostream& out, int ch, const ChannelConfig& cfg, bool withComma);
void WriteChannelMixState(std::ostream& out, int ch, const ChannelMixState& mix, bool withComma);
} // namespace config::internal
