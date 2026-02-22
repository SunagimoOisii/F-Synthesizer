#pragma once

#include <filesystem>
#include <string>

#include "gui/GUIState.h"

namespace gui
{
void AppendGUILog(GUIState& state, const std::string& line);
void RefreshPresetItems(GUIState& state, const std::string& preferName);
bool ApplySelectedPresetPaths(GUIState& state, std::string& err);
bool SavePresetDiffFromState(const GUIState& state, const std::filesystem::path& presetPath, std::string& err);
void AnalyzeRenderPeakFromLogs(GUIState& state);
void ActivateSoloPreview(GUIState& state, int channel);
void DeactivateSoloPreview(GUIState& state);
void RaiseGUIError(GUIState& state, const std::string& message, int actionHint, bool showDialog);
void ClearGUIError(GUIState& state);
void StartGUIRun(GUIState& state, bool previewSelected);
void StartGUISoundTonePreview(GUIState& state);
void StopGUIRunAndPreview(GUIState& state);
bool TryFinalizeCompletedRun(GUIState& state);
} // namespace gui
