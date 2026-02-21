#include "core/RenderGateway.h"

#include "SynthEngine/SynthEngine.h"

void RenderWithEngine(
    SoundData& sound,
    const std::vector<MIDIEvent>& events,
    const std::array<ChannelConfig, 16>& channelConfigs,
    const std::array<ChannelMixState, 16>& channelMixStates,
    const std::function<bool()>& shouldCancel,
    bool* canceled)
{
    RenderMIDIEvents(sound, events, channelConfigs, channelMixStates, shouldCancel, canceled);
}
