#pragma once

struct SmoothingRange
{
    double minValue = -1.0e300;
    double maxValue = 1.0e300;
};

struct SmoothedParam
{
    double current = 0.0;
    double target = 0.0;
    double alpha = 1.0;
    double timeMs = 0.0;
    int sampleRate = 44100;
    SmoothingRange range{};
};

void SetSmoothingRange(SmoothedParam& param, double minValue, double maxValue);
void SetSmoothingSampleRate(SmoothedParam& param, int sampleRate);
void SetSmoothingTimeMs(SmoothedParam& param, double timeMs);
void ResetSmoothedParam(SmoothedParam& param, double value);
void SetSmoothedTarget(SmoothedParam& param, double target);
double StepSmoothedParam(SmoothedParam& param);
bool IsSmoothingBypassed(const SmoothedParam& param);

