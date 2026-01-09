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
        double bendSemis = bend * 2.0;
        state.channelPitch[ch] = std::pow(2.0, bendSemis / 12.0);
    }

    void HandleNoteOff(const MIDIEvent& e, std::vector<Voice>& voices)
    {
        for (auto& v : voices)
        {
            if (std::holds_alternative<DrumConfig>(v.source))
            {
                continue;
            }
            if (!v.released && v.noteNumber == e.noteNumber && v.channel == e.channel)
            {
                NoteOff(v.env);
                v.released = true;
                break;
            }
        }
    }
}

void ProcessEventsAtSample(const std::vector<MIDIEvent>& events,
    int sampleIndex,
    const std::array<ChannelConfig, 16>& channelConfigs,
    int sampleRate,
    RenderState& state)
{
    //イベント処理(ControlChange, Note)
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
                int note = e.noteNumber;
                if (note < 0) note = 0;
                if (note > 127) note = 127;
                const DrumConfig& drum = kit->map[note];
                if (drum.type != DrumType::None)
                {
                    ChannelConfig drumCfg = cfg;
                    drumCfg.source = drum;
                    state.voices.push_back(MakeVoiceFromConfig(drumCfg, e, sampleRate));
                }
            }
            else
            {
                state.voices.push_back(MakeVoiceFromConfig(cfg, e, sampleRate));
            }
        }
        else
        {
            HandleNoteOff(e, state.voices);
        }
        state.eventIndex++;
    }
}

