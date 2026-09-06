#pragma once

#include "SynthEngine/Filter.h"
#include "synth/Oscillator.h"

// ノイズ発振方式の最小設定。
struct NoiseConfig
{
    bool operator==(const NoiseConfig&) const = default;
    // ノイズ音源種別。
    // smoothing は非対応（契約上 waveform 専用）。
    NoiseType noise;
    // 共通Filterレイヤー（Waveform/Fm/Analog と同一契約）。
    FilterMode filterMode = FilterMode::Bypass;
    double filterCutoffHz = 8000.0;
    double filterResonance = 0.707;
    double filterDrive = 0.0;
};
