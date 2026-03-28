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
    : (state.UIModeTab == 2)
    ? composeHoverHelp(
        "Exportモードを表示します。",
        "WAV書き出しに特化した操作を行えます。")
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
constexpr double kAutoTonePreviewDebounceSec = 0.4;
auto requestAutoTonePreview = [&]()
{
    if (!state.autoTonePreviewEnabled)
    {
        return;
    }
    state.autoTonePreviewPending = true;
    state.autoTonePreviewLastEditSec = ImGui::GetTime();
};
auto clearAutoTonePreviewRequest = [&]()
{
    state.autoTonePreviewPending = false;
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
        requestAutoTonePreview();
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
        ImGuiTabItemFlags exportFlags =
            (needSync && state.UIModeTab == 2) ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
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
            "MIDIとミックス設定の操作を表示します。");
        if (ImGui::BeginTabItem("Export", nullptr, exportFlags))
        {
            state.UIModeTab = 2;
            ImGui::EndTabItem();
        }
        updateHoverHelp(
            "Exportモードへ切り替えます。",
            "WAV書き出しに特化した操作を表示します。");
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
    clearAutoTonePreviewRequest();
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
        clearAutoTonePreviewRequest();
        StartGUIRun(state, true);
    }
    updateHoverHelp(
        "Play Previewを実行します。",
        "表示chを選択中Slotで再生成して再生します。",
        "WAVファイルは出力しません。");
    ImGui::SameLine();
    if (ImGui::Checkbox("Auto Tone Preview", &state.autoTonePreviewEnabled))
    {
        if (!state.autoTonePreviewEnabled)
        {
            clearAutoTonePreviewRequest();
        }
    }
    updateHoverHelp(
        "Auto Tone Preview を切り替えます。",
        "Sound編集後、400ms無操作で単音試聴を自動実行します。",
        "重い環境ではOFFにして手動Previewを使ってください。");
    ImGui::SameLine();
    ImGui::Checkbox("Loop Preview", &state.previewLoop);
    updateHoverHelp(
        "Loop Previewを切り替えます。",
        "Preview終了後に先頭からループ再生します。");
}
else if (state.UIModeTab == 1)
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
    if (ImGui::Button("Play Preview"))
    {
        StartGUIRun(state, true);
    }
    updateHoverHelp(
        "Play Preview を実行します。",
        "現在設定でプレビュー再生します。",
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
    // Ctrl+Z / Ctrl+Y: Sound タブ専用 Undo/Redo
    // ピアノロールにフォーカスがない場合のみ Sound Undo を消費する
    if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
        !state.running)
    {
        if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Z))
        {
            if (UndoSound(state))
            {
                requestAutoTonePreview();
            }
        }
        if (ImGui::IsKeyChordPressed(ImGuiMod_Ctrl | ImGuiKey_Y))
        {
            if (RedoSound(state))
            {
                requestAutoTonePreview();
            }
        }
    }

    if (ImGui::BeginTable("layout_split", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
    {
        ImGui::TableSetupColumn("left", ImGuiTableColumnFlags_WidthStretch, 0.56f);
        ImGui::TableSetupColumn("right", ImGuiTableColumnFlags_WidthStretch, 0.44f);
        ImGui::TableNextRow();

        ImGui::TableSetColumnIndex(0);
        gui::EnsureChannelConfigs(state);
        state.selectedSoundSlot = std::clamp(state.selectedSoundSlot, 0, 15);
        ChannelConfig& selectedSlotCfg = (*state.channelConfigs)[state.selectedSoundSlot];
        auto buildGuiSourceKinds = [](std::array<config::SourceKind, config::kSourceKindCount>& kinds, size_t& count)
        {
            kinds = {};
            count = 0;
            for (int i = 0; i < config::kSourceKindCount; i++)
            {
                const config::SourceKind kind = config::SourceKindFromIndex(i);
                if (kind == config::SourceKind::Count)
                {
                    continue;
                }
                const config::SourceCapability capability = config::SourceCapabilityOf(kind);
                if (!capability.isPercussion || kind == config::SourceKind::DrumKit)
                {
                    kinds[count++] = kind;
                }
            }
            if (count == 0)
            {
                kinds[count++] = config::SourceKind::Waveform;
            }
        };
        std::array<config::SourceKind, config::kSourceKindCount> guiKinds{};
        size_t guiKindCount = 0;
        buildGuiSourceKinds(guiKinds, guiKindCount);
        const config::SourceKind currentKind = config::SourceConfigKind(selectedSlotCfg.source);
        int sourceKindUiIndex = 0;
        for (size_t i = 0; i < guiKindCount; i++)
        {
            if (guiKinds[i] == currentKind)
            {
                sourceKindUiIndex = static_cast<int>(i);
                break;
            }
        }

        static int lastPresetFilterKey = -1;
        const int filterKey = std::clamp(state.selectedSoundSlot, 0, 15) * 100 +
            config::SourceKindToIndex(currentKind);
        if (filterKey != lastPresetFilterKey)
        {
            RefreshPresetItems(state, state.presetName);
            lastPresetFilterKey = filterKey;
        }

        if (state.presetDirty)
        {
            ImGui::TextColored(ImVec4(0.95f, 0.75f, 0.2f, 1.0f), "Preset: modified (unsaved)");
        }
        else
        {
            ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.0f), "Preset: saved");
        }
        ImGui::BeginDisabled(state.running);
        if (ImGui::BeginCombo("Source Type", config::SourceKindToDisplayName(guiKinds[static_cast<size_t>(sourceKindUiIndex)])))
        {
            for (size_t i = 0; i < guiKindCount; i++)
            {
                const config::SourceKind candidate = guiKinds[i];
                const bool selected = (sourceKindUiIndex == static_cast<int>(i));
                if (ImGui::Selectable(config::SourceKindToDisplayName(candidate), selected))
                {
                    sourceKindUiIndex = static_cast<int>(i);
                    selectedSlotCfg.source = gui::DefaultSourceByType(config::SourceKindToIndex(candidate));
                    state.presetDirty = true;
                    requestAutoTonePreview();
                    RefreshPresetItems(state, state.presetName);
                    AppendGUILog(state, std::string("[GUI] Source type changed (preset scope): ") +
                        config::SourceKindToTypeName(candidate) +
                        " @ slot s" + std::to_string(std::clamp(state.selectedSoundSlot, 0, 15)));
                }
                if (selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        updateHoverHelp(
            "Source Type を選択します。",
            "Sound右ペインの編集対象とPreset一覧の表示対象を切り替えます。",
            "切替時は選択中スロットを該当sourceTypeの初期値で再初期化し、Layer1のプリセット一覧も更新されます。");

        ImGui::SameLine();
        if (ImGui::Button("Reset Defaults"))
        {
            ClearGUIError(state);
            InitializeGUIState(state, [&](const std::string& preferName) { RefreshPresetItems(state, preferName); });
            state.presetDirty = false;
            clearAutoTonePreviewRequest();
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
                requestAutoTonePreview();
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

        // DrumKit 以外の音源では Tone Preview ノートを手動選択する。
        {
            bool isDrumSlot = false;
            if (state.channelConfigs)
            {
                const int slot = std::clamp(state.selectedSoundSlot, 0, 15);
                isDrumSlot = std::holds_alternative<DrumKitConfig>((*state.channelConfigs)[slot].source);
            }

            if (!isDrumSlot)
            {
                ImGui::SetNextItemWidth(160.0f);
                if (ImGui::SliderInt("Preview Note", &state.tonePreviewNoteNumber, 24, 96))
                {
                    state.tonePreviewNoteNumber = std::clamp(state.tonePreviewNoteNumber, 0, 127);
                    requestAutoTonePreview();
                }
                updateHoverHelp(
                    "Tone Preview の再生音程を変更します。",
                    "C2(24)〜C6(84) 付近の範囲で MIDI ノート番号を指定できます。");
                ImGui::SameLine();
                constexpr const char* kNoteNames[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
                const int noteNumber = std::clamp(state.tonePreviewNoteNumber, 0, 127);
                const int oct = noteNumber / 12 - 1;
                ImGui::TextDisabled("(%s%d)", kNoteNames[noteNumber % 12], oct);
            }
        }

        // Tone Preview 波形ビューア: 再生中は frameCursor を窓の中心に追従し、
        // エンベロープの振幅変化（アタック/サステイン/リリース）がそのまま波高に反映される。
        if (state.previewRenderedSound)
        {
            const auto& src = state.previewRenderedSound->dataL.empty()
                ? state.previewRenderedSound->data
                : state.previewRenderedSound->dataL;
            if (!src.empty())
            {
                constexpr int kBufSize = 256;
                constexpr int kWindowSamples = 2048;
                static float wfBuf[kBufSize];
                const int srcSize = static_cast<int>(src.size());

                // 再生中: cursor を窓の中心に / 停止中: 先頭固定
                int windowStart = 0;
                if (state.playback.playing.load(std::memory_order_relaxed))
                {
                    const int cursor = static_cast<int>(
                        state.playback.frameCursor.load(std::memory_order_relaxed));
                    windowStart = std::clamp(cursor - kWindowSamples / 2, 0,
                                             std::max(0, srcSize - kWindowSamples));
                }
                const int total = std::min(srcSize - windowStart, kWindowSamples);

                for (int i = 0; i < kBufSize; i++)
                {
                    const int idx = windowStart + std::clamp(i * total / kBufSize, 0, total - 1);
                    wfBuf[i] = static_cast<float>(src[idx]);
                }
                ImGui::Separator();
                ImGui::TextDisabled("Waveform (Tone Preview)");
                ImGui::PlotLines("##wf", wfBuf, kBufSize, 0, nullptr, -1.0f, 1.0f, ImVec2(-1.0f, 64.0f));
                // --- Spectrum ---
                {
                    constexpr int kFftSize = 256;
                    constexpr int kSpecBins = 128;
                    static float specBuf[kSpecBins];
                    static float timeBuf[kFftSize];

                    std::fill(specBuf, specBuf + kSpecBins, 0.0f);
                    const int fftLen = std::min(kFftSize, total);
                    if (fftLen >= 2)
                    {
                        constexpr double kPi = 3.14159265358979323846;
                        for (int n = 0; n < fftLen; ++n)
                        {
                            const double w = 0.5 * (1.0 - std::cos(2.0 * kPi * n / (fftLen - 1))); // Hann
                            timeBuf[n] = static_cast<float>(src[windowStart + n] * w);
                        }

                        const int halfLen = fftLen / 2;
                        for (int k = 0; k < halfLen; ++k)
                        {
                            double re = 0.0;
                            double im = 0.0;
                            for (int n = 0; n < fftLen; ++n)
                            {
                                const double phase = 2.0 * kPi * k * n / fftLen;
                                re += timeBuf[n] * std::cos(phase);
                                im -= timeBuf[n] * std::sin(phase);
                            }

                            const float mag = static_cast<float>(std::sqrt(re * re + im * im));
                            const int bin = std::clamp(k * kSpecBins / std::max(1, halfLen), 0, kSpecBins - 1);
                            specBuf[bin] = std::max(specBuf[bin], mag);
                        }

                        const float specMax = *std::max_element(specBuf, specBuf + kSpecBins);
                        if (specMax > 0.0f)
                        {
                            for (int b = 0; b < kSpecBins; ++b)
                            {
                                specBuf[b] /= specMax;
                            }
                        }
                    }

                    ImGui::TextDisabled("Spectrum (Tone Preview)");
                    ImGui::PlotHistogram("##spec", specBuf, kSpecBins, 0, nullptr, 0.0f, 1.0f, ImVec2(-1.0f, 48.0f));
                }
            }
        }
        DrawVUMeter(state);

        ImGui::TableSetColumnIndex(1);
        DrawLayer1Discovery(
            state,
            pendingPresetIndex,
            pendingPresetOriginalIndex,
            openUnsavedPopupNextFrame,
            applyPresetByIndex);
        ImGui::Separator();
        DrawLayer2Macros(state);
        ImGui::Separator();
        const bool channelEditorChanged = DrawChannelEditor(
            state,
            false,
            [&](const char* what, const char* impact, const char* caution)
            {
                updateHoverHelp(what, impact, caution);
            });
        state.presetDirty |= channelEditorChanged;
        if (channelEditorChanged)
        {
            requestAutoTonePreview();
        }
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
    // テーブル終了後、全幅で仮想キーボードを描画
    DrawVirtualKeyboard(state);
}
else if (state.UIModeTab == 1)
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
    ImGui::TextUnformatted("Master Effects");
    ImGui::TextDisabled("Order: SampleRateReducer -> BitCrusher -> Chorus -> Flanger -> Delay -> Reverb");
    {
        struct FxBlockDef
        {
            const char* label;
        };
        static constexpr FxBlockDef kFxBlocks[6] = {
            { "SampleRateReducer" },
            { "BitCrusher" },
            { "Chorus" },
            { "Flanger" },
            { "Delay" },
            { "Reverb" },
        };
        // ratio == 1.0 がバイパス仕様（SynthEngine.h "1.0でバイパス"）
        // UI では浮動小数点誤差を吸収するため 0.999 を閾値として使用する
        constexpr double kSrBypass = 0.999;
        auto fxEnabled = [&](int idx) -> bool
        {
            switch (idx)
            {
            case 0: return state.masterEffects.sampleRateReducer.ratio < kSrBypass;
            case 1: return state.masterEffects.bitCrusher.bits < 16;
            case 2: return state.masterEffects.chorus.enabled;
            case 3: return state.masterEffects.flanger.enabled;
            case 4: return state.masterEffects.delay.enabled;
            case 5: return state.masterEffects.reverb.enabled;
            default: return false;
            }
        };
        auto toggleFx = [&](int idx)
        {
            switch (idx)
            {
            case 0:
                // ON 時は 0.5 (UI 初期値として中間値を選択)、OFF 時は 1.0 (バイパス)
                state.masterEffects.sampleRateReducer.ratio =
                    (state.masterEffects.sampleRateReducer.ratio < kSrBypass) ? 1.0 : 0.5;
                break;
            case 1:
                // ON 時は 8bit (視聴しやすい中間値)、OFF 時は 16bit (バイパス)
                state.masterEffects.bitCrusher.bits =
                    (state.masterEffects.bitCrusher.bits < 16) ? 16 : 8;
                break;
            case 2: state.masterEffects.chorus.enabled = !state.masterEffects.chorus.enabled; break;
            case 3: state.masterEffects.flanger.enabled = !state.masterEffects.flanger.enabled; break;
            case 4: state.masterEffects.delay.enabled = !state.masterEffects.delay.enabled; break;
            case 5: state.masterEffects.reverb.enabled = !state.masterEffects.reverb.enabled; break;
            default: break;
            }
            state.presetDirty = true;
        };

        constexpr float kBlockW = 132.0f;
        constexpr float kBlockH = 38.0f;
        constexpr float kGap = 10.0f;
        const float totalWidth = (kBlockW * 6.0f) + (kGap * 5.0f);
        const float availWidth = ImGui::GetContentRegionAvail().x;
        const float startX = ImGui::GetCursorPosX() + (std::max)(0.0f, (availWidth - totalWidth) * 0.5f);
        const ImVec2 startPos = ImVec2(startX, ImGui::GetCursorPosY());
        ImDrawList* dl = ImGui::GetWindowDrawList();

        // ラベル幅は静的文字列なので一度だけ計算する
        ImVec2 labelSizes[6];
        for (int i = 0; i < 6; ++i) { labelSizes[i] = ImGui::CalcTextSize(kFxBlocks[i].label); }
        const ImVec2 onSize  = ImGui::CalcTextSize("ON");
        const ImVec2 offSize = ImGui::CalcTextSize("OFF");

        for (int i = 0; i < 6; ++i)
        {
            const ImVec2 p0 = ImVec2(startPos.x + i * (kBlockW + kGap), startPos.y);
            const ImVec2 p1 = ImVec2(p0.x + kBlockW, p0.y + kBlockH);
            const bool on = fxEnabled(i);
            const ImU32 fill = on ? IM_COL32(46, 126, 74, 235) : IM_COL32(70, 70, 70, 220);
            const ImU32 border = on ? IM_COL32(92, 195, 126, 255) : IM_COL32(118, 118, 118, 255);
            const ImU32 text = on ? IM_COL32(242, 255, 246, 255) : IM_COL32(212, 212, 212, 255);

            ImGui::SetCursorScreenPos(p0);
            ImGui::PushID(i + 7000);
            ImGui::InvisibleButton("fx_chain_block", ImVec2(kBlockW, kBlockH));
            if (ImGui::IsItemClicked())
            {
                toggleFx(i);
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s: %s", kFxBlocks[i].label, on ? "ON" : "OFF");
                updateHoverHelp(
                    "エフェクトブロックを切り替えます。",
                    "クリックしたエフェクトのON/OFFを切り替えます。",
                    "処理順は左から右へ固定です。");
            }
            ImGui::PopID();

            dl->AddRectFilled(p0, p1, fill, 6.0f);
            dl->AddRect(p0, p1, border, 6.0f, 0, 2.0f);
            const char* status = on ? "ON" : "OFF";
            const ImVec2& labelSize = labelSizes[i];
            const ImVec2& statusSize = on ? onSize : offSize;
            dl->AddText(
                ImVec2(p0.x + (kBlockW - labelSize.x) * 0.5f, p0.y + 6.0f),
                text,
                kFxBlocks[i].label);
            dl->AddText(
                ImVec2(p0.x + (kBlockW - statusSize.x) * 0.5f, p0.y + 20.0f),
                text,
                status);

            if (i < 5)
            {
                const ImVec2 a = ImVec2(p1.x + 2.0f, p0.y + kBlockH * 0.5f);
                const ImVec2 b = ImVec2(p1.x + kGap - 3.0f, p0.y + kBlockH * 0.5f);
                dl->AddLine(a, b, IM_COL32(150, 150, 150, 210), 2.0f);
                dl->AddTriangleFilled(
                    ImVec2(b.x, b.y),
                    ImVec2(b.x - 5.0f, b.y - 4.0f),
                    ImVec2(b.x - 5.0f, b.y + 4.0f),
                    IM_COL32(150, 150, 150, 210));
            }
        }
        ImGui::SetCursorScreenPos(startPos);
        ImGui::Dummy(ImVec2(totalWidth, kBlockH + 8.0f));
    }
    const auto sliderFxDouble = [&](const char* label, double& value, float minV, float maxV, const char* fmt) -> bool
    {
        float v = static_cast<float>(value);
        const bool edited = ImGui::SliderFloat(label, &v, minV, maxV, fmt);
        if (edited)
        {
            value = static_cast<double>(v);
        }
        return edited;
    };
    if (ImGui::CollapsingHeader("Retro FX", ImGuiTreeNodeFlags_DefaultOpen))
    {
        int bits = std::clamp(state.masterEffects.bitCrusher.bits, 1, 16);
        ImGui::SetNextItemWidth(260.0f);
        if (ImGui::SliderInt("BitCrusher Bits", &bits, 1, 16))
        {
            state.masterEffects.bitCrusher.bits = bits;
            state.presetDirty = true;
        }
        updateHoverHelp(
            "ビットクラッシャーの量子化ビット数を調整します。",
            "小さいほどザラついたレトロ感が強くなります。",
            "16でバイパスされます。");

        ImGui::SetNextItemWidth(260.0f);
        if (sliderFxDouble("SampleRateReducer Ratio", state.masterEffects.sampleRateReducer.ratio, 0.0f, 1.0f, "%.3f"))
        {
            state.presetDirty = true;
        }
        updateHoverHelp(
            "サンプル保持比率を調整します。",
            "小さいほど粗いダウンサンプリング感になります。",
            "1.0でバイパスされます。");
    }

    if (ImGui::CollapsingHeader("Chorus"))
    {
        state.presetDirty |= ImGui::Checkbox("Chorus Enabled", &state.masterEffects.chorus.enabled);
        updateHoverHelp(
            "コーラスの有効/無効を切り替えます。",
            "厚みと揺らぎを加えます。",
            "深くしすぎるとピッチ感がぼやけます。");
        ImGui::SetNextItemWidth(260.0f);
        if (sliderFxDouble("Chorus Mix", state.masterEffects.chorus.mix, 0.0f, 1.0f, "%.3f")) state.presetDirty = true;
        ImGui::SetNextItemWidth(260.0f);
        if (sliderFxDouble("Chorus Base Delay (ms)", state.masterEffects.chorus.baseDelayMs, 2.0f, 40.0f, "%.2f")) state.presetDirty = true;
        ImGui::SetNextItemWidth(260.0f);
        if (sliderFxDouble("Chorus Depth (ms)", state.masterEffects.chorus.depthMs, 0.0f, 20.0f, "%.2f")) state.presetDirty = true;
        ImGui::SetNextItemWidth(260.0f);
        if (sliderFxDouble("Chorus Rate (Hz)", state.masterEffects.chorus.rateHz, 0.05f, 8.0f, "%.2f")) state.presetDirty = true;
        ImGui::SetNextItemWidth(260.0f);
        if (sliderFxDouble("Chorus Feedback", state.masterEffects.chorus.feedback, 0.0f, 0.9f, "%.3f")) state.presetDirty = true;
    }

    if (ImGui::CollapsingHeader("Flanger"))
    {
        state.presetDirty |= ImGui::Checkbox("Flanger Enabled", &state.masterEffects.flanger.enabled);
        updateHoverHelp(
            "フランジャーの有効/無効を切り替えます。",
            "短ディレイの周期変調でジェット感を加えます。",
            "Feedbackを上げすぎると耳障りになりやすいです。");
        ImGui::SetNextItemWidth(260.0f);
        if (sliderFxDouble("Flanger Mix", state.masterEffects.flanger.mix, 0.0f, 1.0f, "%.3f")) state.presetDirty = true;
        ImGui::SetNextItemWidth(260.0f);
        if (sliderFxDouble("Flanger Base Delay (ms)", state.masterEffects.flanger.baseDelayMs, 0.1f, 8.0f, "%.2f")) state.presetDirty = true;
        ImGui::SetNextItemWidth(260.0f);
        if (sliderFxDouble("Flanger Depth (ms)", state.masterEffects.flanger.depthMs, 0.0f, 5.0f, "%.2f")) state.presetDirty = true;
        ImGui::SetNextItemWidth(260.0f);
        if (sliderFxDouble("Flanger Rate (Hz)", state.masterEffects.flanger.rateHz, 0.05f, 8.0f, "%.2f")) state.presetDirty = true;
        ImGui::SetNextItemWidth(260.0f);
        if (sliderFxDouble("Flanger Feedback", state.masterEffects.flanger.feedback, 0.0f, 0.95f, "%.3f")) state.presetDirty = true;
    }

    if (ImGui::CollapsingHeader("Delay"))
    {
        state.presetDirty |= ImGui::Checkbox("Delay Enabled", &state.masterEffects.delay.enabled);
        updateHoverHelp(
            "ディレイの有効/無効を切り替えます。",
            "反復エコーを加えます。",
            "Feedbackを上げすぎると濁りやすくなります。");
        ImGui::SetNextItemWidth(260.0f);
        if (sliderFxDouble("Delay Mix", state.masterEffects.delay.mix, 0.0f, 1.0f, "%.3f")) state.presetDirty = true;
        ImGui::SetNextItemWidth(260.0f);
        if (sliderFxDouble("Delay Feedback", state.masterEffects.delay.feedback, 0.0f, 0.95f, "%.3f")) state.presetDirty = true;
        state.presetDirty |= ImGui::Checkbox("Delay Tempo Sync", &state.masterEffects.delay.tempoSync);
        if (state.masterEffects.delay.tempoSync)
        {
            ImGui::SetNextItemWidth(260.0f);
            if (sliderFxDouble("Delay Sync Beats", state.masterEffects.delay.syncBeats, 0.125f, 4.0f, "%.3f")) state.presetDirty = true;
        }
        else
        {
            ImGui::SetNextItemWidth(260.0f);
            if (sliderFxDouble("Delay Time (sec)", state.masterEffects.delay.timeSec, 0.01f, 2.0f, "%.3f")) state.presetDirty = true;
        }
    }

    if (ImGui::CollapsingHeader("Reverb"))
    {
        state.presetDirty |= ImGui::Checkbox("Reverb Enabled", &state.masterEffects.reverb.enabled);
        updateHoverHelp(
            "リバーブの有効/無効を切り替えます。",
            "残響感と奥行きを加えます。",
            "Mixを上げるほど原音の輪郭が薄くなります。");
        ImGui::SetNextItemWidth(260.0f);
        if (sliderFxDouble("Reverb Mix", state.masterEffects.reverb.mix, 0.0f, 1.0f, "%.3f")) state.presetDirty = true;
        ImGui::SetNextItemWidth(260.0f);
        if (sliderFxDouble("Reverb Room Size", state.masterEffects.reverb.roomSize, 0.1f, 1.0f, "%.3f")) state.presetDirty = true;
        ImGui::SetNextItemWidth(260.0f);
        if (sliderFxDouble("Reverb Damping", state.masterEffects.reverb.damping, 0.0f, 1.0f, "%.3f")) state.presetDirty = true;
    }

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
        const bool isDrumSource = config::SourceCapabilityOf(drumCh.source).isPercussion;
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
                const bool mappedToDrum = config::SourceCapabilityOf(srcCfg).isPercussion;
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
    const bool isShowingDrumCh =
        (state.pianoRoll.displayChannel == drumMidiChannel) && state.drumChannelSpecialHandling;
    if (isShowingDrumCh)
    {
        const bool prSelected = !state.stepSeq.viewActive;
        if (prSelected)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(50, 90, 170, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(70, 110, 200, 255));
        }
        if (ImGui::Button("Piano Roll"))
        {
            if (state.stepSeq.viewActive)
            {
                state.stepSeq.viewActive = false;
            }
        }
        if (prSelected)
        {
            ImGui::PopStyleColor(2);
        }
        updateHoverHelp(
            "Piano Roll ビューへ切り替えます。",
            "通常のピアノロールが表示されます。");

        ImGui::SameLine();

        const bool ssSelected = state.stepSeq.viewActive;
        if (ssSelected)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(50, 90, 170, 255));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(70, 110, 200, 255));
        }
        if (ImGui::Button("Step Seq"))
        {
            if (!state.stepSeq.viewActive)
            {
                LoadStepSeqFromPianoRoll(state.stepSeq, state.pianoRoll);
                state.stepSeq.viewActive = true;
            }
        }
        if (ssSelected)
        {
            ImGui::PopStyleColor(2);
        }
        updateHoverHelp(
            "Step Seq ビューへ切り替えます。",
            "16ステップのドラムグリッドが表示されます。");
    }

    if (isShowingDrumCh && state.stepSeq.viewActive)
    {
        DrawStepSeqPanel(state);
    }
    else
    {
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
}
else
{
    DrawExportView(state, updateHoverHelp);
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
        const std::vector<std::string>& visibleLogs = (state.UIModeTab == 1)
            ? state.musicLogs
            : (state.UIModeTab == 2)
            ? state.exportLogs
            : state.soundLogs;
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
if (state.autoTonePreviewEnabled
    && state.autoTonePreviewPending
    && state.UIModeTab == 0
    && !state.running
    && !state.playback.playing.load(std::memory_order_relaxed))
{
    if ((ImGui::GetTime() - state.autoTonePreviewLastEditSec) >= kAutoTonePreviewDebounceSec)
    {
        clearAutoTonePreviewRequest();
        StartGUISoundTonePreview(state);
    }
}
ImGui::End();
}
