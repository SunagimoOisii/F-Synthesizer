void DrawMainWindowFrame(
    GUIState& state,
    GLFWwindow* window,
    int& lastFrameTab,
    int& pendingPresetIndex,
    int& pendingPresetOriginalIndex,
    bool& pendingCloseRequest,
    bool& openUnsavedPopupNextFrame)
{
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
auto saveWorkspaceOnly = [&]() -> bool
{
    std::string err;
    if (!SaveGUIStateFile(state, err))
    {
        AppendGUILog(state, "[GUI] Save workspace failed: " + err);
        RaiseGUIError(state, "Save Project に失敗しました。設定を確認してください。(" + err + ")", 4, true);
        return false;
    }
    AppendGUILog(state, "[GUI] Workspace saved.");
    return true;
};
auto saveAll = [&]() -> bool
{
    if (!saveWorkspaceOnly())
    {
        return false;
    }
    if (state.presetDirty)
    {
        std::string err;
        std::string presetName = state.presetName;
        if (presetName.empty())
        {
            presetName = "custom";
        }
        const std::filesystem::path presetPath =
            FindProjectRootPath() / "config" / "presets" / (presetName + ".json");
        if (!SavePresetDiffFromState(state, presetPath, err))
        {
            AppendGUILog(state, "[GUI] Save preset failed: " + err);
            RaiseGUIError(state, "Save All の SoundAsset 保存に失敗しました。(" + err + ")", 3, true);
            return false;
        }
        state.lastPresetPath = PathToUtf8(presetPath);
        RefreshPresetItems(state, presetName);
        AppendGUILog(state, "[GUI] Preset saved by Save All: " + state.lastPresetPath);
    }
    state.presetDirty = false;
    AppendGUILog(state, "[GUI] Save All completed.");
    return true;
};
auto applyPresetByIndex = [&](int idx)
{
    if (idx < 0 || idx >= static_cast<int>(state.presetItems.size()))
    {
        return;
    }
    state.presetIndex = idx;
    std::string err;
    if (ApplySelectedPresetPaths(state, err))
    {
        state.presetDirty = false;
        AppendGUILog(state, "[GUI] Preset applied: " + state.presetItems[idx] +
            " -> slot s" + std::to_string(std::clamp(state.selectedSoundSlot, 0, 15)));
    }
    else
    {
        AppendGUILog(state, "[GUI] Apply preset failed: " + err);
        RaiseGUIError(state, "Preset 適用に失敗しました。Sound 設定を確認してください。(" + err + ")", 3, true);
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
ImGui::TextDisabled("Save Targets: SoundAsset(Preset) / MusicProject(GUI+PianoRoll) / Workspace(UI state)");
ImGui::SameLine();
ImGui::BeginDisabled(state.running);
if (ImGui::Button("Save Project"))
{
    // MusicProject + Workspace を保存。SoundAsset は含めない。
    if (saveWorkspaceOnly())
    {
        AppendGUILog(state, "[GUI] Save Project: MusicProject + Workspace");
    }
}
updateHoverHelp("MusicProject(GUI+PianoRoll) と Workspace(UI state) を保存します。");
ImGui::SameLine();
if (ImGui::Button("Save All"))
{
    saveAll();
}
updateHoverHelp("SoundAsset + MusicProject + Workspace をまとめて保存します。");
ImGui::EndDisabled();
if (state.hasUiError)
{
    auto suggestedFix = [&]() -> const char*
    {
        switch (state.uiErrorAction)
        {
        case 1: return "Suggested Fix: MIDI path を選び直す";
        case 2: return "Suggested Fix: Output path を選び直す";
        case 3: return "Suggested Fix: Sound タブで設定を確認する";
        case 4: return "Suggested Fix: Music タブで設定を確認する";
        default: return "Suggested Fix: 設定を見直して再実行する";
        }
    };
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.95f, 0.35f, 0.35f, 1.0f), "Problem: %s", state.uiErrorMessage.c_str());
    ImGui::TextDisabled("%s", suggestedFix());
    if (state.uiErrorAction == 1)
    {
        ImGui::SameLine();
        if (ImGui::Button("Recover: Browse MIDI"))
        {
            std::string selected;
            const wchar_t* midiFilter = L"MIDI Files (*.mid;*.midi)\0*.mid;*.midi\0All Files (*.*)\0*.*\0";
            if (BrowseOpenPath(state.midiPath, midiFilter, selected))
            {
                strncpy_s(state.midiPath, sizeof(state.midiPath), selected.c_str(), _TRUNCATE);
                state.presetDirty = true;
                ClearGUIError(state);
            }
        }
    }
    else if (state.uiErrorAction == 2)
    {
        ImGui::SameLine();
        if (ImGui::Button("Recover: Browse Output"))
        {
            std::string selected;
            const wchar_t* wavFilter = L"WAV Files (*.wav)\0*.wav\0All Files (*.*)\0*.*\0";
            if (BrowseSavePath(state.wavPath, wavFilter, L"wav", selected))
            {
                strncpy_s(state.wavPath, sizeof(state.wavPath), selected.c_str(), _TRUNCATE);
                state.presetDirty = true;
                ClearGUIError(state);
            }
        }
    }
    else if (state.uiErrorAction == 3)
    {
        ImGui::SameLine();
        if (ImGui::Button("Recover: Go Sound Tab"))
        {
            state.uiModeTab = 0;
            ClearGUIError(state);
        }
    }
    else if (state.uiErrorAction == 4)
    {
        ImGui::SameLine();
        if (ImGui::Button("Recover: Go Music Tab"))
        {
            state.uiModeTab = 1;
            ClearGUIError(state);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear Error"))
    {
        ClearGUIError(state);
    }
}
if (state.showErrorDialog)
{
    ImGui::OpenPopup("Error");
    state.showErrorDialog = false;
}
if (ImGui::BeginPopupModal("Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
{
    ImGui::TextWrapped("Problem: %s", state.uiErrorMessage.c_str());
    switch (state.uiErrorAction)
    {
    case 1: ImGui::TextDisabled("Suggested Fix: MIDI path を選び直してください。"); break;
    case 2: ImGui::TextDisabled("Suggested Fix: Output path を選び直してください。"); break;
    case 3: ImGui::TextDisabled("Suggested Fix: Sound タブの設定を確認してください。"); break;
    case 4: ImGui::TextDisabled("Suggested Fix: Music タブの設定を確認してください。"); break;
    default: ImGui::TextDisabled("Suggested Fix: 設定を見直して再実行してください。"); break;
    }
    if (ImGui::Button("OK"))
    {
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Dismiss"))
    {
        ClearGUIError(state);
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}
ImGui::Separator();
ImGui::BeginDisabled(state.running);
if (state.uiModeTab == 0)
{
    if (ImGui::Button("Play Preview (PR ch + Selected Sound Slot)"))
    {
        StartGUIRun(state, true);
    }
    updateHoverHelp("ピアノロール表示chのノートを、選択中Sound Slotの音色でプレビュー再生します。");
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
        state.selectedSoundSlot = std::clamp(state.pianoRoll.displayChannel, 0, 15);
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
    if (state.presetDirty)
    {
        pendingCloseRequest = true;
        ImGui::OpenPopup("Unsaved Changes");
    }
    else
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
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
if (openUnsavedPopupNextFrame)
{
    ImGui::OpenPopup("Unsaved Changes");
    openUnsavedPopupNextFrame = false;
}
if (ImGui::BeginPopupModal("Unsaved Changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
{
    ImGui::TextUnformatted("変更が未保存です。どうしますか？");
    if (ImGui::Button("保存して続行"))
    {
        if (saveAll())
        {
            if (pendingCloseRequest)
            {
                glfwSetWindowShouldClose(window, GLFW_TRUE);
            }
            else if (pendingPresetIndex >= 0)
            {
                applyPresetByIndex(pendingPresetIndex);
            }
            pendingCloseRequest = false;
            pendingPresetIndex = -1;
            pendingPresetOriginalIndex = -1;
            ImGui::CloseCurrentPopup();
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("保存せず続行"))
    {
        if (pendingCloseRequest)
        {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
        else if (pendingPresetIndex >= 0)
        {
            applyPresetByIndex(pendingPresetIndex);
        }
        pendingCloseRequest = false;
        pendingPresetIndex = -1;
        pendingPresetOriginalIndex = -1;
        ImGui::CloseCurrentPopup();
    }
    ImGui::SameLine();
    if (ImGui::Button("キャンセル"))
    {
        if (pendingPresetOriginalIndex >= 0)
        {
            state.presetIndex = pendingPresetOriginalIndex;
        }
        pendingCloseRequest = false;
        pendingPresetIndex = -1;
        pendingPresetOriginalIndex = -1;
        openUnsavedPopupNextFrame = false;
        ImGui::CloseCurrentPopup();
    }
    ImGui::EndPopup();
}

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
        const int beforePresetIndex = state.presetIndex;
        if (ImGui::Combo("Preset", &state.presetIndex, presetGetter, &state.presetItems, static_cast<int>(state.presetItems.size())))
        {
            if (state.presetDirty)
            {
                pendingPresetOriginalIndex = beforePresetIndex;
                pendingPresetIndex = state.presetIndex;
                openUnsavedPopupNextFrame = true;
            }
            else
            {
                applyPresetByIndex(state.presetIndex);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Apply Preset Paths"))
        {
            if (state.presetDirty)
            {
                pendingPresetOriginalIndex = state.presetIndex;
                pendingPresetIndex = state.presetIndex;
                openUnsavedPopupNextFrame = true;
            }
            else
            {
                applyPresetByIndex(state.presetIndex);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset Defaults"))
        {
            ClearGUIError(state);
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
                RaiseGUIError(state, "Preset 保存に失敗しました。(" + err + ")", 3, true);
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
                RaiseGUIError(state, "Preset 複製に失敗しました。(" + err + ")", 3, true);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset Sound Slot"))
        {
            gui::EnsureChannelConfigs(state);
            AppConfig def = DefaultConfig();
            if (def.channelConfigs)
            {
                (*state.channelConfigs)[state.selectedSoundSlot] = (*def.channelConfigs)[state.selectedSoundSlot];
                state.presetDirty = true;
                AppendGUILog(state, "[GUI] Sound slot reset: s" + std::to_string(state.selectedSoundSlot));
            }
        }
        if (!state.lastPresetPath.empty())
        {
            ImGui::Text("Last Preset: %s", state.lastPresetPath.c_str());
        }

        ImGui::TextDisabled("Song export settings are in Music tab.");
        const char* waves[] = { "sine", "square", "saw", "triangle" };
        state.presetDirty |= ImGui::Combo("Default Wave", &state.defaultWave, waves, IM_ARRAYSIZE(waves));
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

    ImGui::Separator();
    ImGui::TextUnformatted("Sound Reference");
    if (ImGui::RadioButton("Snapshot (Recommended)", state.assetReferenceMode == 0))
    {
        state.assetReferenceMode = 0;
        state.presetDirty = true;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Link (Advanced)", state.assetReferenceMode == 1))
    {
        state.assetReferenceMode = 1;
        state.presetDirty = true;
    }
    if (state.assetReferenceMode == 0)
    {
        ImGui::TextDisabled("Snapshot keeps export reproducible.");
    }
    else
    {
        ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.25f, 1.0f), "Link may change export result after sound edits.");
    }
    if (ImGui::CollapsingHeader("Reference Mode Details"))
    {
        state.showReferenceAdvanced = true;
        ImGui::TextWrapped("Snapshot: prioritize reproducibility for beginners.");
        ImGui::TextWrapped("Link: follow latest SoundAsset edits for iterative sound design.");
    }
    else
    {
        state.showReferenceAdvanced = false;
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
    ImGui::TextUnformatted("Render Settings");
    state.presetDirty |= ImGui::InputInt("Sample Rate", &state.sampleRate);
    state.presetDirty |= ImGui::InputInt("Initial Seconds", &state.initialSeconds);
    state.presetDirty |= ImGui::InputInt("Bits", &state.bits);
    state.presetDirty |= ImGui::InputFloat("Extra Release (sec)", &state.extraReleaseSec, 0.01f, 0.1f, "%.2f");
    state.presetDirty |= ImGui::Checkbox("Serial Save (timestamp suffix)", &state.serialSave);

    ImGui::Separator();
    ImGui::TextUnformatted("Music Mixer / Assignment");
    constexpr int drumMidiChannel = 9; // MIDI ch10 (0-based index)
    const int prChannel = std::clamp(state.pianoRoll.displayChannel, 0, 15);
    const int assignedFromPr = std::clamp(state.channelAssignments[prChannel], 0, 15);
    const bool singleOutput = (state.targetChannel >= 0);
    ImGui::Text("PR Channel: ch%d  ->  Assigned Sound Slot: s%d", prChannel, assignedFromPr);
    if (singleOutput)
    {
        ImGui::Text("Current Export: Single Channel ch%d", std::clamp(state.targetChannel, 0, 15));
    }
    else
    {
        ImGui::TextUnformatted("Current Export: All Channels");
    }
    if (ImGui::Button("Set PR Assign = Same slot index"))
    {
        state.channelAssignments[prChannel] = prChannel;
        state.presetDirty = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Set Output Target = PR ch"))
    {
        state.targetChannel = prChannel;
        state.presetDirty = true;
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset All Assign = Same slot index"))
    {
        for (int ch = 0; ch < 16; ch++)
        {
            state.channelAssignments[ch] = ch;
        }
        state.presetDirty = true;
    }

    auto applyDrumCh10Setup = [&]()
    {
        state.channelAssignments[drumMidiChannel] = drumMidiChannel;
        ChannelConfig& drumCh = (*state.channelConfigs)[drumMidiChannel];
        const bool isDrumSource =
            std::holds_alternative<DrumConfig>(drumCh.source) ||
            std::holds_alternative<DrumKitConfig>(drumCh.source);
        if (!isDrumSource)
        {
            drumCh.source = gui::DefaultSourceByType(config::SourceKindToIndex(config::SourceKind::DrumKit));
        }
        state.presetDirty = true;
    };

    ImGui::Separator();
    ImGui::TextUnformatted("Drum (MIDI ch10) Quick Setup");
    ImGui::TextDisabled("Use this when ch10 should behave as percussion.");
    ImGui::Checkbox("Enable ch10 Drum Guard", &state.drumChannelSpecialHandling);
    ImGui::SameLine();
    if (ImGui::Button("Auto Setup ch10 Drum"))
    {
        applyDrumCh10Setup();
    }
    ImGui::SameLine();
    if (ImGui::Button("Focus PR ch10"))
    {
        state.pianoRoll.displayChannel = drumMidiChannel;
    }

    if (ImGui::BeginTable("music_mixer_assignment_table", 8,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY,
            ImVec2(0.0f, 290.0f)))
    {
        ImGui::TableSetupColumn("ch", ImGuiTableColumnFlags_WidthFixed, 42.0f);
        ImGui::TableSetupColumn("slot", ImGuiTableColumnFlags_WidthFixed, 78.0f);
        ImGui::TableSetupColumn("M", ImGuiTableColumnFlags_WidthFixed, 32.0f);
        ImGui::TableSetupColumn("S", ImGuiTableColumnFlags_WidthFixed, 32.0f);
        ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthStretch, 0.36f);
        ImGui::TableSetupColumn("Pan", ImGuiTableColumnFlags_WidthStretch, 0.32f);
        ImGui::TableSetupColumn("Gain", ImGuiTableColumnFlags_WidthStretch, 0.32f);
        ImGui::TableSetupColumn("Note", ImGuiTableColumnFlags_WidthFixed, 92.0f);
        ImGui::TableSetupScrollFreeze(0, 1);
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
            if (ch == prChannel)
            {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(36, 56, 96, 64));
            }
            else if (singleOutput && ch == std::clamp(state.targetChannel, 0, 15))
            {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(36, 96, 56, 64));
            }
            if (state.drumChannelSpecialHandling && ch == drumMidiChannel)
            {
                ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, IM_COL32(120, 86, 20, 68));
            }

            ImGui::TableSetColumnIndex(0);
            if (ch == drumMidiChannel && state.drumChannelSpecialHandling)
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
            if (ImGui::SliderInt("##assign", &assigned, 0, 15, "s%d"))
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
            if (ch == drumMidiChannel && state.drumChannelSpecialHandling)
            {
                const int src = std::clamp(state.channelAssignments[ch], 0, 15);
                const SourceConfig& srcCfg = (*state.channelConfigs)[src].source;
                const bool mappedToDrum =
                    std::holds_alternative<DrumConfig>(srcCfg) ||
                    std::holds_alternative<DrumKitConfig>(srcCfg);
                if (mappedToDrum)
                {
                    ImGui::TextUnformatted("Drum Ready");
                }
                else
                {
                    ImGui::TextColored(ImVec4(0.95f, 0.45f, 0.35f, 1.0f), "Needs Drum Source");
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
            state.selectedSoundSlot = std::clamp(state.pianoRoll.displayChannel, 0, 15);
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
}
