#include "Internal.h"

#include <algorithm>

namespace
{
void CleanupVoices(RenderState& state)
{
    if (state.voices.empty() || state.pendingRemoveCount == 0)
    {
        return;
    }

    size_t removed = 0;
    state.voices.erase(
        std::remove_if(state.voices.begin(), state.voices.end(), [&](const Voice& v)
            {
                if (v.pendingRemove)
                {
                    removed++;
                    return true;
                }
                return false;
            }),
        state.voices.end());

    if (removed > 0)
    {
        state.pendingRemoveCount = 0;
    }
}
} // namespace

void RenderMIDIEvents(
    SoundData& sound,
    const std::vector<MIDIEvent>& events,
    const std::array<ChannelConfig, 16>& channelConfigs)
{
    RenderState state;
    for (int i = 0; i < 16; i++)
    {
        state.channelCc7[i] = 1.0;
        state.channelCc11[i] = 1.0;
        state.channelPitch[i] = 1.0;
    }

    const int cleanupInterval = 256;
    for (int i = 0; i < sound.length; i++)
    {
        ProcessEventsAtSample(events, i, channelConfigs, sound.fs, state);
        sound.data[i] = RenderVoices(state, sound);

        if ((i % cleanupInterval) == 0)
        {
            CleanupVoices(state);
        }
    }
}
