#pragma once

#include <array>

#include "SynthEngine/ChannelConfig.h"
#include "gui/GUIState.h"
#include "project/ProjectModel.h"

namespace gui
{
// 既存 GUIState の背後で ProjectModel へ接続する Facade。
// 画面コードは段階的にこの入口へ寄せる。
ProjectModel BuildProjectModelFromGUI(const GUIState& state);
void ApplyProjectModelToGUI(GUIState& state, const ProjectModel& model);

std::array<ChannelConfig, 16>& MutableChannelConfigs(GUIState& state);
std::array<ChannelMixState, 16>& MutableChannelMixStates(GUIState& state);
const std::array<ChannelConfig, 16>& ReadChannelConfigs(GUIState& state);
const std::array<ChannelMixState, 16>& ReadChannelMixStates(GUIState& state);
} // namespace gui
