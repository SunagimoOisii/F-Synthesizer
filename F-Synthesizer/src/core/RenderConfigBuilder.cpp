#include "core/RenderConfigBuilder.h"

namespace
{
std::shared_ptr<const std::array<ChannelConfig, 16>> ResolveDefaultChannelConfigs()
{
    const ProjectModel defaults = DefaultProjectModel();
    if (defaults.channelConfigs)
    {
        return defaults.channelConfigs;
    }
    return std::make_shared<const std::array<ChannelConfig, 16>>();
}

std::shared_ptr<const std::array<ChannelMixState, 16>> ResolveDefaultChannelMixStates()
{
    const ProjectModel defaults = DefaultProjectModel();
    if (defaults.channelMixStates)
    {
        return defaults.channelMixStates;
    }
    return std::make_shared<const std::array<ChannelMixState, 16>>();
}
} // namespace

ResolvedRenderConfigInputs ResolveRenderConfigInputs(const ProjectModel& project)
{
    auto channelConfigs = std::make_shared<std::array<ChannelConfig, 16>>(*ResolveDefaultChannelConfigs());
    auto channelMixStates = std::make_shared<std::array<ChannelMixState, 16>>(*ResolveDefaultChannelMixStates());

    if (project.instruments && project.projectChannels)
    {
        for (int ch = 0; ch < 16; ch++)
        {
            const ProjectChannelConfig& projectChannel = (*project.projectChannels)[ch];
            if (!projectChannel.enabled)
            {
                continue;
            }
            const auto instrumentIt = project.instruments->find(projectChannel.instrumentId);
            if (instrumentIt != project.instruments->end())
            {
                (*channelConfigs)[ch] = instrumentIt->second.sound;
            }
            (*channelMixStates)[ch] = projectChannel.mix;
        }
    }
    else
    {
        if (project.channelConfigs)
        {
            *channelConfigs = *project.channelConfigs;
        }
        if (project.channelMixStates)
        {
            *channelMixStates = *project.channelMixStates;
        }
    }

    return ResolvedRenderConfigInputs{
        std::static_pointer_cast<const std::array<ChannelConfig, 16>>(channelConfigs),
        std::static_pointer_cast<const std::array<ChannelMixState, 16>>(channelMixStates)
    };
}

RenderConfig BuildRenderConfig(
    const ProjectModel& project,
    const RenderOptions& options,
    const std::vector<MIDIEvent>& events,
    const MIDIBuildOutput& midiOut,
    const ResolvedRenderConfigInputs& inputs)
{
    return RenderConfig{
        events,
        midiOut.tempoEvents,
        midiOut.ticksPerQuarter,
        options.startSec,
        *inputs.channelConfigs,
        *inputs.channelMixStates,
        project.masterEffects
    };
}
