#pragma once

#include <array>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "AppCore.h"

// 保存、preset、config の正本モデル。
// 実行時だけの差し替え入力や preview override は RenderRuntimeOverrides 側に分離し、
// ここには projectModel.v3 として永続化する値だけを置く。
struct RecommendedRange
{
    int low = 48;
    int high = 84;
    int preview = 60;
};

struct MacroHint
{
    std::string id;
    std::string label;
    std::string description;
};

// InstrumentConfig::sound は Sound Card の音色本体として扱う。
struct InstrumentConfig
{
    std::string displayName;
    std::string category;
    bool internal = false;
    std::vector<std::string> tags;
    std::string description;
    RecommendedRange recommendedRange;
    std::vector<MacroHint> macroHints;
    InstrumentSoundConfig sound;
};

struct ProjectChannelAssignment
{
    bool enabled = false;
    std::string instrumentId;
    ChannelMixState mix;
};

struct ProjectModel
{
    std::filesystem::path midiPath;
    std::filesystem::path wavPath;
    int targetChannel = -1;
    int initialSeconds = 0;
    int bits = 16;
    int sampleRate = 0;
    double extraReleaseSec = 0.0;
    MasterEffectConfig masterEffects;
    std::shared_ptr<const std::map<std::string, InstrumentConfig>> instruments;
    std::shared_ptr<const std::array<ProjectChannelAssignment, 16>> projectChannels;
};

ProjectModel DefaultProjectModel();
bool LoadProjectModelFile(const std::filesystem::path& configPath, ProjectModel& model, std::string& err);
bool SaveProjectModelFile(const std::filesystem::path& configPath, const ProjectModel& model, std::string& err);
