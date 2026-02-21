#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <string>

#include "SynthEngine/SynthEngine.h"

struct AppConfig
{
    std::filesystem::path midiPath;
    std::filesystem::path wavPath;
    int targetChannel;
    WaveType defaultWave;
    int initialSeconds;
    int bits;
    int sampleRate;
    double extraReleaseSec;
    std::shared_ptr<const std::array<ChannelConfig, 16>> channelConfigs;
    std::shared_ptr<const std::array<ChannelMixState, 16>> channelMixStates;
};

struct IRunObserver
{
    virtual ~IRunObserver() = default;
    virtual void OnLogLine(const std::string& line) = 0;
};

std::filesystem::path FindProjectRootPath();
AppConfig DefaultConfig();
bool LoadConfigFile(const std::filesystem::path& configPath, AppConfig& cfg, std::string& err);
bool SaveConfigFile(const std::filesystem::path& configPath, const AppConfig& config, std::string& err);
int Run(const AppConfig& config);
int Run(const AppConfig& config, IRunObserver* observer);
