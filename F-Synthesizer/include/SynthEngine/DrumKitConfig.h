#pragma once

#include <array>

#include "SynthEngine/DrumConfig.h"

// DrumKit 用の note(0..127) マップ。
// 各ノートは DrumConfig を持ち、None で未割り当てを表す。
struct DrumKitConfig
{
    // GM想定の note(0..127) -> DrumConfig マップ。
    std::array<DrumConfig, 128> map;
    DrumBusConfig drumBus{};
    double velocityCeiling = 1.0;
    double velocityCurve = 1.0;
};
