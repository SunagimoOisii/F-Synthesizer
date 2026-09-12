#include <string>
#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <random>
#include <variant>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <objbase.h>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "AppCore.h"
#include "config/SourceRegistry.h"
#include "gui/GUIActions.h"
#include "gui/GUIChannelEditor.h"
#include "gui/GUIConfigUtils.h"
#include "gui/GUIMacroMapping.h"
#include "gui/GUIPianoRoll.h"
#include "gui/GUIPlatform.h"
#include "gui/GUIProjectFacade.h"
#include "gui/GUIPresetIO.h"
#include "config/ProjectJSON.h"
#include <fstream>
#include "gui/GUIState.h"
#include "gui/GUIStateModel.h"
#include "gui/GUIStatePersistence.h"
#include "gui/PreviewAudio.h"
#include "io/PlatformPaths.h"

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glfw3dll.lib")
#ifdef _DEBUG
#pragma comment(lib, "imguid.lib")
#else
#pragma comment(lib, "imgui.lib")
#endif

namespace
{
using gui::DrawChannelEditor;
using gui::InitializeGUIState;
using gui::RepairGUIStatePaths;
using gui::AppendGUILog;
using gui::RefreshPresetItems;
using gui::ApplySelectedPresetPaths;
using gui::SaveUserPresetFromState;
using gui::AnalyzeRenderPeakFromLogs;
using gui::StartGUIRun;
using gui::StartGUISoundTonePreview;
using gui::StopGUIRunAndPreview;
using gui::TryFinalizeCompletedRun;
using gui::RaiseGUIError;
using gui::ClearGUIError;
using gui::LoadGUIStateFile;
using gui::SaveGUIStateFile;
using gui::GUIStatePath;
using gui::DrawPianoRollPanel;
using ::PushSoundHistoryEntry;
using ::UndoSound;
using ::RedoSound;

#include "main/StepSequencer.inl"
#include "main/StudioWidgets.inl"
#include "main/StudioPanels.inl"
#include "main/StudioFiles.inl"
#include "main/StudioWindow.inl"
} // namespace

#include "main/RunLoop.inl"
