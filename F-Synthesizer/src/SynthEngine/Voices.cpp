#include "Internal.h"

#include <cmath>

#include "Oscillator.h"

namespace
{
    constexpr double kPi = 3.14159265358979323846;

    void InitDrumVoice(const DrumConfig& drum, Voice& v, int sampleRate)
    {
        if (drum.type == DrumType::Kick)
        {
            v.drumBaseFreq = (drum.baseFreq > 0.0) ? drum.baseFreq : 60.0;
            v.drumPitchDrop = (drum.pitchDrop > 0.0) ? drum.pitchDrop : 3.0;
            v.drumPitchDecaySec = (drum.pitchDecaySec > 0.0) ? drum.pitchDecaySec : 0.06;
            v.phaseInc = v.drumBaseFreq / sampleRate;
        }
        else if (drum.type == DrumType::Snare)
        {
            v.drumBaseFreq = (drum.toneFreq > 0.0) ? drum.toneFreq : 200.0;
            v.phaseInc = v.drumBaseFreq / sampleRate;
            double hpCut = (drum.hpCut > 0.0) ? drum.hpCut : 1200.0;
            v.drumHpAlpha = std::exp(-2.0 * kPi * hpCut / sampleRate);
            double lpCut = (drum.lpCut > 0.0) ? drum.lpCut : 6000.0;
            v.drumLpAlpha = std::exp(-2.0 * kPi * lpCut / sampleRate);
        }
        else if (drum.type == DrumType::Hat)
        {
            double hpCut = (drum.hpCut > 0.0) ? drum.hpCut : 6000.0;
            v.drumHpAlpha = std::exp(-2.0 * kPi * hpCut / sampleRate);
            double lpCut = (drum.lpCut > 0.0) ? drum.lpCut : 12000.0;
            v.drumLpAlpha = std::exp(-2.0 * kPi * lpCut / sampleRate);
            v.drumBaseFreq = (drum.toneFreq > 0.0) ? drum.toneFreq : 8000.0;
        }
    }
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

    //Drum パラメータ
    v.drumTime = 0.0;
    v.drumBaseFreq = 0.0;
    v.drumPitchDrop = 1.0;
    v.drumPitchDecaySec = 0.0;
    v.drumNoisePrev = 0.0;
    v.drumHpPrev = 0.0;
    v.drumHpAlpha = 0.0;
    v.drumLpPrev = 0.0;
    v.drumLpAlpha = 0.0;

    if (const auto* drum = std::get_if<DrumConfig>(&cfg.source))
    {
        InitDrumVoice(*drum, v, sampleRate);
    }
    return v;
}

