#pragma once

#include <filesystem>
#include <string>

#include "gui/GUIState.h"

namespace gui
{
double SongSecondsAtTick(const GUIState& state, int tick);
std::vector<int> SongBarTicks(const GUIState& state);
int SongTickAtSeconds(const GUIState& state, double seconds);
bool SongIsPlaying(const GUIState& state);
void PauseSongPlayback(GUIState& state);
void RequestSongPlayback(GUIState& state);
void SeekSong(GUIState& state, int tick);
void RequestToneAudition(GUIState& state);
void UpdateGUITransport(GUIState& state);
void AppendGUILog(GUIState& state, const std::string& line);
void RefreshPresetItems(GUIState& state, const std::string& preferName);
bool ApplySelectedPresetPaths(GUIState& state, std::string& err);
bool SaveUserPresetFromState(GUIState& state, std::string& err);
void AnalyzeRenderPeakFromLogs(GUIState& state);
void ActivateSoloPreview(GUIState& state, int channel);
void DeactivateSoloPreview(GUIState& state);
void RaiseGUIError(GUIState& state, const std::string& message, int actionHint, bool showDialog);
void ClearGUIError(GUIState& state);
// 非同期Runを開始し、戻り値ではなく GUIState.runFuture / running へ状態を反映する。
void StartGUIRun(GUIState& state, bool previewSelected, bool selectedChannelOnly = false);
// Play/Advanced向けの単音プレビュー処理。midiPath を使わず runtime override note ticks を注入する。
void StartGUISoundTonePreview(GUIState& state);
// 実行中ならキャンセル要求を送る。完了待ちは行わない。
void StopGUIRunAndPreview(GUIState& state);
// 完了済み runFuture を取得し、ログ・preview再生・solo復元まで終えたら true を返す。
bool TryFinalizeCompletedRun(GUIState& state);
} // namespace gui
