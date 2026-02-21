#include "Internal.h"

#include <cmath>

namespace
{
double PanToMonoGain(double pan)
{
    if (pan < -1.0) pan = -1.0;
    if (pan > 1.0) pan = 1.0;
    // Mono renderer approximation: reduce level as pan moves away from center.
    return 1.0 - 0.5 * std::abs(pan);
}

void CleanupVoices(RenderState& state)
{
    if (state.voices.empty() || state.pendingRemoveCount == 0)
    {
        return;
    }

    const size_t removed = state.voices.CleanupPending();

    if (removed > 0)
    {
        state.pendingRemoveCount = 0;
    }
}
} // namespace

void RenderMIDIEvents(
    SoundData& sound,
    const std::vector<MIDIEvent>& events,
    const std::array<ChannelConfig, 16>& channelConfigs,
    const std::array<ChannelMixState, 16>& channelMixStates,
    const std::function<bool()>& shouldCancel,
    bool* canceled)
{
    if (canceled != nullptr)
    {
        *canceled = false;
    }

    RenderState state;
    state.voices.reserve(256);
    for (int i = 0; i < 16; i++)
    {
        state.channelCc7[i] = 1.0;
        state.channelCc11[i] = 1.0;
        state.channelPitch[i] = 1.0;
        const ChannelMixState& mix = channelMixStates[i];
        state.channelMute[i] = mix.mute;
        state.channelSolo[i] = mix.solo;
        if (mix.solo)
        {
            state.hasAnySolo = true;
        }
        state.channelMixGain[i] = mix.level * mix.gain * PanToMonoGain(mix.pan);
    }

    const int cleanupInterval = 256;
    for (int i = 0; i < sound.length; i++)
    {
        if (shouldCancel && ((i % cleanupInterval) == 0) && shouldCancel())
        {
            if (canceled != nullptr)
            {
                *canceled = true;
            }
            break;
        }

        ProcessEventsAtSample(events, i, channelConfigs, sound.fs, state);
        sound.data[i] = RenderVoices(state, sound);

        if ((i % cleanupInterval) == 0)
        {
            CleanupVoices(state);
        }
    }
}
