#include <string>
#include <cstring>
#include <vector>
#include <memory>
#include <atomic>
#include <mutex>
#include <future>
#include <chrono>
#include <fstream>
#include <sstream>
#include <regex>
#include <optional>
#include <ctime>
#include <algorithm>
#include <cmath>
#include <cctype>
#include <type_traits>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "AppCore.h"
#include "gui/GUIChannelEditor.h"
#include "gui/GUIConfigUtils.h"
#include "gui/GUIPlatform.h"
#include "gui/GUIPresetIO.h"
#include "gui/GUIRunHelpers.h"
#include "gui/GUIState.h"
#include "gui/GUIStateModel.h"
#include "gui/GUIStateStorage.h"
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
float UiScaleFromIndex(int idx);
const char* UiScaleLabelFromIndex(int idx);
void DrawStatusBadge(const GUIState& state);
using gui::WaveToIndex;
using gui::BuildPreviewWavPath;
using gui::BuildSerialWavPath;
using gui::BuildConfigFromGUI;
using gui::CollectPresetItems;
using gui::DrawChannelEditor;
using gui::EnsureChannelConfigs;
using gui::EnsureChannelMixStates;
using gui::InitializeGUIState;
using gui::RepairGUIStatePaths;

void SetupImGuiFont()
{
    ImGuiIO& io = ImGui::GetIO();
    const ImWchar* ranges = io.Fonts->GetGlyphRangesJapanese();
    const char* candidates[] = {
        "C:\\Windows\\Fonts\\meiryo.ttc",
        "C:\\Windows\\Fonts\\YuGothM.ttc",
        "C:\\Windows\\Fonts\\msgothic.ttc"
    };

    for (const char* fontPath : candidates)
    {
        std::error_code ec;
        if (!std::filesystem::exists(fontPath, ec))
        {
            continue;
        }
        ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath, 20.0f, nullptr, ranges);
        if (font != nullptr)
        {
            io.FontDefault = font;
            return;
        }
    }
}

void AppendGUILog(GUIState& state, const std::string& line)
{
    std::lock_guard<std::mutex> lock(state.logMutex);
    state.logs.push_back(line);
}

bool SavePresetDiff(const GUIState& state, const std::filesystem::path& presetPath, std::string& err)
{
    if (!state.channelConfigs || !state.channelMixStates)
    {
        err = "channel configs or mix states are not initialized";
        return false;
    }
    gui::GUIPresetSnapshot snapshot{};
    snapshot.midiPathUtf8 = state.midiPath;
    snapshot.wavPathUtf8 = state.wavPath;
    snapshot.channelConfigs = *state.channelConfigs;
    snapshot.channelMixStates = *state.channelMixStates;
    return gui::SavePresetDiffFile(snapshot, presetPath, err);
}

int FindPresetIndex(const GUIState& state, const std::string& name)
{
    for (int i = 0; i < static_cast<int>(state.presetItems.size()); i++)
    {
        if (state.presetItems[i] == name)
        {
            return i;
        }
    }
    return -1;
}

void RefreshPresetItems(GUIState& state, const std::string& preferName)
{
    state.presetItems = CollectPresetItems(FindProjectRootPath());
    if (state.presetItems.empty())
    {
        state.presetItems.push_back("basic_wave");
    }

    int idx = FindPresetIndex(state, preferName);
    if (idx < 0)
    {
        idx = FindPresetIndex(state, "basic_wave");
    }
    state.presetIndex = (idx >= 0) ? idx : 0;
}

