#pragma once

#include <array>
#include <functional>
#include <vector>

#include "core/AudioBuffer.h"
#include "core/RenderConfig.h"

// app 層から core(SynthEngine) への実行境界。
// 呼び出し点を固定して依存方向を保つ。
// 目的: render用に解決済みの入力を受け取り、SynthEngine で SoundData を更新する。
// 副作用: sound を上書きし、キャンセル成立時は canceled(true) を返す。
void RenderWithEngine(
    SoundData& sound,
    const RenderConfig& config,
    const std::function<bool()>& shouldCancel,
    bool* canceled);

void RenderWithEngineFrames(
    int length,
    int sampleRate,
    const RenderConfig& config,
    const std::function<bool(int, double, double)>& onFrame,
    const std::function<bool()>& shouldCancel,
    bool* canceled);
void RenderWithEngineFrameBlocks(
    int length,
    int sampleRate,
    const RenderConfig& config,
    const std::function<bool(int, const double*, int)>& onFrames,
    const std::function<bool()>& shouldCancel,
    bool* canceled);
