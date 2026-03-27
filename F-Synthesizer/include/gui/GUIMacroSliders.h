#pragma once

// 1チャンネル分のマクロスライダー状態。
// brightness/roughness/movement/envelope は 0..1 の正規化値。
// lastLayer2* はγポリシー用: Layer2 で最後に操作した値。
// Layer3 個別編集後もこの値を保持し続ける。
struct MacroSliderState
{
    float brightness = 0.5f;
    float roughness = 0.0f;
    float movement = 0.0f;
    float envelope = 0.3f;

    // γポリシー用保持値（マルチパラメータスライダーのみ使用）
    float lastLayer2Roughness = 0.0f;
    float lastLayer2Envelope = 0.3f;
    // Analog の 揺れ は lfo1.depth + drift の複合なのでγ
    float lastLayer2Movement = 0.0f;
};
