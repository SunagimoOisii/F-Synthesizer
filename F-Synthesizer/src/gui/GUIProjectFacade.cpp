#include "gui/GUIProjectFacade.h"

#include <algorithm>
#include <map>

#include "gui/GUIPlatform.h"
#include "io/PlatformPaths.h"

namespace gui
{
namespace
{
std::string SlotId(int slot)
{
    return "slot_" + std::to_string(slot);
}
} // namespace

std::array<ChannelMixState, 16>& MutableChannelMixStates(GUIState& state)
{
    return state.channelMixStates;
}

const std::array<ChannelMixState, 16>& ReadChannelMixStates(GUIState& state)
{
    return state.channelMixStates;
}

const InstrumentSoundConfig& ReadSoundSlot(GUIState& state, int slot)
{
    return state.instruments[std::clamp(slot, 0, 15)].sound;
}

const InstrumentSoundConfig& ReadSoundSlot(const GUIState& state, int slot)
{
    return state.instruments[std::clamp(slot, 0, 15)].sound;
}

InstrumentSoundConfig& MutableSoundSlot(GUIState& state, int slot)
{
    return state.instruments[std::clamp(slot, 0, 15)].sound;
}

const ChannelMixState& ReadChannelMix(GUIState& state, int channel)
{
    return state.channelMixStates[std::clamp(channel, 0, 15)];
}

ChannelMixState& MutableChannelMix(GUIState& state, int channel)
{
    return state.channelMixStates[std::clamp(channel, 0, 15)];
}

int AssignedSoundSlot(const GUIState& state, int channel)
{
    return std::clamp(state.channelAssignments[std::clamp(channel, 0, 15)], 0, 15);
}

void SetChannelAssignment(GUIState& state, int channel, int slot)
{
    state.channelAssignments[std::clamp(channel, 0, 15)] = std::clamp(slot, 0, 15);
}

MacroSliderState& MutableMacroSliders(GUIState& state, int slot)
{
    return state.macroSliders[std::clamp(slot, 0, 15)];
}

const MacroSliderState& ReadMacroSliders(const GUIState& state, int slot)
{
    return state.macroSliders[std::clamp(slot, 0, 15)];
}

void PublishLiveRenderSettings(GUIState& state)
{
    const auto previous = state.liveSettings->load(std::memory_order_acquire);
    bool changed = !previous || previous->mixes != state.channelMixStates || previous->effects != state.masterEffects;
    for (int ch = 0; ch < 16 && !changed; ++ch)
    {
        const int slot = ch == state.livePreviewChannel && state.livePreviewSlot >= 0
            ? state.livePreviewSlot : AssignedSoundSlot(state, ch);
        changed = previous->sounds[ch] != state.instruments[slot].sound;
    }
    if (!changed) return;
    auto next = std::make_shared<LiveRenderSettings>();
    for (int ch = 0; ch < 16; ++ch)
    {
        const int slot = ch == state.livePreviewChannel && state.livePreviewSlot >= 0
            ? state.livePreviewSlot : AssignedSoundSlot(state, ch);
        next->sounds[ch] = state.instruments[slot].sound;
    }
    next->mixes = state.channelMixStates;
    next->effects = state.masterEffects;
    state.liveSettings->store(std::move(next), std::memory_order_release);
}

ProjectModel BuildProjectModelFromGUI(const GUIState& state)
{
    ProjectModel model{};
    model.midiPath = Utf8ToPath(state.midiPath);
    model.wavPath = Utf8ToPath(state.wavPath);
    model.targetChannel = state.targetChannel;
    model.sampleRate = state.sampleRate;
    model.initialSeconds = state.initialSeconds;
    model.bits = state.bits;
    model.extraReleaseSec = state.extraReleaseSec;
    model.masterEffects = state.masterEffects;
    auto instruments = std::make_shared<std::map<std::string, InstrumentConfig>>();
    auto channels = std::make_shared<std::array<ProjectChannelAssignment, 16>>();
    for (int ch = 0; ch < 16; ch++)
    {
        instruments->emplace(SlotId(ch), state.instruments[ch]);
        auto& channel = (*channels)[ch];
        channel.enabled = true;
        channel.instrumentId = SlotId(AssignedSoundSlot(state, ch));
        channel.mix = state.channelMixStates[ch];
    }
    model.instruments = instruments;
    model.projectChannels = channels;
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
    state.channelMixStates = {};
    std::map<std::string, int> slots;
    if (model.instruments)
    {
        // Workspace IDs preserve unused sounds as well as channel assignments.
        for (int slot = 0; slot < 16; slot++)
        {
            const auto it = model.instruments->find(SlotId(slot));
            if (it != model.instruments->end())
            {
                state.instruments[slot] = it->second;
                slots.emplace(it->first, slot);
            }
        }
    }
    if (!model.instruments || !model.projectChannels) return;
    for (int ch = 0; ch < 16; ch++)
    {
        const auto& channel = (*model.projectChannels)[ch];
        const auto it = model.instruments->find(channel.instrumentId);
        if (it == model.instruments->end()) continue;
        const auto known = slots.find(channel.instrumentId);
        int slot = ch;
        if (known != slots.end())
        {
            slot = known->second;
        }
        else
        {
            for (slot = 0; slot < 16; slot++)
            {
                const bool used = std::any_of(slots.begin(), slots.end(),
                    [slot](const auto& entry) { return entry.second == slot; });
                if (!used) break;
            }
            if (slot == 16) continue;
            state.instruments[slot] = it->second;
            slots.emplace(channel.instrumentId, slot);
        }
        state.channelAssignments[ch] = slot;
        state.channelMixStates[ch] = channel.mix;
    }
}

ProjectModel BuildRuntimeProjectFromGUI(GUIState& state, const char*, bool applyChannelAssignments)
{
    ProjectModel project = BuildProjectModelFromGUI(state);
    if (!applyChannelAssignments)
    {
        auto channels = std::make_shared<std::array<ProjectChannelAssignment, 16>>(*project.projectChannels);
        for (int ch = 0; ch < 16; ch++) (*channels)[ch].instrumentId = SlotId(ch);
        project.projectChannels = channels;
    }
    return project;
}

void OverrideProjectChannelWithSoundSlot(GUIState& state, int previewChannel, int soundSlot, ProjectModel& project)
{
    if (!project.instruments || !project.projectChannels) return;
    auto instruments = std::make_shared<std::map<std::string, InstrumentConfig>>(*project.instruments);
    auto channels = std::make_shared<std::array<ProjectChannelAssignment, 16>>(*project.projectChannels);
    const std::string id = "preview_override";
    (*instruments)[id] = state.instruments[std::clamp(soundSlot, 0, 15)];
    (*channels)[std::clamp(previewChannel, 0, 15)].instrumentId = id;
    project.instruments = instruments;
    project.projectChannels = channels;
}
} // namespace gui
