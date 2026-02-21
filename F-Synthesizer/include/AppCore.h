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

// Run実行時の挙動を切り替えるオプション集合。
// app層から実行コアへ渡し、Export/Previewの境界条件を統一する。
// durationSec < 0 は「末尾まで」を表す。
struct RenderOptions
{
    RunMode mode = RunMode::Export;
    double startSec = 0.0;
    double durationSec = -1.0; // < 0 means full length
    bool writeWav = true;
    bool allowCancel = true;
};

// アプリ実行に必要な解決済み設定。
// CLI/GUI/ConfigResolver で構築され、Run境界へ受け渡される。
// channelConfigs/channelMixStates は共有所有で不変参照する前提。
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

// Run実行の進捗通知と停止要求を受け持つ観測I/F。
// GUIとCLIの双方から同じログ経路を扱えるようにするための境界型。
struct IRunObserver
{
    virtual ~IRunObserver() = default;
    virtual void OnLogLine(const std::string& line) = 0;
    virtual bool ShouldCancel() { return false; }
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
