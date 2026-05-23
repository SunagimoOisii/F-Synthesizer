#include "core/RenderGateway.h"

#include "SynthEngine/SynthEngine.h"

void RenderWithEngine(
    SoundData& sound,
    const RenderConfig& config,
    const std::function<bool()>& shouldCancel,
    bool* canceled)
{
    // app 層から SynthEngine への単一入口。
    // 境界を固定しておくことで、将来の実装差し替え時に呼び出し側を汚染しない。
    RenderMIDIEvents(
        sound,
        config.events,
        config.channelConfigs,
        config.channelMixStates,
        config.effects,
        &config.tempoEvents,
        config.ticksPerQuarter,
        config.renderStartSec,
        shouldCancel,
        canceled);
}

void RenderWithEngine(
    SoundData& sound,
    const std::vector<MIDIEvent>& events,
    const std::vector<TempoEvent>& tempoEvents,
    int ticksPerQuarter,
    double renderStartSec,
    const std::array<ChannelConfig, 16>& channelConfigs,
    const std::array<ChannelMixState, 16>& channelMixStates,
    const MasterEffectConfig& effects,
    const std::function<bool()>& shouldCancel,
    bool* canceled)
{
    const RenderConfig config{
        events,
        tempoEvents,
        ticksPerQuarter,
        renderStartSec,
        channelConfigs,
        channelMixStates,
        effects
    };
    RenderWithEngine(sound, config, shouldCancel, canceled);
}
