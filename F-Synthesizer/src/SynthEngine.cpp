#include "SynthEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <type_traits>

#include "Oscillator.h"

int ClampChannel(int channel);

Voice MakeVoiceFromConfig(const ChannelConfig& cfg, const MIDIEvent& e, int sampleRate);

void ApplyControlChange(const MIDIEvent& e,
    std::array<double, 16>& channelCc7,
    std::array<double, 16>& channelCc11);

void ApplyPitchBend(const MIDIEvent& e,
    std::array<double, 16>& channelPitch);

void HandleNoteOff(const MIDIEvent& e, std::vector<Voice>& voices);

void ProcessEventsAtSample(size_t& eventIndex,
    const std::vector<MIDIEvent>& events,
    int sampleIndex,
    const std::array<ChannelConfig, 16>& channelConfigs,
    int sampleRate,
    std::vector<Voice>& voices,
    std::array<double, 16>& channelCc7,
    std::array<double, 16>& channelCc11,
    std::array<double, 16>& channelPitch);

double RenderVoices(std::vector<Voice>& voices,
    const SoundData& sound,
    const std::array<double, 16>& channelCc7,
    const std::array<double, 16>& channelCc11,
    const std::array<double, 16>& channelPitch,
    size_t& pendingRemoveCount);

void RenderMIDIEvents(
    SoundData& sound,
    const std::vector<MIDIEvent>& events,
    const std::array<ChannelConfig, 16>& channelConfigs)
{
    //Voice, CC初期化
    std::vector<Voice> voices;
    size_t eventIndex = 0;
    size_t pendingRemoveCount = 0;
    std::array<double, 16> channelCc7{};
    std::array<double, 16> channelCc11{};
    std::array<double, 16> channelPitch{};
    for (int i = 0; i < 16; i++)
    {
        channelCc7[i] = 1.0;
        channelCc11[i] = 1.0;
        channelPitch[i] = 1.0;
    }

    //サンプルループ
    const int cleanupInterval = 256;
    for (int i = 0; i < sound.length; i++)
    {
        ProcessEventsAtSample(eventIndex, events, i, channelConfigs, sound.fs, voices, channelCc7, channelCc11, channelPitch);
        sound.data[i] = RenderVoices(voices, sound, channelCc7, channelCc11, channelPitch, pendingRemoveCount);

        if ((i % cleanupInterval) == 0 && !voices.empty() && pendingRemoveCount > 0)
        {
            size_t removed = 0;
            voices.erase(
                std::remove_if(voices.begin(), voices.end(), [&](const Voice& v)
                    {
                        if (v.pendingRemove)
                        {
                            removed++;
                            return true;
                        }
                        return false;
                    }),
                voices.end());
            if (removed > 0)
            {
                pendingRemoveCount = 0;
            }
        }
    }
}

void ProcessEventsAtSample(size_t& eventIndex,
    const std::vector<MIDIEvent>& events,
    int sampleIndex,
    const std::array<ChannelConfig, 16>& channelConfigs,
    int sampleRate,
    std::vector<Voice>& voices,
    std::array<double, 16>& channelCc7,
    std::array<double, 16>& channelCc11,
    std::array<double, 16>& channelPitch)
{
    //イベント処理(ControlChange, Note)
    while (eventIndex < events.size() && events[eventIndex].sample <= sampleIndex)
    {
        const auto& e = events[eventIndex];
        if (e.type == MIDIEventType::ControlChange)
        {
            ApplyControlChange(e, channelCc7, channelCc11);
            eventIndex++;
            continue;
        }

        if (e.type == MIDIEventType::PitchBend)
        {
            ApplyPitchBend(e, channelPitch);
            eventIndex++;
            continue;
        }

        if (e.isNoteOn)
        {
            const ChannelConfig& cfg = channelConfigs[ClampChannel(e.channel)];
            voices.push_back(MakeVoiceFromConfig(cfg, e, sampleRate));
        }
        else
        {
            HandleNoteOff(e, voices);
        }
        eventIndex++;
    }
}

void ApplyControlChange(const MIDIEvent& e,
    std::array<double, 16>& channelCc7,
    std::array<double, 16>& channelCc11)
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
        channelCc7[ch] = norm;
    }
    else
    {
        channelCc11[ch] = norm;
    }
}

