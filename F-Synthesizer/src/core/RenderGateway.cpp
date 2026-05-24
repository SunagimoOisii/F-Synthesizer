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

void RenderWithEngineFrames(
    int length,
    int sampleRate,
    const RenderConfig& config,
    const std::function<bool(int, double, double)>& onFrame,
    const std::function<bool()>& shouldCancel,
    bool* canceled)
{
    RenderMIDIEventsWithFrameCallback(
        length,
        sampleRate,
        config.events,
        config.channelConfigs,
        config.channelMixStates,
        onFrame,
        config.effects,
        &config.tempoEvents,
        config.ticksPerQuarter,
        config.renderStartSec,
        shouldCancel,
        canceled);
}

void RenderWithEngineFrameBlocks(
    int length,
    int sampleRate,
    const RenderConfig& config,
    const std::function<bool(int, const double*, int)>& onFrames,
    const std::function<bool()>& shouldCancel,
    bool* canceled)
{
    RenderMIDIEventsWithFrameBlockCallback(
        length,
        sampleRate,
        config.events,
        config.channelConfigs,
        config.channelMixStates,
        onFrames,
        config.effects,
        &config.tempoEvents,
        config.ticksPerQuarter,
        config.renderStartSec,
        shouldCancel,
        canceled);
}
