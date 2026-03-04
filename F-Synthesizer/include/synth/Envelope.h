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
    ADSRStage stage          = ADSRStage::Off;
    double level             = 0.0;
    double timeInStage       = 0.0;
    double releaseStartLevel = 0.0;
};

// 目的: 1サンプル分だけADSR状態を進め、現在レベルを返す。
// 前提: deltaTimeSec はサンプル周期（> 0）を想定。各秒指定が 0 以下なら即時遷移として扱う。
// 副作用: env.stage / level / timeInStage / releaseStartLevel を更新する。
double StepADSR(ADSRState& env, double deltaTimeSec,
    double attackSec, double decaySec, double sustainLevel, double releaseSec);
// NoteOn はアタック開始、NoteOff は現在レベルからリリース開始に使い分ける。
void NoteOn(ADSRState& env);
void NoteOff(ADSRState& env);
