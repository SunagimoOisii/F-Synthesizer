#include <string>
#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <random>
#include <variant>

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

#include "main/TopBar.inl"
#include "main/Layer2Macros.inl"
#include "main/VirtualKeyboard.inl"
#include "main/VUMeter.inl"
#include "main/StepSequencer.inl"
#include "main/ExportView.inl"
#include "main/ExperienceViews.inl"
#include "main/MainWindow.inl"
} // namespace

#include "main/RunLoop.inl"
