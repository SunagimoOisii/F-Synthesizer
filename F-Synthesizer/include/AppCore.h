#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <string>

#include "AudioBuffer.h"
#include "SynthEngine/SynthEngine.h"

enum class RunMode
{
    Export,
    Preview
};

struct RenderOptions
{
    RunMode mode = RunMode::Export;
    double startSec = 0.0;
    double durationSec = -1.0; // < 0 means full length
    bool writeWav = true;
    bool allowCancel = true;
};

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
RenderOptions DefaultRenderOptions();
RenderOptions DefaultPreviewRenderOptions();
int Run(const AppConfig& config);
int Run(const AppConfig& config, IRunObserver* observer);
int Run(const AppConfig& config, const RenderOptions& options);
int Run(const AppConfig& config, const RenderOptions& options, IRunObserver* observer);
int Run(const AppConfig& config, const RenderOptions& options, IRunObserver* observer, SoundData* renderedSound);
