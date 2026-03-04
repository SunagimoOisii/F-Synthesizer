#pragma once

#include <filesystem>
#include <string>

#include "gui/GUIState.h"

namespace gui
{
std::filesystem::path GUIStatePath();
// GUI状態 + PianoRollプロジェクトを読込み、失敗時は err に原因を返す。
// 不足キーは呼び出し前 state の値を保持する。
bool LoadGUIStateFile(GUIState& state, std::string& err);
// GUI状態 + PianoRollプロジェクトを保存する。片方でも失敗した場合は false。
bool SaveGUIStateFile(const GUIState& state, std::string& err);
} // namespace gui