bool ApplySelectedPresetPaths(GUIState& state, std::string& err)
{
    if (state.presetItems.empty())
    {
        err = "preset list is empty";
        return false;
    }
    if (state.presetIndex < 0 || state.presetIndex >= static_cast<int>(state.presetItems.size()))
    {
        err = "invalid preset index";
        return false;
    }

    const std::string& presetName = state.presetItems[state.presetIndex];
    strncpy_s(state.presetName, sizeof(state.presetName), presetName.c_str(), _TRUNCATE);
    AppConfig cfg{};
    if (!gui::LoadPresetConfig(FindProjectRootPath(), presetName, cfg, err))
    {
        return false;
    }

    CopyPath(state.midiPath, sizeof(state.midiPath), cfg.midiPath);
    CopyPath(state.wavPath, sizeof(state.wavPath), cfg.wavPath);
    state.targetChannel = cfg.targetChannel;
    state.sampleRate = cfg.sampleRate;
    state.initialSeconds = cfg.initialSeconds;
    state.bits = cfg.bits;
    state.extraReleaseSec = static_cast<float>(cfg.extraReleaseSec);
    state.defaultWave = WaveToIndex(cfg.defaultWave);

    EnsureChannelConfigs(state);
    EnsureChannelMixStates(state);
    if (cfg.channelConfigs)
    {
        *state.channelConfigs = *cfg.channelConfigs;
    }
    if (cfg.channelMixStates)
    {
        *state.channelMixStates = *cfg.channelMixStates;
    }
    return true;
}

std::filesystem::path GUIStatePath()
{
    return FindProjectRootPath() / "config" / "gui_state.json";
}

GUIStateStorageData BuildStateStorageData(const GUIState& state)
{
    GUIStateStorageData data{};
    data.midiPath = state.midiPath;
    data.wavPath = state.wavPath;
    data.targetChannel = state.targetChannel;
    data.sampleRate = state.sampleRate;
    data.initialSeconds = state.initialSeconds;
    data.bits = state.bits;
    data.extraReleaseSec = state.extraReleaseSec;
    data.defaultWave = state.defaultWave;
    data.uiScaleIndex = state.uiScaleIndex;
    data.logPanelHeight = state.logPanelHeight;
    data.presetIndex = state.presetIndex;
    data.serialSave = state.serialSave;
    data.previewLoop = state.previewLoop;
    data.selectedChannel = state.selectedChannel;
    data.selectedDrumNote = state.selectedDrumNote;
    data.presetName = state.presetName;
    data.lastPresetPath = state.lastPresetPath;
    for (int ch = 0; ch < 16; ch++)
    {
        data.channelMixStates[ch] = (state.channelMixStates != nullptr)
            ? (*state.channelMixStates)[ch]
            : ChannelMixState{};
    }
    return data;
}

void ApplyStateStorageData(GUIState& state, const GUIStateStorageData& data)
{
    strncpy_s(state.midiPath, sizeof(state.midiPath), data.midiPath.c_str(), _TRUNCATE);
    strncpy_s(state.wavPath, sizeof(state.wavPath), data.wavPath.c_str(), _TRUNCATE);
    state.targetChannel = data.targetChannel;
    state.sampleRate = data.sampleRate;
    state.initialSeconds = data.initialSeconds;
    state.bits = data.bits;
    state.extraReleaseSec = data.extraReleaseSec;
    state.defaultWave = data.defaultWave;
    state.uiScaleIndex = data.uiScaleIndex;
    state.logPanelHeight = data.logPanelHeight;
    state.presetIndex = data.presetIndex;
    state.serialSave = data.serialSave;
    state.previewLoop = data.previewLoop;
    state.selectedChannel = data.selectedChannel;
    state.selectedDrumNote = data.selectedDrumNote;
    strncpy_s(state.presetName, sizeof(state.presetName), data.presetName.c_str(), _TRUNCATE);
    state.lastPresetPath = data.lastPresetPath;

    EnsureChannelMixStates(state);
    for (int ch = 0; ch < 16; ch++)
    {
        (*state.channelMixStates)[ch] = data.channelMixStates[ch];
    }
}

bool LoadGUIStateFile(GUIState& state, std::string& err)
{
    GUIStateStorageData data = BuildStateStorageData(state);
    if (!LoadGUIStateStorageFile(GUIStatePath(), data, err))
    {
        return false;
    }
    ApplyStateStorageData(state, data);
    return true;
}

