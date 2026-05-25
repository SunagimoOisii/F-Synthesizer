#include "Internal.h"

#include <algorithm>
#include <cmath>

namespace
{
    constexpr int kPitchBendRangeMinSemis = 2;
    constexpr int kPitchBendRangeMaxSemis = 12;

    double NormalizeCc(int value)
    {
        int v = value;
        if (v < 0) v = 0;
        if (v > 127) v = 127;
        return v / 127.0;
    }

    double NormalizeCcCentered(int value)
    {
        return std::clamp((value - 64) / 63.0, -1.0, 1.0);
    }

    void RecomputePitchFactor(int ch, RenderState& state)
    {
        const double bendSemis = state.channelPitchBendNorm[ch] * state.channelPitchBendRangeSemis[ch];
        state.channelPitch[ch] = std::pow(2.0, bendSemis / 12.0);
    }

    bool IsPitchBendRpnSelected(int ch, const RenderState& state)
    {
        return state.channelRpnMsb[ch] == 0 && state.channelRpnLsb[ch] == 0;
    }

    // 目的: 主要CCをチャンネル状態へ反映する。
    void ApplyControlChange(const MIDIEvent& e, RenderState& state)
    {
        int ch = ClampChannel(e.channel);
        const double norm = NormalizeCc(e.value);
        switch (e.controller)
        {
        case 1:
            state.channelModwheel[ch] = norm;
            break;
        case 5:
            // CC5 0..127 を 0..2sec に変換（CC65 ON時のみ有効）。
            state.channelPortamentoTimeSec[ch] = norm * 2.0;
            break;
        case 7:
            state.channelCc7[ch] = norm;
            RecomputeChannelCcGain(state, ch);
            break;
        case 11:
            state.channelCc11[ch] = norm;
            RecomputeChannelCcGain(state, ch);
            break;
        case 64:
        {
            const bool sustainOn = (e.value >= 64);
            if (state.channelSustain[ch] && !sustainOn)
            {
                state.voices.ReleaseSustained(ch);
            }
            state.channelSustain[ch] = sustainOn;
            break;
        }
        case 65:
            state.channelPortamentoOn[ch] = (e.value >= 64);
            break;
        case 70:
            state.channelAdsrOffset[ch].sustain = NormalizeCcCentered(e.value);
            RecomputeChannelAdsrCache(state, ch);
            break;
        case 71:
            state.channelResonance[ch] = norm;
            RecomputeChannelToneCache(state, ch);
            break;
        case 72:
            state.channelAdsrOffset[ch].release = NormalizeCcCentered(e.value);
            RecomputeChannelAdsrCache(state, ch);
            break;
        case 73:
            state.channelAdsrOffset[ch].attack = NormalizeCcCentered(e.value);
            RecomputeChannelAdsrCache(state, ch);
            break;
        case 74:
            state.channelBrightness[ch] = norm;
            RecomputeChannelToneCache(state, ch);
            break;
        case 75:
            state.channelAdsrOffset[ch].decay = NormalizeCcCentered(e.value);
            RecomputeChannelAdsrCache(state, ch);
            break;
        case 101:
            state.channelRpnMsb[ch] = std::clamp(e.value, 0, 127);
            break;
        case 100:
            state.channelRpnLsb[ch] = std::clamp(e.value, 0, 127);
            break;
        case 6:
            if (IsPitchBendRpnSelected(ch, state))
            {
                const int coarse = std::clamp(e.value, kPitchBendRangeMinSemis, kPitchBendRangeMaxSemis);
                const int fine = static_cast<int>(std::round((state.channelPitchBendRangeSemis[ch] - coarse) * 100.0));
                // F-2仕様: レンジは ±2..±12 半音。上限時の fine は 0 に固定する。
                const int clampedFine = (coarse >= kPitchBendRangeMaxSemis) ? 0 : std::clamp(fine, 0, 99);
                state.channelPitchBendRangeSemis[ch] = coarse + clampedFine / 100.0;
                RecomputePitchFactor(ch, state);
            }
            break;
        case 38:
            if (IsPitchBendRpnSelected(ch, state))
            {
                const int coarse = std::clamp(
                    static_cast<int>(state.channelPitchBendRangeSemis[ch]),
                    kPitchBendRangeMinSemis,
                    kPitchBendRangeMaxSemis);
                const int fine = (coarse >= kPitchBendRangeMaxSemis) ? 0 : std::clamp((e.value * 100) / 127, 0, 99);
                state.channelPitchBendRangeSemis[ch] = coarse + fine / 100.0;
                RecomputePitchFactor(ch, state);
            }
            break;
        default:
            break;
        }
    }

