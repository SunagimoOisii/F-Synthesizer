#include "gui/GUIProjectFacade.h"

#include <algorithm>
#include <map>

#include "gui/GUIPlatform.h"
#include "io/PlatformPaths.h"

namespace gui
{
namespace
{
std::array<InstrumentSoundConfig, 16> DefaultSoundSlots()
{
    std::array<InstrumentSoundConfig, 16> sounds{};
    const ProjectModel defaults = DefaultProjectModel();
    if (defaults.instruments && defaults.projectChannels)
    {
        for (int ch = 0; ch < 16; ch++)
        {
            const auto& channel = (*defaults.projectChannels)[ch];
            const auto it = defaults.instruments->find(channel.instrumentId);
            if (it != defaults.instruments->end())
            {
                sounds[static_cast<size_t>(ch)] = it->second.sound;
            }
        }
    }
    return sounds;
}

std::array<ChannelMixState, 16> DefaultChannelMixStates()
{
    std::array<ChannelMixState, 16> mixes{};
    const ProjectModel defaults = DefaultProjectModel();
    if (defaults.projectChannels)
    {
        for (int ch = 0; ch < 16; ch++)
        {
            mixes[static_cast<size_t>(ch)] = (*defaults.projectChannels)[ch].mix;
        }
    }
    return mixes;
}
} // namespace

std::array<InstrumentSoundConfig, 16>& MutableSoundSlots(GUIState& state)
{
    if (!state.soundSlots)
    {
        state.soundSlots = std::make_shared<std::array<InstrumentSoundConfig, 16>>(DefaultSoundSlots());
    }
    return *state.soundSlots;
}

std::array<ChannelMixState, 16>& MutableChannelMixStates(GUIState& state)
{
    if (!state.channelMixStates)
    {
        state.channelMixStates = std::make_shared<std::array<ChannelMixState, 16>>(DefaultChannelMixStates());
    }
    return *state.channelMixStates;
}

const std::array<InstrumentSoundConfig, 16>& ReadSoundSlots(GUIState& state)
{
    return MutableSoundSlots(state);
}

const std::array<ChannelMixState, 16>& ReadChannelMixStates(GUIState& state)
{
    return MutableChannelMixStates(state);
}

const InstrumentSoundConfig& ReadSoundSlot(GUIState& state, int slot)
{
    return MutableSoundSlots(state)[std::clamp(slot, 0, 15)];
}

const InstrumentSoundConfig& ReadSoundSlot(const GUIState& state, int slot)
{
    slot = std::clamp(slot, 0, 15);
    if (state.soundSlots)
    {
        return (*state.soundSlots)[slot];
    }

    static const std::array<InstrumentSoundConfig, 16> fallback = DefaultSoundSlots();
    return fallback[static_cast<size_t>(slot)];
}

InstrumentSoundConfig& MutableSoundSlot(GUIState& state, int slot)
{
    return MutableSoundSlots(state)[std::clamp(slot, 0, 15)];
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
    auto instruments = std::make_shared<std::map<std::string, InstrumentConfig>>();
    auto projectChannels = std::make_shared<std::array<ProjectChannelAssignment, 16>>();
    const auto soundSlots = state.soundSlots ? *state.soundSlots : DefaultSoundSlots();
    const auto mixStates = state.channelMixStates ? *state.channelMixStates : DefaultChannelMixStates();
    for (int ch = 0; ch < 16; ch++)
    {
        const std::string id = "gui_project__ch" + std::to_string(ch);
        InstrumentConfig instrument{};
        instrument.sound = soundSlots[static_cast<size_t>(ch)];
        instruments->emplace(id, instrument);

        ProjectChannelAssignment channel{};
        channel.enabled = true;
        channel.instrumentId = id;
        channel.mix = mixStates[static_cast<size_t>(ch)];
        (*projectChannels)[static_cast<size_t>(ch)] = channel;
    }
    model.instruments = instruments;
    model.projectChannels = projectChannels;
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

    state.soundSlots = std::make_shared<std::array<InstrumentSoundConfig, 16>>(DefaultSoundSlots());
    state.channelMixStates = std::make_shared<std::array<ChannelMixState, 16>>(DefaultChannelMixStates());
    if (model.instruments && model.projectChannels)
    {
        for (int ch = 0; ch < 16; ch++)
        {
            const auto& channel = (*model.projectChannels)[ch];
            const auto it = model.instruments->find(channel.instrumentId);
            if (it != model.instruments->end())
            {
                (*state.soundSlots)[static_cast<size_t>(ch)] = it->second.sound;
            }
            (*state.channelMixStates)[static_cast<size_t>(ch)] = channel.mix;
        }
    }
}

ProjectModel BuildRuntimeProjectFromGUI(GUIState& state, const char* instrumentPrefix, bool applyChannelAssignments)
{
    ProjectModel project = BuildProjectModelFromGUI(state);
    auto instruments = std::make_shared<std::map<std::string, InstrumentConfig>>();
    auto projectChannels = std::make_shared<std::array<ProjectChannelAssignment, 16>>();

    const char* prefix = (instrumentPrefix != nullptr && instrumentPrefix[0] != '\0')
        ? instrumentPrefix
        : "gui_runtime__ch";
    const auto& soundSlots = ReadSoundSlots(state);
    const auto& channelMixStates = ReadChannelMixStates(state);

    for (int ch = 0; ch < 16; ch++)
    {
        const int src = applyChannelAssignments ? AssignedSoundSlot(state, ch) : ch;
        const std::string id = std::string(prefix) + std::to_string(ch);
        InstrumentConfig instrument{};
        instrument.sound = soundSlots[src];
        instruments->emplace(id, instrument);

        ProjectChannelAssignment projectChannel{};
        projectChannel.enabled = true;
        projectChannel.instrumentId = id;
        projectChannel.mix = channelMixStates[ch];
        (*projectChannels)[ch] = projectChannel;
    }

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