bool SaveGUIStateFile(const GUIState& state, std::string& err)
{
    const GUIStateStorageData data = BuildStateStorageData(state);
    return SaveGUIStateStorageFile(GUIStatePath(), data, err);
}

bool ValidateBeforeRun(const GUIState& state, std::string& err)
{
    return gui::ValidateRunSettings(
        state.midiPath,
        state.wavPath,
        state.targetChannel,
        state.sampleRate,
        state.initialSeconds,
        state.bits,
        err);
}

void AnalyzeRenderPeakFromLogs(GUIState& state)
{
    std::lock_guard<std::mutex> lock(state.logMutex);
    for (auto it = state.logs.rbegin(); it != state.logs.rend(); ++it)
    {
        const std::string& line = *it;
        const std::string key = "[RenderStats] peak=";
        const size_t pos = line.find(key);
        if (pos == std::string::npos)
        {
            continue;
        }
        const size_t start = pos + key.size();
        size_t end = start;
        while (end < line.size() && (std::isdigit(static_cast<unsigned char>(line[end])) || line[end] == '.' || line[end] == '-'))
        {
            end++;
        }
        if (end <= start)
        {
            break;
        }
        try
        {
            state.lastPeak = std::stod(line.substr(start, end - start));
            state.hasPeak = true;
        }
        catch (...)
        {
            state.hasPeak = false;
        }
        return;
    }
}

void ActivateSoloPreview(GUIState& state, int channel)
{
    EnsureChannelMixStates(state);
    channel = std::clamp(channel, 0, 15);
    if (!state.soloPreviewActive)
    {
        state.soloPreviewBackup = *state.channelMixStates;
    }
    for (int ch = 0; ch < 16; ch++)
    {
        ChannelMixState& mix = (*state.channelMixStates)[ch];
        mix.solo = (ch == channel);
        if (ch == channel)
        {
            mix.mute = false;
        }
    }
    state.soloPreviewChannel = channel;
    state.soloPreviewActive = true;
    AppendGUILog(state, "[GUI] Solo Preview ON: ch" + std::to_string(channel));
}

void DeactivateSoloPreview(GUIState& state)
{
    if (!state.soloPreviewActive || !state.channelMixStates)
    {
        return;
    }
    *state.channelMixStates = state.soloPreviewBackup;
    AppendGUILog(state, "[GUI] Solo Preview OFF: restore previous mix state");
    state.soloPreviewActive = false;
    state.restorePreviewOnRunComplete = false;
}

float UiScaleFromIndex(int idx)
{
    switch (idx)
    {
    case 0: return 1.0f;
    case 1: return 1.25f;
    case 2: return 1.5f;
    default: return 1.25f;
    }
}

const char* UiScaleLabelFromIndex(int idx)
{
    switch (idx)
    {
    case 0: return "100%";
    case 1: return "125%";
    case 2: return "150%";
    default: return "125%";
    }
}

void DrawStatusBadge(const GUIState& state)
{
    ImVec4 color = ImVec4(0.55f, 0.55f, 0.55f, 1.0f);
    const char* label = "Idle";

    if (state.running)
    {
        color = ImVec4(0.95f, 0.78f, 0.2f, 1.0f);
        label = "Running";
    }
    else if (state.playback.playing.load(std::memory_order_relaxed))
    {
        color = ImVec4(0.28f, 0.82f, 0.95f, 1.0f);
        label = state.previewLoop ? "Preview (Loop)" : "Preview";
    }
    else if (state.hasRun && state.lastRunExitCode == 2)
    {
        color = ImVec4(0.95f, 0.70f, 0.25f, 1.0f);
        label = "Canceled";
    }
    else if (state.hasRun && state.lastRunExitCode == 0)
    {
        color = ImVec4(0.30f, 0.82f, 0.40f, 1.0f);
        label = "Success";
    }
    else if (state.hasRun)
    {
        color = ImVec4(0.95f, 0.35f, 0.35f, 1.0f);
        label = "Failed";
    }

    ImGui::TextUnformatted("Status:");
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
}
} // namespace

