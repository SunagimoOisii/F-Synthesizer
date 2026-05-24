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
        "Playを表示します。",
        "Sound Cardを選んで試聴できます。")
    : (state.UIModeTab == 1)
    ? composeHoverHelp(
        "Composeを表示します。",
        "MIDIとノート編集を扱えます。")
    : (state.UIModeTab == 2)
    ? composeHoverHelp(
        "Exportを表示します。",
        "WAV書き出しに集中できます。")
    : composeHoverHelp(
        "Advancedを表示します。",
        "詳細音色、FX、ミックスを編集できます。");
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
auto preparePlayPresetTarget = [&]()
{
    const int targetCh = state.playEditingChannel;
    if (targetCh < 0 || targetCh > 15)
    {
        return;
    }
    gui::EnsureChannelConfigs(state);
    const int currentSound = std::clamp(state.channelAssignments[targetCh], 0, 15);
    int users = 0;
    for (int ch = 0; ch < 16; ++ch)
    {
        if (std::clamp(state.channelAssignments[ch], 0, 15) == currentSound)
        {
            ++users;
        }
    }
    if (users <= 1)
    {
        state.selectedSoundSlot = currentSound;
        return;
    }

    bool used[16]{};
    for (int ch = 0; ch < 16; ++ch)
    {
        used[std::clamp(state.channelAssignments[ch], 0, 15)] = true;
    }
    int freeSound = -1;
    for (int i = 0; i < 16; ++i)
    {
        if (!used[i])
        {
            freeSound = i;
            break;
        }
    }
    if (freeSound < 0)
    {
        state.selectedSoundSlot = currentSound;
        AppendGUILog(state, "[GUI] Shared sound could not be isolated for " + ChannelLabel(targetCh) + ".");
        return;
    }

    (*state.channelConfigs)[freeSound] = (*state.channelConfigs)[currentSound];
    state.macroSliders[freeSound] = state.macroSliders[currentSound];
    state.channelAssignments[targetCh] = freeSound;
    state.selectedSoundSlot = freeSound;
    state.presetDirty = true;
    AppendGUILog(state, "[GUI] " + ChannelLabel(targetCh) + " sound isolated before preset apply.");
};
auto applyPresetByIndex = [&](int idx)
{
    if (idx < 0 || idx >= static_cast<int>(state.presetItems.size()))
    {
        return;
    }
    preparePlayPresetTarget();
    state.presetIndex = idx;
    std::string err;
    if (ApplySelectedPresetPaths(state, err))
    {
        state.presetDirty = false;
        requestAutoTonePreview();
        AppendGUILog(state, "[GUI] Preset applied: " + state.presetItems[idx] +
            " -> sound " + std::to_string(std::clamp(state.selectedSoundSlot, 0, 15) + 1));
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
    ImGui::TableSetupColumn("left", ImGuiTableColumnFlags_WidthStretch, 0.65f);
    ImGui::TableSetupColumn("right", ImGuiTableColumnFlags_WidthStretch, 0.35f);
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    if (ImGui::BeginTabBar("mode_tabs"))
    {
        const bool needSync = (syncedTab != state.UIModeTab);
        ImGuiTabItemFlags playFlags =
            (needSync && state.UIModeTab == 0) ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
        ImGuiTabItemFlags composeFlags =
            (needSync && state.UIModeTab == 1) ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
        ImGuiTabItemFlags exportFlags =
            (needSync && state.UIModeTab == 2) ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
        ImGuiTabItemFlags advancedFlags =
            (needSync && state.UIModeTab == 3) ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
        if (ImGui::BeginTabItem("Compose", nullptr, composeFlags))
        {
            state.UIModeTab = 1;
            ImGui::EndTabItem();
        }
        updateHoverHelp(
            "Composeへ切り替えます。",
            "MIDI、ピアノロール、ドラム入力を表示します。");
        if (ImGui::BeginTabItem("Play", nullptr, playFlags))
        {
            if (state.UIModeTab == 1)
            {
                const int ch = std::clamp(state.pianoRoll.displayChannel, 0, 15);
                state.playEditingChannel = ch;
                state.selectedSoundSlot = std::clamp(state.channelAssignments[ch], 0, 15);
            }
            else if (state.UIModeTab != 0)
            {
                state.playEditingChannel = -1;
            }
            state.UIModeTab = 0;
            ImGui::EndTabItem();
        }
        updateHoverHelp(
            "Playへ切り替えます。",
            "Sound Card選択と短い試聴を表示します。");
        if (ImGui::BeginTabItem("Export", nullptr, exportFlags))
        {
            state.UIModeTab = 2;
            ImGui::EndTabItem();
        }
        updateHoverHelp(
            "Exportへ切り替えます。",
            "WAV書き出しに特化した操作を表示します。");
        if (ImGui::BeginTabItem("Advanced", nullptr, advancedFlags))
        {
            state.UIModeTab = 3;
            ImGui::EndTabItem();
        }
        updateHoverHelp(
            "Advancedへ切り替えます。",
            "詳細音色、FX、ミックス編集を表示します。");
        ImGui::EndTabBar();
        syncedTab = state.UIModeTab;
    }

    ImGui::TableSetColumnIndex(1);
    const int selectedSlotChip = std::clamp(state.selectedSoundSlot, 0, 15);
    const char* sourceChip = "-";
    if (state.channelConfigs)
    {
        const config::SourceKind sourceKindChip =
            config::SourceConfigKind((*state.channelConfigs)[selectedSlotChip].source);
        sourceChip = config::SourceKindToDisplayName(sourceKindChip);
    }
    if (ImGui::BeginTable("top_header_chips", 5, ImGuiTableFlags_SizingStretchProp))
    {
        static const char* uiThemes[] = { "Light", "Dark" };
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        DrawStatusBadge(state);
        ImGui::TableSetColumnIndex(1);
        ImGui::AlignTextToFramePadding();
        ImGui::Text("表示倍率");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(96.0f);
        const char* uiScales[] = { "100%", "125%", "150%" };
        if (ImGui::Combo("##ui_scale", &state.UIScaleIndex, uiScales, IM_ARRAYSIZE(uiScales)))
        {
            AppendGUILog(state, std::string("[GUI] UI scale changed: ") + UIScaleLabelFromIndex(state.UIScaleIndex));
        }
        updateHoverHelp(
            "UI全体の表示倍率を変更します。",
            "表示倍率だけが変わります。");
        ImGui::TableSetColumnIndex(2);
        ImGui::AlignTextToFramePadding();
        ImGui::Text("テーマ");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(154.0f);
        if (ImGui::Combo("##ui_theme", &state.UIThemeIndex, uiThemes, IM_ARRAYSIZE(uiThemes)))
        {
            AppendGUILog(state, std::string("[GUI] UI mode changed: Blueprint Beat ") + uiThemes[state.UIThemeIndex]);
        }
        updateHoverHelp(
            "Blueprint Beat の表示モードを切り替えます。",
            "Light/Dark の表示モードが切り替わります。");
        ImGui::TableSetColumnIndex(3);
        ImGui::AlignTextToFramePadding();
        if (state.UIModeTab == 0 && state.playEditingChannel >= 0 && state.playEditingChannel < 16)
        {
            ImGui::Text("対象 %s", ChannelLabel(state.playEditingChannel).c_str());
        }
        else
        {
            ImGui::Text("音色 %d", selectedSlotChip + 1);
        }
        ImGui::TableSetColumnIndex(4);
        ImGui::AlignTextToFramePadding();
        ImGui::Text("音源 %s", sourceChip);
        ImGui::EndTable();
    }
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
    RefreshPresetItems(state, state.presetName);
    lastFrameTab = state.UIModeTab;
}
ImGui::Separator();
if (state.hasUIError)
{
    auto suggestedFix = [&]() -> const char*
    {
        switch (state.UIErrorAction)
        {
        case 1: return "対処案: MIDIパスを選び直す";
        case 2: return "対処案: 出力パスを選び直す";
        case 3: return "対処案: Advancedで設定を確認する";
        case 4: return "対処案: Composeで設定を確認する";
        default: return "対処案: 設定を見直して再実行する";
        }
    };
    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.95f, 0.35f, 0.35f, 1.0f), "問題: %s", state.UIErrorMessage.c_str());
    ImGui::TextDisabled("%s", suggestedFix());
    if (state.UIErrorAction == 1)
    {
        ImGui::SameLine();
        if (ImGui::Button("復旧: MIDIを選び直す"))
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
            "復旧: MIDIを選び直す を実行します。",
            "MIDI Pathを再選択して復旧します。");
    }
    else if (state.UIErrorAction == 2)
    {
        ImGui::SameLine();
        if (ImGui::Button("復旧: 出力先を選び直す"))
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
            "復旧: 出力先を選び直す を実行します。",
            "Output Pathを再選択して復旧します。");
    }
    else if (state.UIErrorAction == 3)
    {
        ImGui::SameLine();
        if (ImGui::Button("復旧: Advancedへ移動"))
        {
            state.UIModeTab = 3;
            ClearGUIError(state);
        }
        updateHoverHelp(
            "復旧: Advancedへ移動 を実行します。",
            "詳細設定へ移動します。");
    }
    else if (state.UIErrorAction == 4)
    {
        ImGui::SameLine();
        if (ImGui::Button("復旧: Composeへ移動"))
        {
            state.UIModeTab = 1;
            ClearGUIError(state);
        }
        updateHoverHelp(
            "復旧: Composeへ移動 を実行します。",
            "Composeへ移動します。");
    }
    ImGui::SameLine();
    if (ImGui::Button("エラーをクリア"))
    {
        ClearGUIError(state);
    }
    updateHoverHelp(
        "エラーをクリア を実行します。",
        "エラー表示をクリアします。",
        "原因が未解決の場合は再実行時に再発します。");
}
if (state.showErrorDialog)
{
    ImGui::OpenPopup("エラー");
    state.showErrorDialog = false;
}
if (ImGui::BeginPopupModal("エラー", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
{
    ImGui::TextWrapped("問題: %s", state.UIErrorMessage.c_str());
    switch (state.UIErrorAction)
    {
    case 1: ImGui::TextDisabled("対処案: MIDIパスを選び直してください。"); break;
    case 2: ImGui::TextDisabled("対処案: 出力パスを選び直してください。"); break;
    case 3: ImGui::TextDisabled("対処案: Advancedの設定を確認してください。"); break;
    case 4: ImGui::TextDisabled("対処案: Composeの設定を確認してください。"); break;
    default: ImGui::TextDisabled("対処案: 設定を見直して再実行してください。"); break;
    }
    if (ImGui::Button("OK"))
    {
        ImGui::CloseCurrentPopup();
    }
    updateHoverHelp(
        "Errorダイアログを閉じます。",
        "表示のみ閉じ、エラー状態は維持します。");
    ImGui::SameLine();
    if (ImGui::Button("状態をクリア"))
    {
        ClearGUIError(state);
        ImGui::CloseCurrentPopup();
    }
    updateHoverHelp(
        "状態をクリア を実行します。",
        "エラー状態をクリアして閉じます。");
    ImGui::EndPopup();
}
ImGui::Separator();
if (ImGui::BeginTable("sound_action_bar", 3, ImGuiTableFlags_SizingStretchProp))
{
    ImGui::TableSetupColumn("save", ImGuiTableColumnFlags_WidthStretch, 0.48f);
    ImGui::TableSetupColumn("primary", ImGuiTableColumnFlags_WidthStretch, 0.32f);
    ImGui::TableSetupColumn("aux", ImGuiTableColumnFlags_WidthStretch, 0.20f);
    ImGui::TableNextRow();

    ImGui::TableSetColumnIndex(0);
    ImGui::BeginDisabled(state.running);
    if (ImGui::Button("プロジェクト保存"))
    {
        if (saveWorkspaceOnly())
        {
            AppendGUILog(state, "[GUI] Save Project: MusicProject + Workspace");
        }
    }
    updateHoverHelp(
        "プロジェクト保存を実行します。",
        "MusicProjectとWorkspaceを保存します。",
        "SoundAsset(Preset)は保存対象に含みません。");
    ImGui::SameLine();
    if (ImGui::Button("すべて保存"))
    {
        saveAll();
    }
    updateHoverHelp(
        "すべて保存を実行します。",
        "SoundAsset/MusicProject/Workspaceを保存します。");
    ImGui::EndDisabled();

    ImGui::TableSetColumnIndex(1);
    ImGui::BeginDisabled(state.running);
    if (state.UIModeTab == 0)
    {
        if (ImGui::Button("試聴"))
        {
            clearAutoTonePreviewRequest();
            StartGUIRun(state, true);
        }
        updateHoverHelp(
            "試聴を実行します。",
            "表示中の音色で再生成して再生します。",
            "WAVファイルは出力しません。");
    }
    else if (state.UIModeTab == 1)
    {
        if (ImGui::Button("書き出し"))
        {
            StartGUIRun(state, false);
        }
        updateHoverHelp(
            "書き出しを実行します。",
            "現在設定でWAVを書き出します。",
            "再生中の場合は停止してから書き出しを開始します。");
        ImGui::SameLine();
        if (ImGui::Button("試聴"))
        {
            StartGUIRun(state, true);
        }
        updateHoverHelp(
            "試聴を実行します。",
            "表示chを現在の割当/ミックスで再生します。",
            "WAVファイルは出力しません。");
    }
    else
    {
        if (ImGui::Button("書き出し"))
        {
            StartGUIRun(state, false);
        }
        updateHoverHelp(
            "書き出しを実行します。",
            "現在設定でWAVを書き出します。",
            "再生中の場合は停止してから書き出しを開始します。");
        ImGui::SameLine();
        if (ImGui::Button("試聴"))
        {
            StartGUIRun(state, true);
        }
        updateHoverHelp(
            "試聴を実行します。",
            "現在設定でプレビュー再生します。",
            "WAVファイルは出力しません。");
    }
    ImGui::EndDisabled();

    ImGui::TableSetColumnIndex(2);
    ImGui::BeginDisabled(state.running);
    if (state.UIModeTab == 0)
    {
        if (ImGui::Checkbox("自動", &state.autoTonePreviewEnabled))
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
    }
    ImGui::Checkbox("ループ", &state.previewLoop);
    updateHoverHelp(
        "Loop Previewを切り替えます。",
        "Preview終了後に先頭からループ再生します。");
    ImGui::EndDisabled();

    ImGui::SameLine();
    const bool canStop = state.running || state.playback.playing.load(std::memory_order_relaxed);
    ImGui::BeginDisabled(!canStop);
    if (ImGui::Button("停止"))
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
    if (ImGui::Button("閉じる"))
    {
        if (state.presetDirty)
        {
            pendingCloseRequest = true;
            ImGui::OpenPopup("未保存の変更");
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
    ImGui::EndTable();
}
ImGui::Separator();
if (openUnsavedPopupNextFrame)
{
    ImGui::OpenPopup("未保存の変更");
    openUnsavedPopupNextFrame = false;
}
if (ImGui::BeginPopupModal("未保存の変更", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
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
const ImGuiWindowFlags bodyPanelFlags = (state.UIModeTab == 1)
    ? (ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)
    : ImGuiWindowFlags_None;
ImGui::BeginChild("body_panel", ImVec2(0, bodyHeight), true, bodyPanelFlags);
if (state.UIModeTab == 0)
{
    DrawPlayView(
        state,
        pendingPresetIndex,
        pendingPresetOriginalIndex,
        openUnsavedPopupNextFrame,
        applyPresetByIndex,
        [&](const char* what, const char* impact, const char* caution)
        {
            updateHoverHelp(what, impact, caution);
        });
}
else if (state.UIModeTab == 1)
{
    DrawComposeView(
        state,
        [&](const char* what, const char* impact, const char* caution)
        {
            updateHoverHelp(what, impact, caution);
        });
}
else if (state.UIModeTab == 2)
{
    DrawExportView(state, updateHoverHelp);
}
else
{
    DrawAdvancedView(
        state,
        applyPresetByIndex,
        [&](const char* what, const char* impact, const char* caution)
        {
            updateHoverHelp(what, impact, caution);
        },
        requestAutoTonePreview);
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
logPanelExpanded = ImGui::CollapsingHeader("ログ");
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