void ApplyPitchBend(const MIDIEvent& e,
    std::array<double, 16>& channelPitch)
{
    int ch = ClampChannel(e.channel);
    int v = e.value;
    if (v < 0) v = 0;
    if (v > 16383) v = 16383;
    double bend = (v - 8192) / 8192.0;
    double bendSemis = bend * 2.0;
    channelPitch[ch] = std::pow(2.0, bendSemis / 12.0);
}

Voice MakeVoiceFromConfig(const ChannelConfig& cfg, const MIDIEvent& e, int sampleRate)
{
    Voice v{};
    //識別, 状態
    v.source = cfg.source;
    v.noteNumber = e.noteNumber;
    v.velocity = e.velocity;
    v.channel = e.channel;
    v.channelIndex = ClampChannel(e.channel);
    v.released = false;
    v.pendingRemove = false;

    //レベル, エンベロープ
    v.amp = cfg.amp;
    v.attackSec = cfg.attackSec;
    v.decaySec = cfg.decaySec;
    v.sustainLevel = cfg.sustainLevel;
    v.releaseSec = cfg.releaseSec;
    NoteOn(v.env);

    //基本波形位相
    v.phase = 0.0;
    v.phaseInc = NoteNumberToFreq(v.noteNumber) / sampleRate;

    //FM パラメータ
    v.fmCarrierPhase = 0.0;
    v.fmModPhase = 0.0;
    return v;
}

void HandleNoteOff(const MIDIEvent& e, std::vector<Voice>& voices)
{
    for (auto& v : voices)
    {
        if (!v.released && v.noteNumber == e.noteNumber && v.channel == e.channel)
        {
            NoteOff(v.env);
            v.released = true;
            break;
        }
    }
}

double RenderVoices(std::vector<Voice>& voices,
    const SoundData& sound,
    const std::array<double, 16>& channelCc7,
    const std::array<double, 16>& channelCc11,
    const std::array<double, 16>& channelPitch,
    size_t& pendingRemoveCount)
{
    //Voice合成
    double sum = 0.0;
    for (auto& v : voices)
    {
        if (v.env.stage == ADSRStage::Off)
        {
            continue;
        }

        double envGain = StepADSR(v.env, 1.0 / sound.fs, v.attackSec, v.decaySec, v.sustainLevel, v.releaseSec);
        if (!v.pendingRemove && v.env.stage == ADSRStage::Off)
        {
            v.pendingRemove = true;
            pendingRemoveCount++;
            continue;
        }
        double w = 0.0;
        double velGain = VelocityToGain(v.velocity);
        int ch = v.channelIndex;
        double pitchFactor = channelPitch[ch];

        std::visit([&](const auto& src)
        {
            using T = std::decay_t<decltype(src)>;
            if constexpr (std::is_same_v<T, WaveformConfig>)
            {
                w = SampleWavePhase(src.wave, v.phase);
                sum += v.amp * channelCc7[ch] * channelCc11[ch] * velGain * w * envGain;

                v.phase += v.phaseInc * pitchFactor;
                if (v.phase >= 1.0) v.phase -= 1.0;
            }
            else if constexpr (std::is_same_v<T, NoiseConfig>)
            {
                w = SampleNoise(src.noise);
                sum += v.amp * channelCc7[ch] * channelCc11[ch] * velGain * w * envGain;
            }
            else if constexpr (std::is_same_v<T, FmConfig>)
            {
                w = SampleFmPhase(src.carrierWave, src.modWave, v.fmCarrierPhase, v.fmModPhase, src.index);
                sum += v.amp * src.outLevel * channelCc7[ch] * channelCc11[ch] * velGain * w * envGain;

                v.fmCarrierPhase += v.phaseInc * pitchFactor * src.carrierRatio;
                if (v.fmCarrierPhase >= 1.0) v.fmCarrierPhase -= 1.0;
                v.fmModPhase += v.phaseInc * pitchFactor * src.modRatio;
                if (v.fmModPhase >= 1.0) v.fmModPhase -= 1.0;
            }
        }, v.source);
    }
    return sum;
}

int ClampChannel(int channel)
{
    return (channel >= 0 && channel < 16) ? channel : 0;
}