int RunGUIApp()
{
    if (!glfwInit())
    {
        return 1;
    }

    const char* glslVersion = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "F-Synthesizer GUI (Preview)", nullptr, nullptr);
    if (window == nullptr)
    {
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    SetupImGuiFont();
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glslVersion);
    GUIState state{};
    InitializeGUIState(state, [&](const std::string& preferName) { RefreshPresetItems(state, preferName); });

    {
        std::string err;
        if (!LoadGUIStateFile(state, err))
        {
            AppendGUILog(state, "[GUI] gui_state load failed: " + err);
        }
        else
        {
            AppendGUILog(state, "[GUI] gui_state loaded: " + GUIStatePath().string());
        }
        RepairGUIStatePaths(
            state,
            [&](const std::string& preferName) { RefreshPresetItems(state, preferName); },
            [&](const std::string& line) { AppendGUILog(state, line); });
    }

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        if (state.running &&
            state.runFuture.valid() &&
            state.runFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready)
        {
            state.lastRunExitCode = state.runFuture.get();
            state.hasRun = true;
            state.running = false;
            AppendGUILog(state, std::string("[GUI] Run finished: exit=") + std::to_string(state.lastRunExitCode));
            if (state.runIsPreview)
            {
                if (state.lastRunExitCode == 0 &&
                    state.runOutputBuffer != nullptr &&
                    !state.runOutputBuffer->data.empty())
                {
                    state.previewRenderedSound = state.runOutputBuffer;
                    state.previewAudioReady = true;
                    AppendGUILog(state, "[GUI] Preview audio buffer ready");
                    if (state.autoPlayPreviewOnRunComplete)
                    {
                        std::string playErr;
                        if (PlayPreviewAudio(state.playback, *state.previewRenderedSound, state.previewLoop, playErr))
                        {
                            AppendGUILog(state, "[GUI] Preview playback started");
                        }
                        else
                        {
                            AppendGUILog(state, "[GUI] Preview playback failed: " + playErr);
                        }
                    }
                }
                else
                {
                    state.previewAudioReady = false;
                    state.previewRenderedSound.reset();
                }
                state.runOutputBuffer.reset();
                state.runIsPreview = false;
                state.autoPlayPreviewOnRunComplete = false;
            }
            if (state.restorePreviewOnRunComplete)
            {
                DeactivateSoloPreview(state);
            }
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::GetIO().FontGlobalScale = UiScaleFromIndex(state.uiScaleIndex);

        ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        const ImGuiWindowFlags rootFlags =
            ImGuiWindowFlags_NoTitleBar |
            ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoSavedSettings;
        ImGui::Begin("F-SynthesizerRoot", nullptr, rootFlags);

        ImGui::TextUnformatted("F-Synthesizer GUI");
        ImGui::Separator();
        DrawStatusBadge(state);
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 16.0f);
        ImGui::TextUnformatted("UI Scale");
        ImGui::SameLine();
        const char* uiScales[] = { "100%", "125%", "150%" };
        if (ImGui::Combo("##ui_scale", &state.uiScaleIndex, uiScales, IM_ARRAYSIZE(uiScales)))
        {
            AppendGUILog(state, std::string("[GUI] UI scale changed: ") + UiScaleLabelFromIndex(state.uiScaleIndex));
        }
        ImGui::Separator();
        auto startRun = [&](bool previewSelected)
        {
            std::string validationError;
            if (!ValidateBeforeRun(state, validationError))
            {
                state.hasRun = true;
                state.lastRunExitCode = 1;
                AppendGUILog(state, "[GUI] Validation failed: " + validationError);
            }
            else
            {
                if (previewSelected)
                {
                    ActivateSoloPreview(state, state.selectedChannel);
                }
                if (state.playback.playing.load(std::memory_order_relaxed))
                {
                    StopPreviewAudio(state.playback);
                    AppendGUILog(state, "[GUI] Previous preview playback stopped for new run");
                }
                AppConfig cfg = BuildConfigFromGUI(state);
                RenderOptions options = previewSelected ? DefaultPreviewRenderOptions() : DefaultRenderOptions();
                if (!previewSelected && state.serialSave)
                {
                    cfg.wavPath = BuildSerialWavPath(cfg.wavPath);
                }
                if (previewSelected)
                {
                    state.restorePreviewOnRunComplete = true;
                    options = DefaultPreviewRenderOptions();
                    options.writeWav = false;
                }
                state.lastOutputPath = previewSelected ? "[memory preview]" : PathToUtf8(cfg.wavPath);

                state.logs.clear();
                state.lastPeak = 0.0;
                state.hasPeak = false;
                state.runOutputBuffer = previewSelected ? std::make_shared<SoundData>() : nullptr;
                state.runIsPreview = previewSelected;
                state.autoPlayPreviewOnRunComplete = previewSelected;
                AppendGUILog(state, previewSelected ? "[GUI] Preview Play started" : "[GUI] Play started");
                AppendGUILog(state, "[GUI] Effective Output: " + state.lastOutputPath);
                state.hasRun = false;
                state.stopRequested.store(false, std::memory_order_relaxed);
                state.running = true;
                state.runFuture = std::async(std::launch::async, [cfg, options, outBuffer = state.runOutputBuffer, &state]() {
                    return Run(cfg, options, &state.observer, outBuffer.get());
                    });
            }
        };

        ImGui::BeginDisabled(state.running);
        if (ImGui::Button("Play"))
        {
            startRun(false);
        }
        ImGui::SameLine();
        if (ImGui::Button("Play Preview (Selected ch)"))
        {
            startRun(true);
        }
        ImGui::SameLine();
        ImGui::Checkbox("Loop Preview", &state.previewLoop);
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled(state.running || !state.previewAudioReady || !state.previewRenderedSound);
        if (ImGui::Button("Replay Preview"))
        {
            std::string err;
            if (PlayPreviewAudio(state.playback, *state.previewRenderedSound, state.previewLoop, err))
            {
                AppendGUILog(state, "[GUI] Preview replay started");
            }
            else
            {
                AppendGUILog(state, "[GUI] Preview replay failed: " + err);
            }
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        const bool canStop = state.running || state.playback.playing.load(std::memory_order_relaxed);
        ImGui::BeginDisabled(!canStop);
        if (ImGui::Button("Stop"))
        {
            if (state.playback.playing.load(std::memory_order_relaxed))
            {
                StopPreviewAudio(state.playback);
                AppendGUILog(state, "[GUI] Preview playback stopped");
            }
            if (state.running)
            {
                state.stopRequested.store(true, std::memory_order_relaxed);
                AppendGUILog(state, "[GUI] Stop requested (render cancellation signal sent)");
            }
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled(state.running);
        if (ImGui::Button("Close"))
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        ImGui::EndDisabled();
        ImGui::TextDisabled("Play: export full run / Play Preview: selected channel memory preview");
        ImGui::Separator();

        const float availY = ImGui::GetContentRegionAvail().y;
        const float reserveForLog = state.logPanelHeight + ImGui::GetFrameHeightWithSpacing() + 12.0f;
        const float bodyHeight = (std::max)(180.0f, availY - reserveForLog);
        ImGui::BeginChild("body_panel", ImVec2(0, bodyHeight), true);
        if (ImGui::BeginTable("layout_split", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("left", ImGuiTableColumnFlags_WidthStretch, 0.56f);
            ImGui::TableSetupColumn("right", ImGuiTableColumnFlags_WidthStretch, 0.44f);
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            if (state.presetDirty)
            {
                ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.2f, 1.0f), "Preset: modified (unsaved)");
            }
            else
            {
                ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.0f), "Preset: saved");
            }
            ImGui::BeginDisabled(state.running);
            auto presetGetter = [](void* data, int idx, const char** outText) -> bool
            {
                auto* items = static_cast<std::vector<std::string>*>(data);
                if (items == nullptr || idx < 0 || idx >= static_cast<int>(items->size()))
                {
                    return false;
                }
                *outText = (*items)[idx].c_str();
                return true;
            };
            if (ImGui::Combo("Preset", &state.presetIndex, presetGetter, &state.presetItems, static_cast<int>(state.presetItems.size())))
            {
                std::string err;
                if (ApplySelectedPresetPaths(state, err))
                {
                    state.presetDirty = true;
                }
                else
                {
                    AppendGUILog(state, "[GUI] Apply preset failed: " + err);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Apply Preset Paths"))
            {
                std::string err;
                if (ApplySelectedPresetPaths(state, err))
                {
                    state.presetDirty = true;
                }
                else
                {
                    AppendGUILog(state, "[GUI] Apply preset failed: " + err);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset Defaults"))
            {
                InitializeGUIState(state, [&](const std::string& preferName) { RefreshPresetItems(state, preferName); });
                state.presetDirty = false;
            }

            ImGui::InputText("Preset Name", state.presetName, IM_ARRAYSIZE(state.presetName));
            ImGui::SameLine();
            if (ImGui::Button("Save Preset As"))
            {
                const std::filesystem::path p = FindProjectRootPath() / "config" / "presets" / (std::string(state.presetName) + ".json");
                std::string err;
                if (SavePresetDiff(state, p, err))
                {
                    state.lastPresetPath = PathToUtf8(p);
                    state.presetDirty = false;
                    RefreshPresetItems(state, state.presetName);
                    AppendGUILog(state, "[GUI] Preset saved: " + state.lastPresetPath);
                }
                else
                {
                    AppendGUILog(state, "[GUI] Preset save failed: " + err);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Duplicate Preset"))
            {
                std::string copyName = std::string(state.presetName) + "_copy";
                strncpy_s(state.presetName, sizeof(state.presetName), copyName.c_str(), _TRUNCATE);
                const std::filesystem::path p = FindProjectRootPath() / "config" / "presets" / (std::string(state.presetName) + ".json");
                std::string err;
                if (SavePresetDiff(state, p, err))
                {
                    state.lastPresetPath = PathToUtf8(p);
                    state.presetDirty = false;
                    RefreshPresetItems(state, state.presetName);
                    AppendGUILog(state, "[GUI] Preset duplicated: " + state.lastPresetPath);
                }
                else
                {
                    AppendGUILog(state, "[GUI] Preset duplicate failed: " + err);
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Reset Channel"))
            {
                EnsureChannelConfigs(state);
                EnsureChannelMixStates(state);
                AppConfig def = DefaultConfig();
                if (def.channelConfigs)
                {
                    (*state.channelConfigs)[state.selectedChannel] = (*def.channelConfigs)[state.selectedChannel];
                    if (def.channelMixStates)
                    {
                        (*state.channelMixStates)[state.selectedChannel] = (*def.channelMixStates)[state.selectedChannel];
                    }
                    state.presetDirty = true;
                    AppendGUILog(state, "[GUI] Channel reset: ch" + std::to_string(state.selectedChannel));
                }
            }
            if (!state.lastPresetPath.empty())
            {
                ImGui::Text("Last Preset: %s", state.lastPresetPath.c_str());
            }

            state.presetDirty |= ImGui::InputText("MIDI Path", state.midiPath, IM_ARRAYSIZE(state.midiPath));
            ImGui::SameLine();
            if (ImGui::Button("Browse MIDI..."))
            {
                std::string selected;
                const wchar_t* midiFilter = L"MIDI Files (*.mid;*.midi)\0*.mid;*.midi\0All Files (*.*)\0*.*\0";
                if (BrowseOpenPath(state.midiPath, midiFilter, selected))
                {
                    strncpy_s(state.midiPath, sizeof(state.midiPath), selected.c_str(), _TRUNCATE);
                    state.presetDirty = true;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Copy MIDI"))
            {
                ImGui::SetClipboardText(state.midiPath);
            }
            {
                const std::string compact = CompactPathForUi(state.midiPath);
                ImGui::TextDisabled("%s", compact.c_str());
                if (ImGui::IsItemHovered() && std::strlen(state.midiPath) > 0)
                {
                    ImGui::SetTooltip("%s", state.midiPath);
                }
            }

            state.presetDirty |= ImGui::InputText("Output Path", state.wavPath, IM_ARRAYSIZE(state.wavPath));
            ImGui::SameLine();
            if (ImGui::Button("Browse Output..."))
            {
                std::string selected;
                const wchar_t* wavFilter = L"WAV Files (*.wav)\0*.wav\0All Files (*.*)\0*.*\0";
                if (BrowseSavePath(state.wavPath, wavFilter, L"wav", selected))
                {
                    strncpy_s(state.wavPath, sizeof(state.wavPath), selected.c_str(), _TRUNCATE);
                    state.presetDirty = true;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Copy Output"))
            {
                ImGui::SetClipboardText(state.wavPath);
            }
            {
                const std::string compact = CompactPathForUi(state.wavPath);
                ImGui::TextDisabled("%s", compact.c_str());
                if (ImGui::IsItemHovered() && std::strlen(state.wavPath) > 0)
                {
                    ImGui::SetTooltip("%s", state.wavPath);
                }
            }
            state.presetDirty |= ImGui::InputInt("Target Channel", &state.targetChannel);
            state.presetDirty |= ImGui::InputInt("Sample Rate", &state.sampleRate);
            state.presetDirty |= ImGui::InputInt("Initial Seconds", &state.initialSeconds);
            state.presetDirty |= ImGui::InputInt("Bits", &state.bits);
            state.presetDirty |= ImGui::InputFloat("Extra Release (sec)", &state.extraReleaseSec, 0.01f, 0.1f, "%.2f");
            const char* waves[] = { "sine", "square", "saw", "triangle" };
            state.presetDirty |= ImGui::Combo("Default Wave", &state.defaultWave, waves, IM_ARRAYSIZE(waves));
            state.presetDirty |= ImGui::Checkbox("Serial Save (timestamp suffix)", &state.serialSave);
            ImGui::EndDisabled();

            ImGui::TableSetColumnIndex(1);
            state.presetDirty |= DrawChannelEditor(state);
            if (!state.lastOutputPath.empty())
            {
                ImGui::Text("Last Output: %s", state.lastOutputPath.c_str());
            }
            AnalyzeRenderPeakFromLogs(state);
            if (state.hasPeak)
            {
                const float meter = static_cast<float>(std::clamp(state.lastPeak, 0.0, 1.0));
                ImGui::Text("Peak: %.4f", state.lastPeak);
                ImGui::ProgressBar(meter, ImVec2(-1, 0));
                if (state.lastPeak > 1.0)
                {
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.95f, 0.3f, 0.3f, 1.0f), "CLIP");
                }
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();

        ImGui::Separator();
        ImGui::SetNextItemWidth(260.0f);
        ImGui::SliderFloat("Log Height", &state.logPanelHeight, 140.0f, 520.0f, "%.0f");
        ImGui::Text("Logs");
        ImGui::BeginChild("log_panel", ImVec2(0, state.logPanelHeight), true);
        {
            std::lock_guard<std::mutex> lock(state.logMutex);
            for (const std::string& line : state.logs)
            {
                ImGui::TextUnformatted(line.c_str());
            }
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
            {
                ImGui::SetScrollHereY(1.0f);
            }
        }
        ImGui::EndChild();
        ImGui::End();

        ImGui::Render();
        int displayW = 0;
        int displayH = 0;
        glfwGetFramebufferSize(window, &displayW, &displayH);
        glViewport(0, 0, displayW, displayH);
        glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    if (state.running && state.runFuture.valid())
    {
        state.stopRequested.store(true, std::memory_order_relaxed);
        state.lastRunExitCode = state.runFuture.get();
        state.hasRun = true;
        state.running = false;
    }

    {
        std::string err;
        if (!SaveGUIStateFile(state, err))
        {
            AppendGUILog(state, "[GUI] gui_state save failed: " + err);
        }
    }

    ShutdownPreviewAudio(state.playback);

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

