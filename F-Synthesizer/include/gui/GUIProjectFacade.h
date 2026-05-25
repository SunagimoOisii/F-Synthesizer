#pragma once

#include <array>

#include "SynthEngine/InstrumentSoundConfig.h"
#include "gui/GUIState.h"
#include "project/ProjectModel.h"

namespace gui
{
// 既存 GUIState の背後で ProjectModel へ接続する Facade。
// GUI 表示コードが保存形式や preset 差分仕様を直接知らないようにする正規入口。
// Play/Compose/preview/export の主要導線はこの入口経由にし、
// Advanced の詳細 editor も含め、新規の GUI 操作はこの入口経由にする。
ProjectModel BuildProjectModelFromGUI(const GUIState& state);
void ApplyProjectModelToGUI(GUIState& state, const ProjectModel& model);

std::array<InstrumentSoundConfig, 16>& MutableSoundSlots(GUIState& state);
std::array<ChannelMixState, 16>& MutableChannelMixStates(GUIState& state);
const std::array<InstrumentSoundConfig, 16>& ReadSoundSlots(GUIState& state);
const std::array<ChannelMixState, 16>& ReadChannelMixStates(GUIState& state);

const InstrumentSoundConfig& ReadSoundSlot(GUIState& state, int slot);
const InstrumentSoundConfig& ReadSoundSlot(const GUIState& state, int slot);
InstrumentSoundConfig& MutableSoundSlot(GUIState& state, int slot);
const ChannelMixState& ReadChannelMix(GUIState& state, int channel);
ChannelMixState& MutableChannelMix(GUIState& state, int channel);
int AssignedSoundSlot(const GUIState& state, int channel);
void SetChannelAssignment(GUIState& state, int channel, int slot);
MacroSliderState& MutableMacroSliders(GUIState& state, int slot);
const MacroSliderState& ReadMacroSliders(const GUIState& state, int slot);

ProjectModel BuildRuntimeProjectFromGUI(GUIState& state, const char* instrumentPrefix, bool applyChannelAssignments);
void OverrideProjectChannelWithSoundSlot(GUIState& state, int previewChannel, int soundSlot, ProjectModel& project);
} // namespace gui
