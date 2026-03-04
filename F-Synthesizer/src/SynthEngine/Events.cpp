#include "Internal.h"

#include <cmath>

namespace
{
    void ApplyControlChange(const MIDIEvent& e, RenderState& state)
    {
        if (e.controller != 7 && e.controller != 11)
        {
            return;
        }

        int ch = ClampChannel(e.channel);
        int v = e.value;
        if (v < 0) v = 0;
        if (v > 127) v = 127;
        // 範囲外入力は 0..127 に丸めて 0..1 に正規化し、ミックスゲイン計算を安定させる。
        double norm = v / 127.0;
        if (e.controller == 7)
        {
            state.channelCc7[ch] = norm;
        }
        else
        {
            state.channelCc11[ch] = norm;
        }
    }

    void ApplyPitchBend(const MIDIEvent& e, RenderState& state)
    {
        int ch = ClampChannel(e.channel);
        int v = e.value;
        if (v < 0) v = 0;
        if (v > 16383) v = 16383;
        double bend = (v - 8192) / 8192.0;
        // 現実装は ±2 半音を基準に、ピッチ比へ変換する。
        double bendSemis = bend * 2.0;
        state.channelPitch[ch] = std::pow(2.0, bendSemis / 12.0);
    }

    void HandleNoteOff(const MIDIEvent& e, VoicesSoA& voices)
    {
        voices.MarkNoteOff(e.channel, e.noteNumber, e.noteInstanceID);
    }
}

void ProcessEventsAtSample(const std::vector<MIDIEvent>& events,
    int sampleIndex,
    const std::array<ChannelConfig, 16>& channelConfigs,
    int sampleRate,
    RenderState& state)
{
    // sampleIndex までに到達したイベントを順次適用する。
    // 目的: RenderVoices 実行前に CC/Pitch/Note 状態をサンプル境界で確定させる。
    while (state.eventIndex < events.size() && events[state.eventIndex].sample <= sampleIndex)
    {
        const auto& e = events[state.eventIndex];
        if (e.type == MIDIEventType::ControlChange)
        {
            ApplyControlChange(e, state);
            state.eventIndex++;
            continue;
        }

        if (e.type == MIDIEventType::PitchBend)
        {
            ApplyPitchBend(e, state);
            state.eventIndex++;
            continue;
        }

        if (e.isNoteOn)
        {
            const ChannelConfig& cfg = channelConfigs[ClampChannel(e.channel)];
            if (const auto* kit = std::get_if<DrumKitConfig>(&cfg.source))
            {
                // DrumKit は note -> DrumConfig へ展開してから通常 Voice として投入する。
                int note = e.noteNumber;
                if (note < 0) note = 0;
                if (note > 127) note = 127;
                const DrumConfig& drum = kit->map[note];
                if (drum.type != DrumType::None)
                {
                    ChannelConfig drumCfg = cfg;
                    drumCfg.source = drum;
                    state.voices.AddVoice(drumCfg, e, sampleRate);
                }
            }
            else
            {
                state.voices.AddVoice(cfg, e, sampleRate);
            }
        }
        else
        {
            HandleNoteOff(e, state.voices);
        }
        state.eventIndex++;
    }
}
