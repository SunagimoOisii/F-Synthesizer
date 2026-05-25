#pragma once

#include <array>

#include "SynthEngine/ChannelConfig.h"
#include "gui/GUIState.h"
#include "project/ProjectModel.h"

namespace gui
{
// 既存 GUIState の背後で ProjectModel へ接続する Facade。
// GUI 表示コードが保存形式や preset 差分仕様を直接知らないようにする正規入口。
// 画面コードの直接 state.channelConfigs 参照は Phase 5 で段階的にこの入口へ寄せる。
ProjectModel BuildProjectModelFromGUI(const GUIState& state);
void ApplyProjectModelToGUI(GUIState& state, const ProjectModel& model);

std::array<ChannelConfig, 16>& MutableChannelConfigs(GUIState& state);
std::array<ChannelMixState, 16>& MutableChannelMixStates(GUIState& state);
const std::array<ChannelConfig, 16>& ReadChannelConfigs(GUIState& state);
const std::array<ChannelMixState, 16>& ReadChannelMixStates(GUIState& state);
} // namespace gui
