#include "gui/GUIProjectFacade.h"
#include "gui/GUIPlatform.h"
#include "io/PlatformPaths.h"
#include <algorithm>
#include <map>

namespace gui
{
namespace
{
std::string ChannelInstrumentId(int channel) { return "slot_" + std::to_string(channel); }
}

void PublishLiveRenderSettings(GUIState& state)
{
    const auto previous = state.liveSettings->load(std::memory_order_acquire);
    auto mixes = state.channelMixStates;
    if (state.toneAuditionActive)
        for (auto& mix : mixes) { mix.mute = false; mix.solo = false; }
    bool changed = !previous || previous->mixes != mixes || previous->effects != state.masterEffects;
    for (int ch = 0; ch < 16 && !changed; ++ch)
        changed = previous->sounds[ch] != RenderSound(AudibleInstrument(state, ch));
    if (!changed) return;
    auto next = std::make_shared<LiveRenderSettings>();
    for (int ch = 0; ch < 16; ++ch) next->sounds[ch] = RenderSound(AudibleInstrument(state, ch));
    next->mixes = mixes;
    next->effects = state.masterEffects;
    next->scope = state.audioScope;
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
    for (int ch = 0; ch < 16; ++ch)
    {
        const auto id = ChannelInstrumentId(ch);
        instruments->emplace(id, state.tones[ch].draft.instrument);
        (*channels)[ch] = {true, id, state.channelMixStates[ch]};
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
    // Reset one channel at a time; the full bank is too large for the stack.
    for (auto& part : state.tones) part = {};
    state.toneWorkspaceReady = false;
    if (!model.instruments || !model.projectChannels) return;
    for (int ch = 0; ch < 16; ++ch)
    {
        const auto& assignment = (*model.projectChannels)[ch];
        const auto instrument = model.instruments->find(assignment.instrumentId);
        if (instrument == model.instruments->end()) continue;
        // Imported shared instruments become independent channel copies.
        state.tones[ch].draft.instrument = instrument->second;
        state.channelMixStates[ch] = assignment.mix;
    }
}
} // namespace gui
