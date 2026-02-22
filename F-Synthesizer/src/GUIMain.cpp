#include <string>
#include <algorithm>
#include <array>
#include <cfloat>
#include <cmath>
#include <cstring>
#include <variant>

#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "AppCore.h"
#include "gui/GUIActions.h"
#include "gui/GUIChannelEditor.h"
#include "gui/GUIConfigUtils.h"
#include "gui/GUIPianoRoll.h"
#include "gui/GUIPlatform.h"
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
float UiScaleFromIndex(int idx);
const char* UiScaleLabelFromIndex(int idx);
void DrawStatusBadge(const GUIState& state);
using gui::DrawChannelEditor;
using gui::InitializeGUIState;
using gui::RepairGUIStatePaths;
using gui::AppendGUILog;
using gui::RefreshPresetItems;
using gui::ApplySelectedPresetPaths;
using gui::SavePresetDiffFromState;
using gui::AnalyzeRenderPeakFromLogs;
using gui::StartGUIRun;
using gui::StartGUISoundTonePreview;
using gui::StopGUIRunAndPreview;
using gui::TryFinalizeCompletedRun;
using gui::LoadGUIStateFile;
using gui::SaveGUIStateFile;
using gui::GUIStatePath;
using gui::DrawPianoRollPanel;

void SetupImGuiFont()
{
    ImGuiIO& io = ImGui::GetIO();
    const ImWchar* ranges = io.Fonts->GetGlyphRangesJapanese();
    const char* candidates[] = {
        "C:\\Windows\\Fonts\\meiryo.ttc",
        "C:\\Windows\\Fonts\\YuGothM.ttc",
        "C:\\Windows\\Fonts\\msgothic.ttc"
    };

    // 日本語表示を崩さないため、Windows既定フォント候補を順に試す。
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

    GLFWwindow* window = glfwCreateWindow(1280, 720, "F-Synthesizer", nullptr, nullptr);
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
    // 起動時に「既定値 -> 保存状態の復元 -> 不正値修復」の順で状態を確定する。
    InitializeGUIState(state, [&](const std::string& preferName) { RefreshPresetItems(state, preferName); });

    {
        std::string err;
        if (!LoadGUIStateFile(state, err))
        {
            AppendGUILog(state, "[GUI] gui_state load failed: " + err);
        }
        else
        {
            AppendGUILog(state, "[GUI] gui_state loaded: " + PathToUtf8(GUIStatePath()));
        }
        RepairGUIStatePaths(
            state,
            [&](const std::string& preferName) { RefreshPresetItems(state, preferName); },
            [&](const std::string& line) { AppendGUILog(state, line); });
    }
    int lastFrameTab = state.uiModeTab;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        // 非同期Runの完了を毎フレーム先頭で回収し、UI遷移を遅延させない。
        TryFinalizeCompletedRun(state);

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
        std::string hoverHelp = (state.uiModeTab == 0)
            ? "Sound: 音色を作り、選択チャンネルをすぐ試聴します。"
            : "Music: ピアノロールを確認し、試聴またはWAV書き出しを行います。";
        auto updateHoverHelp = [&](const char* text)
        {
            if (ImGui::IsItemHovered())
            {
                hoverHelp = text;
            }
        };

        ImGui::Separator();
        static int syncedTab = -1;
        if (ImGui::BeginTable("top_header_row", 2, ImGuiTableFlags_SizingStretchSame))
        {
            ImGui::TableSetupColumn("left", ImGuiTableColumnFlags_WidthStretch, 0.72f);
            ImGui::TableSetupColumn("right", ImGuiTableColumnFlags_WidthFixed, 360.0f);
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            if (ImGui::BeginTabBar("mode_tabs"))
            {
                const bool needSync = (syncedTab != state.uiModeTab);
                ImGuiTabItemFlags soundFlags =
                    (needSync && state.uiModeTab == 0) ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
                ImGuiTabItemFlags musicFlags =
                    (needSync && state.uiModeTab == 1) ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
                if (ImGui::BeginTabItem("Sound", nullptr, soundFlags))
                {
                    state.uiModeTab = 0;
                    ImGui::EndTabItem();
                }
                updateHoverHelp("Sound: 音作りとチャンネル試聴を行うモードです。");
                if (ImGui::BeginTabItem("Music", nullptr, musicFlags))
                {
                    state.uiModeTab = 1;
                    ImGui::EndTabItem();
                }
                updateHoverHelp("Music: ピアノロール確認と書き出しを行うモードです。");
                ImGui::EndTabBar();
                syncedTab = state.uiModeTab;
            }

            ImGui::TableSetColumnIndex(1);
            DrawStatusBadge(state);
            ImGui::SameLine();
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 16.0f);
            ImGui::AlignTextToFramePadding();
            ImGui::TextUnformatted("UI Scale");
            ImGui::SameLine();
            ImGui::SetNextItemWidth(120.0f);
            const char* uiScales[] = { "100%", "125%", "150%" };
            if (ImGui::Combo("##ui_scale", &state.uiScaleIndex, uiScales, IM_ARRAYSIZE(uiScales)))
            {
                AppendGUILog(state, std::string("[GUI] UI scale changed: ") + UiScaleLabelFromIndex(state.uiScaleIndex));
            }
            updateHoverHelp("UI全体の表示倍率を変更します。");
            ImGui::EndTable();
        }
        ImGui::Separator();
        if (state.uiModeTab != lastFrameTab)
        {
            // タブ切替時に再生/実行が残ると意図しない継続再生になるため即停止する。
            if (state.running || state.playback.playing.load(std::memory_order_relaxed))
            {
                StopGUIRunAndPreview(state);
                AppendGUILog(state, "[GUI] Playback stopped by tab switch.");
            }
            lastFrameTab = state.uiModeTab;
        }
        ImGui::Separator();
        ImGui::BeginDisabled(state.running);
        if (state.uiModeTab == 0)
        {
            if (ImGui::Button("Play Preview (Selected ch)"))
            {
                StartGUIRun(state, true);
            }
            updateHoverHelp("選択中チャンネルだけを再生成して再生します。");
            ImGui::SameLine();
            if (ImGui::Button("Play Tone (C4)"))
            {
                StartGUISoundTonePreview(state);
            }
            updateHoverHelp("MIDIに依存せず、現在の音色設定でC4の単音を試聴します。");
            ImGui::SameLine();
            ImGui::Checkbox("Loop Preview", &state.previewLoop);
            updateHoverHelp("Preview終了後に先頭へ戻ってループ再生します。");
        }
        else
        {
            if (ImGui::Button("Export WAV"))
            {
                StartGUIRun(state, false);
            }
            updateHoverHelp("現在設定でWAVを書き出します。");
            ImGui::SameLine();
            if (ImGui::Button("Play Preview (Display ch)"))
            {
                state.selectedChannel = std::clamp(state.pianoRoll.displayChannel, 0, 15);
                StartGUIRun(state, true);
            }
            updateHoverHelp("ピアノロール表示チャンネルを再生成して再生します。");
            ImGui::SameLine();
            ImGui::Checkbox("Loop Preview", &state.previewLoop);
            updateHoverHelp("Preview終了後に先頭へ戻ってループ再生します。");
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        // Stopは「レンダ中」または「プレビュー再生中」のどちらでも有効。
        const bool canStop = state.running || state.playback.playing.load(std::memory_order_relaxed);
        ImGui::BeginDisabled(!canStop);
        if (ImGui::Button("Stop"))
        {
            StopGUIRunAndPreview(state);
        }
        updateHoverHelp("実行中のレンダまたは再生中のPreviewを停止します。");
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled(state.running);
        if (ImGui::Button("Close"))
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        updateHoverHelp("GUIを終了します。");
        ImGui::EndDisabled();
        ImGui::SameLine();
        const std::string helpLine = std::string("Help: ") + hoverHelp;
        const float helpX = ImGui::GetWindowContentRegionMax().x - ImGui::CalcTextSize(helpLine.c_str()).x;
        if (helpX > ImGui::GetCursorPosX())
        {
            ImGui::SetCursorPosX(helpX);
        }
        ImGui::TextDisabled("%s", helpLine.c_str());
        ImGui::Separator();

        static bool logPanelExpanded = false;
        const float availY = ImGui::GetContentRegionAvail().y;
        const float splitterThickness = 8.0f;
        const float logHeaderReserve = ImGui::GetFrameHeightWithSpacing() + 4.0f;
        const float reserveForLog = logPanelExpanded
            ? (splitterThickness + logHeaderReserve + state.logPanelHeight + 8.0f)
            : (logHeaderReserve + 6.0f);
        const float bodyHeight = (std::max)(180.0f, availY - reserveForLog);
        ImGui::BeginChild("body_panel", ImVec2(0, bodyHeight), true);
        if (state.uiModeTab == 0)
        {
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
                    if (SavePresetDiffFromState(state, p, err))
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
                    if (SavePresetDiffFromState(state, p, err))
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
                    gui::EnsureChannelConfigs(state);
                    gui::EnsureChannelMixStates(state);
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
        }
        else
        {
            ImGui::TextUnformatted("Music");
            ImGui::Separator();
            gui::EnsureChannelConfigs(state);
            gui::EnsureChannelMixStates(state);

            ImGui::BeginDisabled(state.running);
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

            ImGui::Separator();
            ImGui::TextUnformatted("Output Target");
            int outputMode = (state.targetChannel < 0) ? 0 : 1;
            if (ImGui::RadioButton("All Channels", outputMode == 0))
            {
                state.targetChannel = -1;
                state.presetDirty = true;
            }
            ImGui::SameLine();
            if (ImGui::RadioButton("Single Channel", outputMode == 1))
            {
                state.targetChannel = std::clamp(state.pianoRoll.displayChannel, 0, 15);
                state.presetDirty = true;
            }
            if (state.targetChannel >= 0)
            {
                ImGui::SetNextItemWidth(220.0f);
                int singleTarget = std::clamp(state.targetChannel, 0, 15);
                if (ImGui::SliderInt("Target Ch", &singleTarget, 0, 15))
                {
                    state.targetChannel = singleTarget;
                    state.presetDirty = true;
                }
            }

            ImGui::Separator();
            ImGui::TextUnformatted("Music Mixer / Assignment");
            ImGui::Checkbox("Drum ch10 Special Handling", &state.drumChannelSpecialHandling);
            ImGui::SameLine();
            if (ImGui::Button("Auto Setup Drum ch10"))
            {
                state.channelAssignments[9] = 9;
                ChannelConfig& drumCh = (*state.channelConfigs)[9];
                const bool isDrumSource =
                    std::holds_alternative<DrumConfig>(drumCh.source) ||
                    std::holds_alternative<DrumKitConfig>(drumCh.source);
                if (!isDrumSource)
                {
                    drumCh.source = gui::DefaultSourceByType(4);
                }
                state.presetDirty = true;
            }

            if (ImGui::BeginTable("music_mixer_assignment_table", 8, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchSame))
            {
                ImGui::TableSetupColumn("ch", ImGuiTableColumnFlags_WidthFixed, 42.0f);
                ImGui::TableSetupColumn("assign", ImGuiTableColumnFlags_WidthFixed, 72.0f);
                ImGui::TableSetupColumn("M", ImGuiTableColumnFlags_WidthFixed, 32.0f);
                ImGui::TableSetupColumn("S", ImGuiTableColumnFlags_WidthFixed, 32.0f);
                ImGui::TableSetupColumn("Level");
                ImGui::TableSetupColumn("Pan");
                ImGui::TableSetupColumn("Gain");
                ImGui::TableSetupColumn("Note");
                ImGui::TableHeadersRow();

                const auto sliderMix = [&](const char* label, double& value, float minV, float maxV) -> bool
                {
                    float v = static_cast<float>(value);
                    const bool edited = ImGui::SliderFloat(label, &v, minV, maxV, "%.2f");
                    if (edited)
                    {
                        value = static_cast<double>(v);
                    }
                    return edited;
                };

                for (int ch = 0; ch < 16; ch++)
                {
                    ChannelMixState& mix = (*state.channelMixStates)[ch];
                    ImGui::TableNextRow();
                    ImGui::PushID(ch);

                    ImGui::TableSetColumnIndex(0);
                    if (ch == 9 && state.drumChannelSpecialHandling)
                    {
                        ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.25f, 1.0f), "ch%d", ch);
                    }
                    else
                    {
                        ImGui::Text("ch%d", ch);
                    }

                    ImGui::TableSetColumnIndex(1);
                    int assigned = std::clamp(state.channelAssignments[ch], 0, 15);
                    ImGui::SetNextItemWidth(-FLT_MIN);
                    if (ImGui::SliderInt("##assign", &assigned, 0, 15, "ch%d"))
                    {
                        state.channelAssignments[ch] = assigned;
                        state.presetDirty = true;
                    }

                    ImGui::TableSetColumnIndex(2);
                    if (ImGui::Checkbox("##mute", &mix.mute)) state.presetDirty = true;

                    ImGui::TableSetColumnIndex(3);
                    if (ImGui::Checkbox("##solo", &mix.solo)) state.presetDirty = true;

                    ImGui::TableSetColumnIndex(4);
                    if (sliderMix("##level", mix.level, 0.0f, 2.0f)) state.presetDirty = true;

                    ImGui::TableSetColumnIndex(5);
                    if (sliderMix("##pan", mix.pan, -1.0f, 1.0f)) state.presetDirty = true;

                    ImGui::TableSetColumnIndex(6);
                    if (sliderMix("##gain", mix.gain, 0.0f, 4.0f)) state.presetDirty = true;

                    ImGui::TableSetColumnIndex(7);
                    if (ch == 9 && state.drumChannelSpecialHandling)
                    {
                        const int src = std::clamp(state.channelAssignments[ch], 0, 15);
                        const SourceConfig& srcCfg = (*state.channelConfigs)[src].source;
                        const bool mappedToDrum =
                            std::holds_alternative<DrumConfig>(srcCfg) ||
                            std::holds_alternative<DrumKitConfig>(srcCfg);
                        if (mappedToDrum)
                        {
                            ImGui::TextUnformatted("Drum OK");
                        }
                        else
                        {
                            ImGui::TextColored(ImVec4(0.95f, 0.45f, 0.35f, 1.0f), "Not Drum");
                        }
                    }
                    else
                    {
                        ImGui::TextUnformatted("-");
                    }

                    ImGui::PopID();
                }
                ImGui::EndTable();
            }
            ImGui::EndDisabled();
            ImGui::Separator();
            DrawPianoRollPanel(
                state.pianoRoll,
                state.midiPath,
                &state.playback,
                [&](const std::string& line) { AppendGUILog(state, line); },
                [&]()
                {
                    state.selectedChannel = std::clamp(state.pianoRoll.displayChannel, 0, 15);
                    StartGUIRun(state, true);
                },
                [&]()
                {
                    StopGUIRunAndPreview(state);
                });
        }
        ImGui::EndChild();

        ImGui::Separator();
        if (logPanelExpanded)
        {
            ImGui::InvisibleButton("log_splitter", ImVec2(-1.0f, splitterThickness));
            if (ImGui::IsItemHovered() || ImGui::IsItemActive())
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
            }
            if (ImGui::IsItemActive())
            {
                state.logPanelHeight = std::clamp(
                    state.logPanelHeight - ImGui::GetIO().MouseDelta.y,
                    140.0f,
                    520.0f);
            }
        }
        ImGui::SetNextItemOpen(logPanelExpanded, ImGuiCond_Once);
        logPanelExpanded = ImGui::CollapsingHeader("Logs");
        if (logPanelExpanded)
        {
            ImGui::BeginChild("log_panel", ImVec2(0, state.logPanelHeight), true);
            {
                std::lock_guard<std::mutex> lock(state.logMutex);
                const std::vector<std::string>& visibleLogs = (state.uiModeTab == 1) ? state.musicLogs : state.soundLogs;
                for (const std::string& line : visibleLogs)
                {
                    ImGui::TextUnformatted(line.c_str());
                }
                if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                {
                    ImGui::SetScrollHereY(1.0f);
                }
            }
            ImGui::EndChild();
        }
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
