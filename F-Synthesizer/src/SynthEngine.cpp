#include "SynthEngine.h"

#include <array>

#include "Oscillator.h"

void RenderMidiEvents(
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
    for (int i = 0; i < sound.length; i++)
    {
        double sum = 0.0;

        //イベント処理(ControlChange, Note)
        while (eventIndex < events.size() && events[eventIndex].sample <= i)
        {
            const auto& e = events[eventIndex];
            if (e.type == MIDIEventType::ControlChange)
            {
                if (e.controller == 7 || e.controller == 11)
                {
                    int ch = (e.channel >= 0 && e.channel < 16) ? e.channel : 0;
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
            }
            else if (e.isNoteOn)
            {
                const ChannelConfig& cfg = channelConfigs[(e.channel >= 0 && e.channel < 16) ? e.channel : 0];

                Voice v{};
                //識別, 状態
                v.mode       = cfg.mode;
                v.type       = cfg.type;
                v.noteNumber = e.noteNumber;
                v.velocity   = e.velocity;
                v.channel    = e.channel;
                v.released   = false;

                //レベル, エンベロープ
                v.amp          = cfg.amp;
                v.attackSec    = cfg.attackSec;
                v.decaySec     = cfg.decaySec;
                v.sustainLevel = cfg.sustainLevel;
                v.releaseSec   = cfg.releaseSec;
                NoteOn(v.env);

                //基本波形位相
                v.phase = 0.0;

                //FM パラメータ
                v.fmCarrierPhase = 0.0;
                v.fmModPhase     = 0.0;
                v.fmCarrierRatio = cfg.fmCarrierRatio;
                v.fmModRatio     = cfg.fmModRatio;
                v.fmIndex        = cfg.fmIndex;
                v.fmOutLevel     = cfg.fmOutLevel;
                voices.push_back(v);
            }
            else //NoteOff
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
            eventIndex++;
        }

        //Voice合成
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
            int ch = (v.channel >= 0 && v.channel < 16) ? v.channel : 0;
            if (v.mode == SynthMode::FM)
            {
                double carrierFreq = freq * v.fmCarrierRatio;
                double modFreq = freq * v.fmModRatio;
                w = SampleFmPhase(v.fmCarrierPhase, v.fmModPhase, v.fmIndex);
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
        sound.data[i] = sum;
    }
}

