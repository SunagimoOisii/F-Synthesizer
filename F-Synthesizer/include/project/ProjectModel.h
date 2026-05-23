#pragma once

#include <array>
#include <filesystem>
#include <memory>

#include "AppCore.h"

// 保存、preset、config の正本モデル。
// 実行時だけの差し替え入力は AppConfig 側に残し、ここには永続化対象だけを置く。
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
    std::shared_ptr<const std::array<ChannelConfig, 16>> channelConfigs;
    std::shared_ptr<const std::array<ChannelMixState, 16>> channelMixStates;
};

ProjectModel DefaultProjectModel();
AppConfig ToAppConfig(const ProjectModel& model);
ProjectModel ProjectModelFromAppConfig(const AppConfig& config);
