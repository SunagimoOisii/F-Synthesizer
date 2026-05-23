#pragma once

#include "SynthEngine/Filter.h"
#include "SynthEngine/Modulation.h"
#include "synth/Oscillator.h"

// FM オペレータ1個分の設定。
struct FmOperator
{
    WaveType wave = WaveType::Sine;
    // 基音に対する周波数比（0.0 < ratio <= 32.0）。
    double ratio = 1.0;
    // 出力レベル（0.0..1.0）。キャリアは音量、モジュレータは変調量への寄与。
    double level = 1.0;
    // 変調深さ（0.0..32.0）。モジュレータとして使われるときのみ有効。
    double index = 0.0;
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
    // 接続アルゴリズム（0-7）。
    //   0: ops[0]→ops[1]（1変調→1キャリア。ops[2]/ops[3] は無視）
    //   1: [ops[0]→ops[1]] + [ops[2]→ops[3]]（2ペア並列）
    //   2: ops[0]→[ops[1]+ops[2]+ops[3]]（1変調→3キャリア）
    //   3: ops[0]→ops[1]→ops[2]→ops[3]（チェーン）
    //   4: [ops[0]→ops[1]] + [ops[2]→ops[3]]（2ペア並列キャリア）
    //   5: ops[0]→[ops[1]+ops[2]+ops[3]]（3並列キャリア）
    //   6: [ops[0]→ops[1]] + ops[2] + ops[3]（1ペア + 2独立）
    //   7: ops[0] + ops[1] + ops[2] + ops[3]（全独立キャリア）
    int algorithm = 0;
    // ops[0] の自己フィードバック量（0.0=なし, 1.0=最大）。
    double feedback = 0.0;
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
