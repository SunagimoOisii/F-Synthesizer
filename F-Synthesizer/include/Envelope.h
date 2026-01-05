#pragma once

enum class ADSRStage
{
    Off,
    Attack,
    Decay,
    Sustain,
    Release
};

struct ADSRState
{
    ADSRStage stage = ADSRStage::Off;
    double level = 0.0;
    double timeInStage = 0.0;
    double releaseStartLevel = 0.0;
};

double StepADSR(ADSRState& env, double deltaTimeSec, double attackSec, double decaySec, double sustainLevel, double releaseSec);
void NoteOn(ADSRState& env);
void NoteOff(ADSRState& env);
