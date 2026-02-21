#include "gui/GUIStateModel.h"

#include "gui/GUIConfigUtils.h"
#include "io/PlatformPaths.h"

namespace gui
{
void EnsureChannelConfigs(GUIState& state)
{
    if (state.channelConfigs)
    {
        return;
    }
    AppConfig cfg = DefaultConfig();
    state.channelConfigs = std::make_shared<std::array<ChannelConfig, 16>>();
    if (cfg.channelConfigs)
    {
        *state.channelConfigs = *cfg.channelConfigs;
    }
}

void EnsureChannelMixStates(GUIState& state)
{
    if (state.channelMixStates)
    {
        return;
    }
    AppConfig cfg = DefaultConfig();
    state.channelMixStates = std::make_shared<std::array<ChannelMixState, 16>>();
    if (cfg.channelMixStates)
    {
        *state.channelMixStates = *cfg.channelMixStates;
    }
}

AppConfig BuildConfigFromGUI(const GUIState& state)
{
    AppConfig cfg = DefaultConfig();
    cfg.midiPath = Utf8ToPath(state.midiPath);
    cfg.wavPath = Utf8ToPath(state.wavPath);
    cfg.targetChannel = state.targetChannel;
    cfg.sampleRate = state.sampleRate;
    cfg.initialSeconds = state.initialSeconds;
    cfg.bits = state.bits;
    cfg.extraReleaseSec = state.extraReleaseSec;
    cfg.defaultWave = WaveFromIndex(state.defaultWave);
    if (state.channelConfigs)
    {
        cfg.channelConfigs = std::static_pointer_cast<const std::array<ChannelConfig, 16>>(state.channelConfigs);
    }
    if (state.channelMixStates)
    {
        cfg.channelMixStates = std::static_pointer_cast<const std::array<ChannelMixState, 16>>(state.channelMixStates);
    }
    return cfg;
}
} // namespace gui
