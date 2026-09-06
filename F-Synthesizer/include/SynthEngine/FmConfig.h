#pragma once

#include "SynthEngine/Filter.h"
#include "SynthEngine/Modulation.h"
#include "synth/Oscillator.h"

// FM オペレータ1個分の設定。
struct FmOperator
{
    bool operator==(const FmOperator&) const = default;
    WaveType wave = WaveType::Sine;
    // 基音に対する周波数比（0.5 または整数 1..15）。
    double ratio = 1.0;
    // 出力レベル（0.0..1.0）。キャリアは音量、モジュレータは変調量への寄与。
    double level = 1.0;
    // 変調深さ（0.0..32.0）。モジュレータとして使われるときのみ有効。
    double index = 4.0;
    // オペレータ出力レベルに掛ける個別エンベロープ。
    ModEnvelopeConfig levelEnv{};
    // オペレータ変調深さに掛ける個別エンベロープ。
    ModEnvelopeConfig indexEnv{};
};

// FM 発振方式の設定集合（4オペレータ）。
// algorithm によってオペレータ間の接続トポロジが変わる。
// smoothing は非対応（契約上 waveform 専用）。
struct FmConfig
{
    bool operator==(const FmConfig&) const = default;
    // ymfm: 0 = YM2151 (X68000), 1 = YM2612 (Mega Drive).
    int chip = 0;
    // Hardware algorithm numbering, 0..7.
    int algorithm = 0;
    // ops[0] の自己フィードバック量（0.0=なし, 1.0=最大）。
    double feedback = 0.0;
    // A musical macro scaling all modulator levels around their preset values.
    double brightness = 0.5;
    FmOperator ops[4];
    FilterMode filterMode = FilterMode::Bypass;
    double filterCutoffHz = 8000.0;
    double filterResonance = 0.707;
    double filterDrive = 0.0;
    // ウェーブシェーパーのドライブ量（0.0=バイパス, 1.0=最大クリップ）。
    double drive = 0.0;
    // 共通 Modulation レイヤー（fm.index / pitchMul / amp をサポート）。
    ModulationConfig modulation{};
};
