#pragma once

#include "SynthEngine/WaveformConfig.h"

// アナログ模倣発振方式の設定集合。
// WaveformConfig と同等のオシレータ基盤に、オシレータドリフトを加えた方式。
// smoothing は waveform と同一契約で適用する。
struct AnalogConfig
{
    using ArpeggioConfig = WaveformConfig::ArpeggioConfig;

    struct SmoothingConfig
    {
        bool enabled = true;
        bool pitchEnabled = false;
        double ampTimeMs = 4.0;
        double pitchTimeMs = 2.0;
        double filterCutoffTimeMs = 8.0;
    };

    WaveType wave;
    int unisonVoices = 1;
    double unisonDetuneCents = 0.0;
    double unisonSpread = 0.0;
    double subOscLevel = 0.0;
    double pulseWidth = 0.5;
    bool hardSyncEnabled = false;
    double hardSyncRatio = 2.0;
    bool ringModEnabled = false;
    double ringModRatio = 2.0;
    double ringModMix = 1.0;
    FilterMode filterMode = FilterMode::Bypass;
    double filterCutoffHz = 8000.0;
    double filterResonance = 0.707;
    double filterDrive = 0.0;
    double filterKeytrack = 0.0;
    double drive = 0.0;
    // アナログ固有: ボイスごとのピッチドリフト。
    double driftDepthCents = 0.0;
    double driftRateHz = 0.3;
    ArpeggioConfig arpeggio{};
    SmoothingConfig smoothing{};
    ModulationConfig modulation{};
};
