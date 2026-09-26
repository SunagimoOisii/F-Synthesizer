#pragma once
#include "StudioWidgets.h"
#include <string>
struct GUIState;
struct InstrumentConfig;
namespace studio
{
inline constexpr const char *categories[] = {"Lead", "Bass", "Keys", "Pad", "Guitar", "Drums", "SFX", "Support"};
inline constexpr const char *categoryLabels[] = {"リード", "ベース", "鍵盤",   "パッド",
                                                 "ギター", "ドラム", "効果音", "電子音"};
inline constexpr Icon categoryGlyphs[] = {MusicIcon,  GuitarIcon, PianoIcon,    LayersIcon,
                                          GuitarIcon, DrumIcon,   SparklesIcon, ChipIcon};
int categoryIndex(const std::string &name);
std::string toneName(const InstrumentConfig &instrument);
// 波形・通常ノブ・追加調整を配置し、試聴・採用操作を置くY座標を返す。
float toneEditor(GUIState &state, float x, float y, float w, float controlsW, float availableH);
// 試聴設定・比較・取り消し・採用を一つの操作領域として扱う。
void toneActions(GUIState &state, float x, float y, float w);
void channelStrip(GUIState &state, float x, float w);
void mixControls(GUIState &state, float x, float y);
void presetList(GUIState &state, float x, float y, float w, float h);
} // namespace studio
