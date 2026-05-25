#include "gui/GUIProjectFacade.h"

#include <algorithm>
#include <map>

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

const ChannelConfig& ReadSoundSlot(GUIState& state, int slot)
{
    return MutableChannelConfigs(state)[std::clamp(slot, 0, 15)];
}

const ChannelConfig& ReadSoundSlot(const GUIState& state, int slot)
{
    slot = std::clamp(slot, 0, 15);
    if (state.channelConfigs)
    {
        return (*state.channelConfigs)[slot];
    }

    static const ProjectModel defaults = DefaultProjectModel();
    if (defaults.channelConfigs)
    {
        return (*defaults.channelConfigs)[slot];
    }

    static const std::array<ChannelConfig, 16> fallback{};
    return fallback[slot];
}

ChannelConfig& MutableSoundSlot(GUIState& state, int slot)
{
    return MutableChannelConfigs(state)[std::clamp(slot, 0, 15)];
}

const ChannelMixState& ReadChannelMix(GUIState& state, int channel)
{
    return MutableChannelMixStates(state)[std::clamp(channel, 0, 15)];
}

ChannelMixState& MutableChannelMix(GUIState& state, int channel)
{
    return MutableChannelMixStates(state)[std::clamp(channel, 0, 15)];
}

int AssignedSoundSlot(const GUIState& state, int channel)
{
    channel = std::clamp(channel, 0, 15);
    return std::clamp(state.channelAssignments[channel], 0, 15);
}

void SetChannelAssignment(GUIState& state, int channel, int slot)
{
    channel = std::clamp(channel, 0, 15);
    state.channelAssignments[channel] = std::clamp(slot, 0, 15);
}

MacroSliderState& MutableMacroSliders(GUIState& state, int slot)
{
    return state.macroSliders[std::clamp(slot, 0, 15)];
}

const MacroSliderState& ReadMacroSliders(const GUIState& state, int slot)
{
    return state.macroSliders[std::clamp(slot, 0, 15)];
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

ProjectModel BuildRuntimeProjectFromGUI(GUIState& state, const char* instrumentPrefix, bool applyChannelAssignments)
{
    ProjectModel project = BuildProjectModelFromGUI(state);
    auto instruments = std::make_shared<std::map<std::string, InstrumentConfig>>();
    auto projectChannels = std::make_shared<std::array<ProjectChannelConfig, 16>>();

    const char* prefix = (instrumentPrefix != nullptr && instrumentPrefix[0] != '\0')
        ? instrumentPrefix
        : "gui_runtime__ch";
    const auto& channelConfigs = ReadChannelConfigs(state);
    const auto& channelMixStates = ReadChannelMixStates(state);

    for (int ch = 0; ch < 16; ch++)
    {
        const int src = applyChannelAssignments ? AssignedSoundSlot(state, ch) : ch;
        const std::string id = std::string(prefix) + std::to_string(ch);
        InstrumentConfig instrument{};
        instrument.sound = channelConfigs[src];
        instruments->emplace(id, instrument);

        ProjectChannelConfig projectChannel{};
        projectChannel.enabled = true;
        projectChannel.instrumentId = id;
        projectChannel.mix = channelMixStates[ch];
        (*projectChannels)[ch] = projectChannel;
    }

    project.channelConfigs = std::make_shared<const std::array<ChannelConfig, 16>>(channelConfigs);
    project.channelMixStates = std::make_shared<const std::array<ChannelMixState, 16>>(channelMixStates);
    project.instruments = instruments;
    project.projectChannels = projectChannels;
    return project;
}

void OverrideProjectChannelWithSoundSlot(GUIState& state, int previewChannel, int soundSlot, ProjectModel& project)
{
    if (!project.instruments || !project.projectChannels)
    {
        return;
    }

    previewChannel = std::clamp(previewChannel, 0, 15);
    const std::string instrumentId = (*project.projectChannels)[previewChannel].instrumentId;
    auto mutableInstruments = std::make_shared<std::map<std::string, InstrumentConfig>>(*project.instruments);
    auto it = mutableInstruments->find(instrumentId);
    if (it != mutableInstruments->end())
    {
        it->second.sound = ReadSoundSlot(state, soundSlot);
    }
    project.instruments = mutableInstruments;
}
} // namespace gui
