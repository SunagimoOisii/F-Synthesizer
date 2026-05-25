#include "core/RenderConfigBuilder.h"

namespace
{
std::shared_ptr<const std::array<InstrumentSoundConfig, 16>> ResolveDefaultSoundSlots()
{
    const ProjectModel defaults = DefaultProjectModel();
    auto soundSlots = std::make_shared<std::array<InstrumentSoundConfig, 16>>();
    if (defaults.instruments && defaults.projectChannels)
    {
        for (int ch = 0; ch < 16; ch++)
        {
            const ProjectChannelAssignment& channel = (*defaults.projectChannels)[ch];
            const auto it = defaults.instruments->find(channel.instrumentId);
            if (channel.enabled && it != defaults.instruments->end())
            {
                (*soundSlots)[ch] = it->second.sound;
            }
        }
    }
    return std::static_pointer_cast<const std::array<InstrumentSoundConfig, 16>>(soundSlots);
}

std::shared_ptr<const std::array<ChannelMixState, 16>> ResolveDefaultChannelMixStates()
{
    const ProjectModel defaults = DefaultProjectModel();
    auto channelMixStates = std::make_shared<std::array<ChannelMixState, 16>>();
    if (defaults.projectChannels)
    {
        for (int ch = 0; ch < 16; ch++)
        {
            if ((*defaults.projectChannels)[ch].enabled)
            {
                (*channelMixStates)[ch] = (*defaults.projectChannels)[ch].mix;
            }
        }
    }
    return std::static_pointer_cast<const std::array<ChannelMixState, 16>>(channelMixStates);
}
} // namespace

ResolvedRenderConfigInputs ResolveRenderConfigInputs(const ProjectModel& project)
{
    auto soundSlots = std::make_shared<std::array<InstrumentSoundConfig, 16>>(*ResolveDefaultSoundSlots());
    auto channelMixStates = std::make_shared<std::array<ChannelMixState, 16>>(*ResolveDefaultChannelMixStates());

    if (project.instruments && project.projectChannels)
    {
        for (int ch = 0; ch < 16; ch++)
        {
            const ProjectChannelAssignment& projectChannel = (*project.projectChannels)[ch];
            if (!projectChannel.enabled)
            {
                continue;
            }
            const auto instrumentIt = project.instruments->find(projectChannel.instrumentId);
            if (instrumentIt != project.instruments->end())
            {
                (*soundSlots)[ch] = instrumentIt->second.sound;
            }
            (*channelMixStates)[ch] = projectChannel.mix;
        }
    }

    return ResolvedRenderConfigInputs{
        std::static_pointer_cast<const std::array<InstrumentSoundConfig, 16>>(soundSlots),
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
        *inputs.soundSlots,
        *inputs.channelMixStates,
        project.masterEffects
    };
}
