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
};

struct IRunObserver
{
    virtual ~IRunObserver() = default;
    virtual void OnLogLine(const std::string& line) = 0;
};

std::filesystem::path FindProjectRootPath();
AppConfig DefaultConfig();
int Run(const AppConfig& config);
int Run(const AppConfig& config, IRunObserver* observer);
