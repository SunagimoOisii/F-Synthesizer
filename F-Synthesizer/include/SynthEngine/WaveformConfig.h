#pragma once

#include <array>

#include "SynthEngine/Filter.h"
#include "SynthEngine/Modulation.h"
#include "synth/Oscillator.h"

// 波形発振方式の設定集合。
// ConfigLoad / GUI編集 / Renderer の共通入力として使う。
struct WaveformConfig
{
    bool operator==(const WaveformConfig&) const = default;
    struct ArpeggioConfig
    {
        bool operator==(const ArpeggioConfig&) const = default;
        bool enabled = false;
        double rateHz = 8.0;
        int steps = 1;
        std::array<int, 8> semitones{};
    };

    struct SmoothingConfig
    {
        bool operator==(const SmoothingConfig&) const = default;
        bool enabled = true;
        bool pitchEnabled = false;
        double ampTimeMs = 4.0;
        double pitchTimeMs = 2.0;
        double filterCutoffTimeMs = 8.0;
    };

    // 位相サンプルで生成する基本波形。
    WaveType wave;
    // Unison voice 数（1 で無効）。
    int unisonVoices = 1;
    // Unison の最大デチューン幅（cent）。
    double unisonDetuneCents = 0.0;
    // Unison ボイス間の位相拡散量（0..1、モノラルでは位相分散として扱う）。
    double unisonSpread = 0.0;
    // Sub-osc (-1 octave) の混合量。
    double subOscLevel = 0.0;
    // Square 波のパルス幅（0.05..0.95）。
    double pulseWidth = 0.5;
    bool hardSyncEnabled = false;
    double hardSyncRatio = 2.0;
    bool ringModEnabled = false;
    double ringModRatio = 2.0;
    double ringModMix = 1.0;
    // 共通Filterレイヤー（Waveform向け初期接続）。
    FilterMode filterMode = FilterMode::Bypass;
    double filterCutoffHz = 8000.0;
    double filterResonance = 0.707;
    double filterDrive = 0.0;
    // キートラック量（0.0=なし, 1.0=フルトラック）。基準ノートは C4(60)。
    double filterKeytrack = 0.0;
    // ウェーブシェーパーのドライブ量（0.0=バイパス, 1.0=最大クリップ）。
    double drive = 0.0;
    ArpeggioConfig arpeggio{};
    // 共通Smoothingレイヤー（amp/pitch/filterCutoff）。
    SmoothingConfig smoothing{};
    // 共通Modulationレイヤー（LFO/Env2/Matrix）。
    ModulationConfig modulation{};
};
