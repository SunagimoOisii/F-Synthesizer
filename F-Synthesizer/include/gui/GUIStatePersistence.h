#pragma once

#include <filesystem>
#include <string>

#include "gui/GUIState.h"

namespace gui
{
std::filesystem::path GUIStatePath();
// 音色、割当、ノート、画面状態を一つの workspace.json から復元する。
// 読み込み失敗時は編集中の状態を変更しない。
bool LoadGUIStateFile(GUIState& state, std::string& err);
// プリセット原本には書き込まない。
bool SaveGUIStateFile(const GUIState& state, std::string& err);
} // namespace gui
