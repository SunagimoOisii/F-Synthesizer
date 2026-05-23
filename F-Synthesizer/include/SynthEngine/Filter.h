#pragma once

enum class FilterMode
{
    Bypass,
    LowPass,
    HighPass,
    BandPass,
    LadderLowPass
};

struct FilterParams
{
    FilterMode mode = FilterMode::Bypass;
    double cutoffHz = 1200.0;
    double resonance = 0.707; // Q
    double drive = 0.0;
};

struct BiquadCoefficients
{
    double b0 = 1.0;
    double b1 = 0.0;
    double b2 = 0.0;
    double a1 = 0.0;
    double a2 = 0.0;
};

struct BiquadState
{
    double z1 = 0.0;
    double z2 = 0.0;
};

struct FilterInstance
{
    FilterParams params{};
    int sampleRate = 44100;
    BiquadCoefficients coeffs{};
    BiquadState state{};
    double ladderStage[4]{};
    bool dirty = true;
};

FilterParams ClampFilterParams(const FilterParams& params, int sampleRate);
void SetFilterSampleRate(FilterInstance& filter, int sampleRate);
void SetFilterMode(FilterInstance& filter, FilterMode mode);
void SetFilterCutoffHz(FilterInstance& filter, double cutoffHz);
void SetFilterResonance(FilterInstance& filter, double resonance);
void SetFilterDrive(FilterInstance& filter, double drive);
void ResetFilterState(FilterInstance& filter);
void UpdateFilterCoefficients(FilterInstance& filter);
double ProcessFilterSample(FilterInstance& filter, double input);