    // 目的: MIDI Pitch Bend(14bit) をピッチ比へ変換して保持する。
    void ApplyPitchBend(const MIDIEvent& e, RenderState& state)
    {
        int ch = ClampChannel(e.channel);
        int v = e.value;
        if (v < 0) v = 0;
        if (v > 16383) v = 16383;
        state.channelPitchBendNorm[ch] = (v - 8192) / 8192.0;
        RecomputePitchFactor(ch, state);
    }

    void HandleNoteOff(const MIDIEvent& e, RenderState& state)
    {
        const int ch = ClampChannel(e.channel);
        const bool holdBySustain = state.channelSustain[ch];
        state.voices.MarkNoteOff(e.channel, e.noteNumber, e.noteInstanceID, holdBySustain);
    }

    void ApplyChannelPressure(const MIDIEvent& e, RenderState& state)
    {
        const int ch = ClampChannel(e.channel);
        state.channelPressure[ch] = NormalizeCc(e.value);
    }

    void ApplyPolyPressure(const MIDIEvent& e, RenderState& state)
    {
        const int ch = ClampChannel(e.channel);
        const int note = std::clamp(e.noteNumber, 0, 127);
        const double norm = NormalizeCc(e.value);
        state.channelPolyPressure[ch][note] = norm;
        if (norm > 0.0)
        {
            state.channelHasPolyPressure[ch] = true;
        }
    }

    InstrumentSoundConfig ResolveRealtimeInstrumentSound(const InstrumentSoundConfig& cfg, int channel, const RenderState& state)
    {
        InstrumentSoundConfig resolved = cfg;
        const int ch = ClampChannel(channel);
        if (state.channelPortamentoOn[ch])
        {
            resolved.portamentoTimeSec = (std::max)(cfg.portamentoTimeSec, state.channelPortamentoTimeSec[ch]);
        }
        else
        {
            resolved.portamentoTimeSec = 0.0;
        }
        return resolved;
    }
}

void ProcessEventsAtSample(const std::vector<MIDIEvent>& events,
    int sampleIndex,
    const std::array<InstrumentSoundConfig, 16>& soundSlots,
    int sampleRate,
    RenderState& state)
{
    // 呼び出し前提: audio thread のサンプルループから時系列順で呼ぶ。
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
        if (e.type == MIDIEventType::ChannelPressure)
        {
            ApplyChannelPressure(e, state);
            state.eventIndex++;
            continue;
        }
        if (e.type == MIDIEventType::PolyPressure)
        {
            ApplyPolyPressure(e, state);
            state.eventIndex++;
            continue;
        }
        if (e.type == MIDIEventType::ProgramChange)
        {
            state.eventIndex++;
            continue;
        }

        if (e.isNoteOn)
        {
            const InstrumentSoundConfig& cfg = soundSlots[ClampChannel(e.channel)];
            if (const auto* kit = std::get_if<DrumKitConfig>(&cfg.source))
            {
                // DrumKit は note -> DrumConfig へ展開してから通常 Voice として投入する。
                int note = e.noteNumber;
                if (note < 0) note = 0;
                if (note > 127) note = 127;
                const DrumConfig& drum = kit->map[note];
                if (drum.type != DrumType::None)
                {
                    InstrumentSoundConfig drumCfg = ResolveRealtimeInstrumentSound(cfg, e.channel, state);
                    drumCfg.source = drum;
                    drumCfg.drumBus = kit->drumBus;
                    MIDIEvent drumEvent = e;
                    const double ceiling = std::clamp(kit->velocityCeiling, 0.0, 1.0);
                    const double curve = std::clamp(kit->velocityCurve, 0.2, 3.0);
                    const double velNorm = std::clamp(static_cast<double>(drumEvent.velocity) / 127.0, 0.0, 1.0);
                    const double shaped = std::pow(velNorm, curve);
                    drumEvent.velocity = std::clamp(
                        static_cast<int>(std::round(127.0 * std::min(shaped, ceiling))),
                        0,
                        127);
                    state.voices.AddVoice(drumCfg, drumEvent, sampleRate);
                    MarkActiveVoiceIndicesDirty(state);
                }
            }
            else
            {
                const InstrumentSoundConfig resolvedCfg = ResolveRealtimeInstrumentSound(cfg, e.channel, state);
                state.voices.AddVoice(resolvedCfg, e, sampleRate);
                MarkActiveVoiceIndicesDirty(state);
            }
        }
        else
        {
            HandleNoteOff(e, state);
        }
        state.eventIndex++;
    }
}
