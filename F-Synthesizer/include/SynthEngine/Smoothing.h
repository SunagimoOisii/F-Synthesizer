#pragma once

// 波形パラメータ平滑化の公開I/F。
// 呼び出し経路: GUI/Config値 -> Voices初期化/更新 -> Rendererの1sample処理でStep適用。
// 責務境界: 入力値の整形は上位層、平滑化の時定数反映と範囲制約はSynthEngine層で担当する。
struct SmoothingRange
{
    double minValue = -1.0e300;
    double maxValue = 1.0e300;
};

// 1つの連続値パラメータに対する平滑化状態。
// current/target/alpha は audio thread の更新系列で一貫して管理する。
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
// SetSmoothedTarget と異なり、current/target を即時に同値化して段差を消す。
void ResetSmoothedParam(SmoothedParam& param, double value);
// ResetSmoothedParam と異なり、target のみ更新して段階的に追従させる。
void SetSmoothedTarget(SmoothedParam& param, double target);
// 前提: 同一paramの並行更新はしない（audio thread 側の逐次更新を想定）。
double StepSmoothedParam(SmoothedParam& param);
bool IsSmoothingBypassed(const SmoothedParam& param);
