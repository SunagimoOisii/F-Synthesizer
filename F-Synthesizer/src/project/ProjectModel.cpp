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

std::string GeneratedInstrumentId(int ch)
{
    return "generated__ch" + std::to_string(ch);
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

    if (model.instruments && model.projectChannels)
    {
        auto channelConfigs = std::make_shared<std::array<ChannelConfig, 16>>(
            defaults.channelConfigs ? *defaults.channelConfigs : std::array<ChannelConfig, 16>{});
        auto channelMixStates = std::make_shared<std::array<ChannelMixState, 16>>(
            defaults.channelMixStates ? *defaults.channelMixStates : std::array<ChannelMixState, 16>{});

        for (int ch = 0; ch < 16; ch++)
        {
            const ProjectChannelConfig& projectChannel = (*model.projectChannels)[ch];
            if (!projectChannel.enabled)
            {
                continue;
            }
            const auto instrumentIt = model.instruments->find(projectChannel.instrumentId);
            if (instrumentIt != model.instruments->end())
            {
                (*channelConfigs)[ch] = instrumentIt->second.sound;
            }
            (*channelMixStates)[ch] = projectChannel.mix;
        }
        config.channelConfigs = std::static_pointer_cast<const std::array<ChannelConfig, 16>>(channelConfigs);
        config.channelMixStates = std::static_pointer_cast<const std::array<ChannelMixState, 16>>(channelMixStates);
    }
    else
    {
        config.channelConfigs = ResolveChannelConfigs(model.channelConfigs, defaults);
        config.channelMixStates = ResolveChannelMixStates(model.channelMixStates, defaults);
    }
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

    auto instruments = std::make_shared<std::map<std::string, InstrumentConfig>>();
    auto projectChannels = std::make_shared<std::array<ProjectChannelConfig, 16>>();
    for (int ch = 0; ch < 16; ch++)
    {
        const std::string instrumentId = GeneratedInstrumentId(ch);
        InstrumentConfig instrument{};
        if (model.channelConfigs)
        {
            instrument.sound = (*model.channelConfigs)[ch];
        }
        instruments->emplace(instrumentId, instrument);

        ProjectChannelConfig projectChannel{};
        projectChannel.enabled = true;
        projectChannel.instrumentId = instrumentId;
        if (model.channelMixStates)
        {
            projectChannel.mix = (*model.channelMixStates)[ch];
        }
        (*projectChannels)[ch] = projectChannel;
    }
    model.instruments = instruments;
    model.projectChannels = projectChannels;
    return model;
}
