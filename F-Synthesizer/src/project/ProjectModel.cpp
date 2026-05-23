#include "project/ProjectModel.h"

namespace
{
std::shared_ptr<const std::array<ChannelConfig, 16>> ResolveChannelConfigs(
    const std::shared_ptr<const std::array<ChannelConfig, 16>>& channelConfigs,
    const AppConfig& defaults)
{
    return channelConfigs ? channelConfigs : defaults.channelConfigs;
}

std::shared_ptr<const std::array<ChannelMixState, 16>> ResolveChannelMixStates(
    const std::shared_ptr<const std::array<ChannelMixState, 16>>& channelMixStates,
    const AppConfig& defaults)
{
    return channelMixStates ? channelMixStates : defaults.channelMixStates;
}
} // namespace

ProjectModel DefaultProjectModel()
{
    return ProjectModelFromAppConfig(DefaultConfig());
}

AppConfig ToAppConfig(const ProjectModel& model)
{
    const AppConfig defaults = DefaultConfig();

    AppConfig config{};
    config.midiPath = model.midiPath;
    config.wavPath = model.wavPath;
    config.targetChannel = model.targetChannel;
    config.initialSeconds = model.initialSeconds;
    config.bits = model.bits;
    config.sampleRate = model.sampleRate;
    config.extraReleaseSec = model.extraReleaseSec;
    config.masterEffects = model.masterEffects;
    config.channelConfigs = ResolveChannelConfigs(model.channelConfigs, defaults);
    config.channelMixStates = ResolveChannelMixStates(model.channelMixStates, defaults);
    config.overrideNoteTicks.reset();
    config.overrideTicksPerQuarter = 0;
    return config;
}

ProjectModel ProjectModelFromAppConfig(const AppConfig& config)
{
    const AppConfig defaults = DefaultConfig();

    ProjectModel model{};
    model.midiPath = config.midiPath;
    model.wavPath = config.wavPath;
    model.targetChannel = config.targetChannel;
    model.initialSeconds = config.initialSeconds;
    model.bits = config.bits;
    model.sampleRate = config.sampleRate;
    model.extraReleaseSec = config.extraReleaseSec;
    model.masterEffects = config.masterEffects;
    model.channelConfigs = ResolveChannelConfigs(config.channelConfigs, defaults);
    model.channelMixStates = ResolveChannelMixStates(config.channelMixStates, defaults);
    return model;
}
