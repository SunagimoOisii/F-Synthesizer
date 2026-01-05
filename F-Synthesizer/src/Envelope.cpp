#include "Envelope.h"

double StepADSR(ADSRState& env, double deltaTimeSec,
    double attackSec, double decaySec, double sustainLevel, double releaseSec)
{
    switch (env.stage)
    {
    case ADSRStage::Off: //無音
        env.level = 0.0;
        return env.level;

    case ADSRStage::Attack: //0 -> 1 へ立ち上げ
        if (attackSec <= 0.0)
        {
            env.stage       = ADSRStage::Decay;
            env.level       = 1.0;
            env.timeInStage = 0.0;
            return env.level;
        }
        env.level        = env.timeInStage / attackSec;
        env.timeInStage += deltaTimeSec;
        if (env.level >= 1.0)
        {
            env.stage       = ADSRStage::Decay;
            env.level       = 1.0;
            env.timeInStage = 0.0;
        }
        return env.level;

    case ADSRStage::Decay: //1 -> sustainLevel へ移行
        if (decaySec <= 0.0)
        {
            env.stage       = ADSRStage::Sustain;
            env.level       = sustainLevel;
            env.timeInStage = 0.0;
            return env.level;
        }
        env.timeInStage += deltaTimeSec;
        {
            double k  = env.timeInStage / decaySec;
            env.level = 1.0 + (sustainLevel - 1.0) * k;
        }
        if (env.timeInStage >= decaySec)
        {
            env.stage       = ADSRStage::Sustain;
            env.level       = sustainLevel;
            env.timeInStage = 0.0;
        }
        return env.level;

    case ADSRStage::Sustain: //一定値を維持
        env.level = sustainLevel;
        return env.level;

    case ADSRStage::Release: //現在値 -> 0 へ減衰
        if (releaseSec <= 0.0)
        {
            env.stage       = ADSRStage::Off;
            env.level       = 0.0;
            env.timeInStage = 0.0;
            return env.level;
        }
        env.timeInStage += deltaTimeSec;
        {
            double k  = env.timeInStage / releaseSec;
            env.level = env.releaseStartLevel * (1.0 - k);
        }
        if (env.timeInStage >= releaseSec)
        {
            env.stage       = ADSRStage::Off;
            env.level       = 0.0;
            env.timeInStage = 0.0;
        }
        return env.level;

    default:
        env.stage       = ADSRStage::Off;
        env.level       = 0.0;
        env.timeInStage = 0.0;
        return env.level;
    }
}

void NoteOn(ADSRState& env)
{
    env.stage       = ADSRStage::Attack;
    env.timeInStage = 0.0;
}

void NoteOff(ADSRState& env)
{
    env.stage             = ADSRStage::Release;
    env.releaseStartLevel = env.level;
    env.timeInStage       = 0.0;
}
