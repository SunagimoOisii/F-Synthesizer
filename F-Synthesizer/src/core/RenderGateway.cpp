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
    // app 層から SynthEngine への単一入口。
    // 境界を固定しておくことで、将来の実装差し替え時に呼び出し側を汚染しない。
    RenderMIDIEvents(sound, events, channelConfigs, channelMixStates, shouldCancel, canceled);
}
