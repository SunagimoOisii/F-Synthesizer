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
auto composeHoverHelp = [](const char* what, const char* impact, const char* caution = nullptr) -> std::string
{
    (void)what; // 基本はラベルで意図が読めるため、Helpは「影響/注意」を優先表示する。
    std::string line = std::string("影響: ") + impact;
    if (caution != nullptr && caution[0] != '\0')
    {
        line += " / 注意: ";
        line += caution;
    }
    return line;
};
std::string hoverHelp = (state.UIModeTab == 0)
    ? composeHoverHelp(
        "Soundモードを表示します。",
        "Sound Slot中心に音色編集と試聴を行えます。")
    : composeHoverHelp(
        "Musicモードを表示します。",
        "ピアノロール確認、プレビュー、WAV書き出しを行えます。");
// 新規UI項目の追加時は「項目描画 -> updateHoverHelp(what, impact, caution)」の順で統一する。
// whatは補助情報として受け取るが、Help表示は原則「影響/注意」を優先する。
auto updateHoverHelp = [&](const char* what, const char* impact, const char* caution = nullptr)
{
    if (ImGui::IsItemHovered())
    {
        hoverHelp = composeHoverHelp(what, impact, caution);
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
        const bool needSync = (syncedTab != state.UIModeTab);
        ImGuiTabItemFlags soundFlags =
            (needSync && state.UIModeTab == 0) ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
        ImGuiTabItemFlags musicFlags =
            (needSync && state.UIModeTab == 1) ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
        if (ImGui::BeginTabItem("Sound", nullptr, soundFlags))
        {
            state.UIModeTab = 0;
            ImGui::EndTabItem();
        }
        updateHoverHelp(
            "Soundモードへ切り替えます。",
            "Sound編集と試聴の操作を表示します。");
        if (ImGui::BeginTabItem("Music", nullptr, musicFlags))
        {
            state.UIModeTab = 1;
            ImGui::EndTabItem();
        }
        updateHoverHelp(
            "Musicモードへ切り替えます。",
            "Music編集と書き出しの操作を表示します。");
        ImGui::EndTabBar();
        syncedTab = state.UIModeTab;
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
    if (ImGui::Combo("##ui_scale", &state.UIScaleIndex, uiScales, IM_ARRAYSIZE(uiScales)))
    {
        AppendGUILog(state, std::string("[GUI] UI scale changed: ") + UIScaleLabelFromIndex(state.UIScaleIndex));
    }
    updateHoverHelp(
        "UI全体の表示倍率を変更します。",
        "表示倍率だけが変わります。");
    ImGui::EndTable();
}
ImGui::Separator();
if (state.UIModeTab != lastFrameTab)
{
    // タブ切替時に再生/実行が残ると意図しない継続再生になるため即停止する。
    if (state.running || state.playback.playing.load(std::memory_order_relaxed))
    {
        StopGUIRunAndPreview(state);
        AppendGUILog(state, "[GUI] Playback stopped by tab switch.");
    }
    lastFrameTab = state.UIModeTab;
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
updateHoverHelp(
    "Save Projectを実行します。",
    "MusicProjectとWorkspaceを保存します。",
    "SoundAsset(Preset)は保存対象に含みません。");
ImGui::SameLine();
if (ImGui::Button("Save All"))
{
    saveAll();
}
updateHoverHelp(
    "Save Allを実行します。",
    "SoundAsset/MusicProject/Workspaceを保存します。");
ImGui::EndDisabled();
if (state.hasUIError)
{
    auto suggestedFix = [&]() -> const char*
    {
        switch (state.UIErrorAction)
        {
        case 1: return "Suggested Fix: MIDI path を選び直す";
        case 2: return "Suggested Fix: Output path を選び直す";
        case 3: return "Suggested Fix: Sound タブで設定を確認する";
        case 4: return "Suggested Fix: Music タブで設定を確認する";
        default: return "Suggested Fix: 設定を見直して再実行する";
        }
    };
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.95f, 0.35f, 0.35f, 1.0f), "Problem: %s", state.UIErrorMessage.c_str());
    ImGui::TextDisabled("%s", suggestedFix());
    if (state.UIErrorAction == 1)
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
        updateHoverHelp(
            "Recover: Browse MIDI を実行します。",
            "MIDI Pathを再選択して復旧します。");
    }
    else if (state.UIErrorAction == 2)
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
        updateHoverHelp(
            "Recover: Browse Output を実行します。",
            "Output Pathを再選択して復旧します。");
    }
    else if (state.UIErrorAction == 3)
    {
        ImGui::SameLine();
        if (ImGui::Button("Recover: Go Sound Tab"))
        {
            state.UIModeTab = 0;
            ClearGUIError(state);
        }
        updateHoverHelp(
            "Recover: Go Sound Tab を実行します。",
            "Soundタブへ移動します。");
    }
    else if (state.UIErrorAction == 4)
    {
        ImGui::SameLine();
        if (ImGui::Button("Recover: Go Music Tab"))
        {
            state.UIModeTab = 1;
            ClearGUIError(state);
        }
        updateHoverHelp(
            "Recover: Go Music Tab を実行します。",
            "Musicタブへ移動します。");
    }
    ImGui::SameLine();
    if (ImGui::Button("Clear Error"))
    {
        ClearGUIError(state);
    }
    updateHoverHelp(
        "Clear Error を実行します。",
        "エラー表示をクリアします。",
        "原因が未解決の場合は再実行時に再発します。");
}
if (state.showErrorDialog)
{
    ImGui::OpenPopup("Error");
    state.showErrorDialog = false;
}
if (ImGui::BeginPopupModal("Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
{
    ImGui::TextWrapped("Problem: %s", state.UIErrorMessage.c_str());
    switch (state.UIErrorAction)
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
    updateHoverHelp(
        "Errorダイアログを閉じます。",
        "表示のみ閉じ、エラー状態は維持します。");
    ImGui::SameLine();
    if (ImGui::Button("Dismiss"))
    {
        ClearGUIError(state);
        ImGui::CloseCurrentPopup();
    }
    updateHoverHelp(
        "Dismiss を実行します。",
        "エラー状態をクリアして閉じます。");
    ImGui::EndPopup();
}
ImGui::Separator();
ImGui::BeginDisabled(state.running);
if (state.UIModeTab == 0)
{
    if (ImGui::Button("Play Preview (PR Channel using Selected Slot)"))
    {
        StartGUIRun(state, true);
    }
    updateHoverHelp(
        "Play Previewを実行します。",
        "表示chを選択中Slotで再生成して再生します。",
        "WAVファイルは出力しません。");
    ImGui::SameLine();
    if (ImGui::Button("Play Tone (C4)"))
    {
        StartGUISoundTonePreview(state);
    }
    updateHoverHelp(
        "Play Tone (C4) を実行します。",
        "現在音色を単音(C4)で試聴します。",
        "楽曲バランス確認には Play Preview を使ってください。");
    ImGui::SameLine();
    ImGui::Checkbox("Loop Preview", &state.previewLoop);
    updateHoverHelp(
        "Loop Previewを切り替えます。",
        "Preview終了後に先頭からループ再生します。");
}
else
{
    if (ImGui::Button("Export WAV"))
    {
        StartGUIRun(state, false);
    }
    updateHoverHelp(
        "Export WAVを実行します。",
        "現在設定でWAVを書き出します。",
        "再生中の場合は停止してから書き出しを開始します。Preview専用操作ではありません。");
    ImGui::SameLine();
    if (ImGui::Button("Play Preview (Display Channel)"))
    {
        StartGUIRun(state, true);
    }
    updateHoverHelp(
        "Play Preview (Display Channel) を実行します。",
        "表示chを現在の割当/ミックスで再生します。",
        "WAVファイルは出力しません。");
    ImGui::SameLine();
    ImGui::Checkbox("Loop Preview", &state.previewLoop);
    updateHoverHelp(
        "Loop Previewを切り替えます。",
        "Preview終了後に先頭からループ再生します。");
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
updateHoverHelp(
    "Stopを実行します。",
    "レンダまたはPreviewを停止します。",
    "未実行時は無効です。");
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
updateHoverHelp(
    "GUIを終了します。",
    "未保存時は確認ダイアログを表示します。");
ImGui::EndDisabled();
ImGui::SameLine();
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
    updateHoverHelp(
        "変更を保存して続行します。",
        "保存後に保留操作を再開します。");
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
    updateHoverHelp(
        "保存せず続行します。",
        "未保存変更を破棄して保留操作を再開します。",
        "保存していない変更は失われます。");
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
    updateHoverHelp(
        "保留操作をキャンセルします。",
        "未保存確認を閉じて現在画面に戻ります。");
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
if (state.UIModeTab == 0)
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
        updateHoverHelp(
            "読み込むPresetを選択します。",
            "Sound設定の読み込み対象が変わります。",
            "未保存変更がある場合は確認ダイアログを表示します。");
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
        updateHoverHelp(
            "選択中Presetを再適用します。",
            "Preset由来の設定パスを現在状態へ反映します。");
        ImGui::SameLine();
        if (ImGui::Button("Reset Defaults"))
        {
            ClearGUIError(state);
            InitializeGUIState(state, [&](const std::string& preferName) { RefreshPresetItems(state, preferName); });
            state.presetDirty = false;
        }
        updateHoverHelp(
            "設定を既定値へ戻します。",
            "GUI状態とSound設定を初期化します。",
            "未保存変更は失われます。");

        ImGui::InputText("Preset Name", state.presetName, IM_ARRAYSIZE(state.presetName));
        updateHoverHelp(
            "保存時のPreset名を入力します。",
            "Save Preset As / Duplicate Preset の保存先ファイル名に使います。");
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
        updateHoverHelp(
            "現在設定をPresetとして保存します。",
            "Preset Name で指定したJSONファイルへ保存します。",
            "同名が存在する場合は上書きされます。");
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
        updateHoverHelp(
            "Presetを複製保存します。",
            "Preset Name に `_copy` を付けた保存名で複製します。");
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
        updateHoverHelp(
            "選択中Sound Slotを初期化します。",
            "対象スロットの音色設定を既定値へ戻します。");
        if (!state.lastPresetPath.empty())
        {
            ImGui::Text("Last Preset: %s", state.lastPresetPath.c_str());
        }

        ImGui::TextDisabled("Song export settings are in Music tab.");
        ImGui::EndDisabled();

        ImGui::TableSetColumnIndex(1);
        state.presetDirty |= DrawChannelEditor(
            state,
            [&](const char* what, const char* impact, const char* caution)
            {
                updateHoverHelp(what, impact, caution);
            });
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
    ImGui::TextDisabled("Music は現在の Sound 設定（割当/ミックス）をそのまま使って Preview/Export します。");
    ImGui::Separator();
    gui::EnsureChannelConfigs(state);
    gui::EnsureChannelMixStates(state);

    ImGui::BeginDisabled(state.running);
    state.presetDirty |= ImGui::InputText("MIDI Path", state.midiPath, IM_ARRAYSIZE(state.midiPath));
    updateHoverHelp(
        "読み込むMIDIファイルを指定します。",
        "Preview/Exportの入力に使います。",
        "無効パスでは再生/書き出しに失敗します。");
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
    updateHoverHelp(
        "MIDIファイル選択ダイアログを開きます。",
        "MIDI Pathを更新します。",
        "想定外のファイルを選ぶと解析エラーになる場合があります。");
    ImGui::SameLine();
    if (ImGui::Button("Copy MIDI"))
    {
        ImGui::SetClipboardText(state.midiPath);
    }
    updateHoverHelp(
        "MIDI Path をコピーします。",
        "パスをクリップボードへコピーします。");
    {
        const std::string compact = CompactPathForUI(state.midiPath);
        ImGui::TextDisabled("%s", compact.c_str());
        if (ImGui::IsItemHovered() && std::strlen(state.midiPath) > 0)
        {
            ImGui::SetTooltip("%s", state.midiPath);
        }
    }
    state.presetDirty |= ImGui::InputText("Output Path", state.wavPath, IM_ARRAYSIZE(state.wavPath));
    updateHoverHelp(
        "WAVの書き出し先パスを指定します。",
        "WAVの出力先が変わります。",
        "Serial Save が無効だと既存ファイルを上書きする場合があります。");
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
    updateHoverHelp(
        "出力先WAVパス選択ダイアログを開きます。",
        "Output Pathを更新します。",
        "保存権限のない場所は失敗します。");
    ImGui::SameLine();
    if (ImGui::Button("Copy Output"))
    {
        ImGui::SetClipboardText(state.wavPath);
    }
    updateHoverHelp(
        "Output Path をコピーします。",
        "パスをクリップボードへコピーします。");
    {
        const std::string compact = CompactPathForUI(state.wavPath);
        ImGui::TextDisabled("%s", compact.c_str());
        if (ImGui::IsItemHovered() && std::strlen(state.wavPath) > 0)
        {
            ImGui::SetTooltip("%s", state.wavPath);
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
    updateHoverHelp(
        "出力対象を All Channels にします。",
        "全MIDIチャンネルを出力します。",
        "PR Channel の表示先とは独立です。");
    ImGui::SameLine();
    if (ImGui::RadioButton("Single Channel", outputMode == 1))
    {
        state.targetChannel = std::clamp(state.pianoRoll.displayChannel, 0, 15);
        state.presetDirty = true;
    }
    updateHoverHelp(
        "出力対象を Single Channel にします。",
        "指定chのみを出力します。",
        "PR Channel とは自動連動しません。");
    if (state.targetChannel >= 0)
    {
        ImGui::SetNextItemWidth(220.0f);
        int singleTarget = std::clamp(state.targetChannel, 0, 15);
        if (ImGui::SliderInt("Target Ch", &singleTarget, 0, 15))
        {
            state.targetChannel = singleTarget;
            state.presetDirty = true;
        }
        updateHoverHelp(
            "Single Channel の対象chを変更します。",
            "Export対象chが変わります。",
            "プレビュー表示chの切替とは別設定です。");
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Render Settings");
    state.presetDirty |= ImGui::InputInt("Sample Rate", &state.sampleRate);
    updateHoverHelp(
        "出力サンプルレートを設定します。",
        "音質・負荷・サイズが変わります。",
        "高すぎる値は処理時間を増やします。");
    state.presetDirty |= ImGui::InputInt("Initial Seconds", &state.initialSeconds);
    updateHoverHelp(
        "最低レンダ秒数を設定します。",
        "出力の最小長さを確保します。",
        "短くしすぎると余韻が切れやすくなります。");
    state.presetDirty |= ImGui::InputInt("Bits", &state.bits);
    updateHoverHelp(
        "出力ビット深度を設定します。",
        "ダイナミックレンジと互換性が変わります。",
        "現行実装は16bitのみ有効です。");
    state.presetDirty |= ImGui::InputFloat("Extra Release (sec)", &state.extraReleaseSec, 0.01f, 0.1f, "%.2f");
    updateHoverHelp(
        "ノート終端後の追加リリース時間を設定します。",
        "尻切れを抑え、余韻を確保します。",
        "長くしすぎると書き出し時間とサイズが増えます。");
    state.presetDirty |= ImGui::Checkbox("Serial Save (timestamp suffix)", &state.serialSave);
    updateHoverHelp(
        "連番保存（タイムスタンプ付与）を切り替えます。",
        "同名出力の上書きを避けます。",
        "無効時は同名ファイルを上書きします。");

    ImGui::Separator();
    ImGui::TextUnformatted("Music Mixer / Assignment");
    constexpr int drumMidiChannel = 9; // MIDI ch10 (0-based index)
    const int prChannel = std::clamp(state.pianoRoll.displayChannel, 0, 15);
    const int assignedFromPr = std::clamp(state.channelAssignments[prChannel], 0, 15);
    const bool singleOutput = (state.targetChannel >= 0);
    ImGui::Text("PR Channel: ch%d  ->  Assigned Sound Slot: s%d", prChannel, assignedFromPr);
    updateHoverHelp(
        "現在のPR表示chと割当先Sound Slotを確認します。",
        "再生に使う音色割当を確認できます。",
        "表示chと書き出し対象chは別です。");
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
    updateHoverHelp(
        "PR表示chの割当を同一番号スロットへ合わせます。",
        "表示chの割当を同一番号へそろえます。");
    ImGui::SameLine();
    if (ImGui::Button("Set Output Target = PR ch"))
    {
        state.targetChannel = prChannel;
        state.presetDirty = true;
    }
    updateHoverHelp(
        "Output Target を現在のPR chへ合わせます。",
        "Single Channelの対象を更新します。",
        "All Channels の場合は Single Channel 選択後に有効になります。");
    ImGui::SameLine();
    if (ImGui::Button("Reset All Assign = Same slot index"))
    {
        for (int ch = 0; ch < 16; ch++)
        {
            state.channelAssignments[ch] = ch;
        }
        state.presetDirty = true;
    }
    updateHoverHelp(
        "全chの割当を同一番号スロットへ戻します。",
        "ch番号とSlot番号の対応を初期状態へ戻します。");

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
    updateHoverHelp(
        "ch10 Drum Guard を切り替えます。",
        "ch10をドラム運用として監視します。");
    ImGui::SameLine();
    if (ImGui::Button("Auto Setup ch10 Drum"))
    {
        applyDrumCh10Setup();
    }
    updateHoverHelp(
        "ch10 のドラム向け自動セットアップを実行します。",
        "ch10の割当と音源種別をドラム向けに調整します。");
    ImGui::SameLine();
    if (ImGui::Button("Focus PR ch10"))
    {
        state.pianoRoll.displayChannel = drumMidiChannel;
    }
    updateHoverHelp(
        "PR表示chをch10へ切り替えます。",
        "ドラムノート確認へすぐ移動できます。");

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
            updateHoverHelp(
                "MIDI ch の Sound Slot 割当を変更します。",
                "各chの音色割当が変わります。");

            ImGui::TableSetColumnIndex(2);
            if (ImGui::Checkbox("##mute", &mix.mute)) state.presetDirty = true;
            updateHoverHelp(
                "Mute を切り替えます。",
                "対象chを無音化します。");

            ImGui::TableSetColumnIndex(3);
            if (ImGui::Checkbox("##solo", &mix.solo)) state.presetDirty = true;
            updateHoverHelp(
                "Solo を切り替えます。",
                "Solo対象以外のchを抑制します。");

            ImGui::TableSetColumnIndex(4);
            if (sliderMix("##level", mix.level, 0.0f, 2.0f)) state.presetDirty = true;
            updateHoverHelp(
                "Level を調整します。",
                "chの基本音量が変わります。");

            ImGui::TableSetColumnIndex(5);
            if (sliderMix("##pan", mix.pan, -1.0f, 1.0f)) state.presetDirty = true;
            updateHoverHelp(
                "Pan を調整します。",
                "左右定位が変わります。");

            ImGui::TableSetColumnIndex(6);
            if (sliderMix("##gain", mix.gain, 0.0f, 4.0f)) state.presetDirty = true;
            updateHoverHelp(
                "Gain を調整します。",
                "追加ゲインが変わります。",
                "上げすぎるとクリップしやすくなります。");

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
        const std::vector<std::string>& visibleLogs = (state.UIModeTab == 1) ? state.musicLogs : state.soundLogs;
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
const std::string helpLine = hoverHelp;
const ImVec2 helpTextSize = ImGui::CalcTextSize(helpLine.c_str());
const float helpPadding = 8.0f;
const ImVec2 helpPos = ImVec2(
    ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x - helpTextSize.x - helpPadding,
    ImGui::GetWindowPos().y + ImGui::GetWindowContentRegionMax().y - helpTextSize.y - helpPadding);
ImGui::GetWindowDrawList()->AddText(helpPos, ImGui::GetColorU32(ImGuiCol_TextDisabled), helpLine.c_str());
ImGui::End();
}
