#pragma once

#include "SynthEngine/InstrumentSoundConfig.h"
#include "gui/GUIMacroSliders.h"

// スライダー値 -> パラメータ書き込み。
// sliders の brightness/roughness/movement/envelope (0..1) を ch のソース設定へ反映する。
void ApplyMacroSliders(InstrumentSoundConfig& ch, const MacroSliderState& sliders);

// パラメータ -> スライダー逆算。
// 単一パラメータスライダーのみ逆算して返す。
// マルチパラメータスライダー（γポリシー）は current の lastLayer2* 値をそのまま返す。
MacroSliderState ReadMacroSliders(const InstrumentSoundConfig& ch, const MacroSliderState& current);
