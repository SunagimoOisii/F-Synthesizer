#include "gui/GUIProjectFacade.h"

#include "gui/GUIPlatform.h"
#include "io/PlatformPaths.h"

namespace gui
{
std::array<ChannelConfig, 16>& MutableChannelConfigs(GUIState& state)
{
    if (!state.channelConfigs)
    {
        const ProjectModel defaults = DefaultProjectModel();
        state.channelConfigs = std::make_shared<std::array<ChannelConfig, 16>>();
        if (defaults.channelConfigs)
        {
            *state.channelConfigs = *defaults.channelConfigs;
        }
    }
    return *state.channelConfigs;
}

std::array<ChannelMixState, 16>& MutableChannelMixStates(GUIState& state)
{
    if (!state.channelMixStates)
    {
        const ProjectModel defaults = DefaultProjectModel();
        state.channelMixStates = std::make_shared<std::array<ChannelMixState, 16>>();
        if (defaults.channelMixStates)
        {
            *state.channelMixStates = *defaults.channelMixStates;
        }
    }
    return *state.channelMixStates;
}

const std::array<ChannelConfig, 16>& ReadChannelConfigs(GUIState& state)
{
    return MutableChannelConfigs(state);
}

const std::array<ChannelMixState, 16>& ReadChannelMixStates(GUIState& state)
{
    return MutableChannelMixStates(state);
}

ProjectModel BuildProjectModelFromGUI(const GUIState& state)
{
    ProjectModel model = DefaultProjectModel();
    model.midiPath = Utf8ToPath(state.midiPath);
    model.wavPath = Utf8ToPath(state.wavPath);
    model.targetChannel = state.targetChannel;
    model.sampleRate = state.sampleRate;
    model.initialSeconds = state.initialSeconds;
    model.bits = state.bits;
    model.extraReleaseSec = state.extraReleaseSec;
    model.masterEffects = state.masterEffects;
    if (state.channelConfigs)
    {
        model.channelConfigs = std::make_shared<const std::array<ChannelConfig, 16>>(*state.channelConfigs);
    }
    if (state.channelMixStates)
    {
        model.channelMixStates = std::make_shared<const std::array<ChannelMixState, 16>>(*state.channelMixStates);
    }
    return model;
}

void ApplyProjectModelToGUI(GUIState& state, const ProjectModel& model)
{
    CopyPath(state.midiPath, sizeof(state.midiPath), model.midiPath);
    CopyPath(state.wavPath, sizeof(state.wavPath), model.wavPath);
    state.targetChannel = model.targetChannel;
    state.sampleRate = model.sampleRate;
    state.initialSeconds = model.initialSeconds;
    state.bits = model.bits;
    state.extraReleaseSec = static_cast<float>(model.extraReleaseSec);
    state.masterEffects = model.masterEffects;

    state.channelConfigs = std::make_shared<std::array<ChannelConfig, 16>>();
    if (model.channelConfigs)
    {
        *state.channelConfigs = *model.channelConfigs;
    }

    state.channelMixStates = std::make_shared<std::array<ChannelMixState, 16>>();
    if (model.channelMixStates)
    {
        *state.channelMixStates = *model.channelMixStates;
    }
}
} // namespace gui
