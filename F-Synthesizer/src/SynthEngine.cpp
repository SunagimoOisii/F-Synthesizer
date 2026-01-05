#include "SynthEngine.h"

#include <algorithm>
#include <array>

#include "Oscillator.h"

int ClampChannel(int channel);

Voice MakeVoiceFromConfig(const ChannelConfig& cfg, const MIDIEvent& e);

void ApplyControlChange(const MIDIEvent& e,
    std::array<double, 16>& channelCc7,
    std::array<double, 16>& channelCc11);

void HandleNoteOff(const MIDIEvent& e, std::vector<Voice>& voices);

void ProcessEventsAtSample(size_t& eventIndex,
    const std::vector<MIDIEvent>& events,
    int sampleIndex,
    const std::array<ChannelConfig, 16>& channelConfigs,
    std::vector<Voice>& voices,
    std::array<double, 16>& channelCc7,
    std::array<double, 16>& channelCc11);

double RenderVoices(std::vector<Voice>& voices,
    const SoundData& sound,
    const std::array<double, 16>& channelCc7,
    const std::array<double, 16>& channelCc11);

void RenderMIDIEvents(
    SoundData& sound,
    const std::vector<MIDIEvent>& events,
    const std::array<ChannelConfig, 16>& channelConfigs)
{
    //Voice, CC初期化
    std::vector<Voice> voices;
    size_t eventIndex = 0;
    std::array<double, 16> channelCc7{};
    std::array<double, 16> channelCc11{};
    for (int i = 0; i < 16; i++)
    {
        channelCc7[i] = 1.0;
        channelCc11[i] = 1.0;
    }

    //サンプルループ
    const int cleanupInterval = 256;
    for (int i = 0; i < sound.length; i++)
    {
        ProcessEventsAtSample(eventIndex, events, i, channelConfigs, voices, channelCc7, channelCc11);
        sound.data[i] = RenderVoices(voices, sound, channelCc7, channelCc11);

        if ((i % cleanupInterval) == 0 && !voices.empty())
        {
            voices.erase(
                std::remove_if(voices.begin(), voices.end(), [](const Voice& v)
                    {
                        return v.env.stage == ADSRStage::Off;
                    }),
                voices.end());
        }
    }
}

void ProcessEventsAtSample(size_t& eventIndex,
    const std::vector<MIDIEvent>& events,
    int sampleIndex,
    const std::array<ChannelConfig, 16>& channelConfigs,
    std::vector<Voice>& voices,
    std::array<double, 16>& channelCc7,
    std::array<double, 16>& channelCc11)
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

        if (e.isNoteOn)
        {
            const ChannelConfig& cfg = channelConfigs[ClampChannel(e.channel)];
            voices.push_back(MakeVoiceFromConfig(cfg, e));
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

Voice MakeVoiceFromConfig(const ChannelConfig& cfg, const MIDIEvent& e)
{
    Voice v{};
    //識別, 状態
    v.mode = cfg.mode;
    v.source = cfg.source;
    v.type = cfg.type;
    v.noiseType = cfg.noiseType;
    v.noteNumber = e.noteNumber;
    v.velocity = e.velocity;
    v.channel = e.channel;
    v.released = false;

    //レベル, エンベロープ
    v.amp = cfg.amp;
    v.attackSec = cfg.attackSec;
    v.decaySec = cfg.decaySec;
    v.sustainLevel = cfg.sustainLevel;
    v.releaseSec = cfg.releaseSec;
    NoteOn(v.env);

    //基本波形位相
    v.phase = 0.0;

    //FM パラメータ
    v.fmCarrierWave = cfg.fmCarrierWave;
    v.fmCarrierPhase = 0.0;
    v.fmModPhase = 0.0;
    v.fmCarrierRatio = cfg.fmCarrierRatio;
    v.fmModRatio = cfg.fmModRatio;
    v.fmIndex = cfg.fmIndex;
    v.fmOutLevel = cfg.fmOutLevel;
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
    const std::array<double, 16>& channelCc11)
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
        double freq = NoteNumberToFreq(v.noteNumber);
        double w = 0.0;
        double velGain = VelocityToGain(v.velocity);
        int ch = ClampChannel(v.channel);

        if (v.source == SourceType::Noise)
        {
            w = SampleNoise(v.noiseType);
            sum += v.amp * channelCc7[ch] * channelCc11[ch] * velGain * w * envGain;
            continue;
        }

        if (v.mode == SynthMode::FM)
        {
            double carrierFreq = freq * v.fmCarrierRatio;
            double modFreq = freq * v.fmModRatio;
            w = SampleFmPhase(v.fmCarrierWave, v.fmCarrierPhase, v.fmModPhase, v.fmIndex);
            sum += v.amp * v.fmOutLevel * channelCc7[ch] * channelCc11[ch] * velGain * w * envGain;

            v.fmCarrierPhase += carrierFreq / sound.fs;
            if (v.fmCarrierPhase >= 1.0) v.fmCarrierPhase -= 1.0;
            v.fmModPhase += modFreq / sound.fs;
            if (v.fmModPhase >= 1.0) v.fmModPhase -= 1.0;
        }
        else
        {
            w = SampleWavePhase(v.type, v.phase);
            sum += v.amp * channelCc7[ch] * channelCc11[ch] * velGain * w * envGain;

            v.phase += freq / sound.fs;
            if (v.phase >= 1.0) v.phase -= 1.0;
        }
    }
    return sum;
}

int ClampChannel(int channel)
{
    return (channel >= 0 && channel < 16) ? channel : 0;
}


