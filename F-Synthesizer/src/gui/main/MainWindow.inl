void DrawMainWindowFrame(GUIState& state, GLFWwindow*, int& lastFrameTab)
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::Begin("F-Synthesizer", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
    auto help = [](const char*, const char* detail, const char* = nullptr)
    {
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", detail);
    };
    auto requestPreview = [&]()
    {
        if (state.autoTonePreviewEnabled && !state.running)
        { state.autoTonePreviewPending = true; state.autoTonePreviewLastEditSec = ImGui::GetTime(); }
    };
    auto saveSong = [&](bool saveAs) -> bool
    {
        std::string path = state.activeProjectPath;
        if (saveAs || path.empty())
        {
            const wchar_t* filter = L"F-Synthesizer Song (*.fsynth)\0*.fsynth\0";
            if (!BrowseSavePath(path.c_str(), filter, L"fsynth", path)) return false;
            if (Utf8ToPath(path).extension() != ".fsynth") path += ".fsynth";
        }
        std::string error;
        if (!gui::SaveSongProjectFile(state, Utf8ToPath(path), error))
        { RaiseGUIError(state, error, 0, true); return false; }
        state.observedNotesVersion = state.pianoRoll.notesVersion;
        AppendGUILog(state, "曲を保存しました: " + path);
        return true;
    };
    auto openPending = [&]()
    {
        std::string error;
        bool success = false;
        if (state.pendingOpenIsSong)
            success = gui::LoadSongProjectFile(state, Utf8ToPath(state.pendingOpenPath), error);
        else
        {
            gui::PianoRollState piano;
            success = gui::LoadPianoRollMIDI(piano, Utf8ToPath(state.pendingOpenPath));
            if (success)
            {
                StopPreviewAudio(state.playback);
                state.pianoRoll = std::move(piano);
                strncpy_s(state.midiPath, sizeof(state.midiPath), state.pendingOpenPath.c_str(), _TRUNCATE);
                state.songMidiName = PathToUtf8(Utf8ToPath(state.pendingOpenPath).filename());
                state.activeProjectPath.clear();
                state.stepSeq = GUIStepSeqState{};
                state.presetDirty = true;
            }
            else error = piano.lastError;
        }
        state.pendingOpenPath.clear();
        if (!success) RaiseGUIError(state, error, 0, true);
        else
        {
            state.UIModeTab = 1;
            state.observedNotesVersion = state.pianoRoll.notesVersion;
            RefreshPresetItems(state, state.presetName);
        }
    };
    auto chooseFile = [&](bool song)
    {
        std::string selected;
        const wchar_t* filter = song ? L"F-Synthesizer Song (*.fsynth)\0*.fsynth\0" : L"MIDI (*.mid;*.midi)\0*.mid;*.midi\0";
        if (!BrowseOpenPath("", filter, selected)) return;
        state.pendingOpenPath = selected; state.pendingOpenIsSong = song;
        if (state.presetDirty) ImGui::OpenPopup("曲を切り替える");
        else openPending();
    };
    auto applyPreset = [&](int index)
    {
        state.presetIndex = index;
        // Editing a channel never changes another channel sharing an older workspace sound.
        if (state.playEditingChannel >= 0)
        {
            const int ch = std::clamp(state.playEditingChannel, 0, 15);
            const int slot = gui::AssignedSoundSlot(state, ch);
            bool used[16]{}; int count = 0;
            for (int channel = 0; channel < 16; ++channel)
            { used[gui::AssignedSoundSlot(state, channel)] = true; count += gui::AssignedSoundSlot(state, channel) == slot; }
            state.selectedSoundSlot = slot;
            if (count > 1)
                for (int free = 0; free < 16; ++free) if (!used[free])
                {
                    state.instruments[free] = state.instruments[slot];
                    state.macroSliders[free] = state.macroSliders[slot];
                    gui::SetChannelAssignment(state, ch, free); state.selectedSoundSlot = free; break;
                }
        }
        std::string error;
        if (ApplySelectedPresetPaths(state, error)) { state.presetDirty = true; requestPreview(); }
        else RaiseGUIError(state, error, 0, true);
    };
    const std::string title = state.activeProjectPath.empty() ? "名前を付けていない曲" : PathToUtf8(Utf8ToPath(state.activeProjectPath).filename());
    ImGui::Text("%s%s", title.c_str(), state.presetDirty ? " *" : "");
    ImGui::SameLine(); DrawStatusBadge(state);
    ImGui::BeginDisabled(state.running);
    if (ImGui::Button("MIDIを開く")) chooseFile(false);
    ImGui::SameLine();
    if (ImGui::Button("曲を開く")) chooseFile(true);
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("保存")) saveSong(false);
    ImGui::SameLine();
    if (ImGui::Button("別名保存")) saveSong(true);
    ImGui::SameLine();
    if (ImGui::Button("音色を別名保存")) ImGui::OpenPopup("音色を保存");
    if (ImGui::BeginPopupModal("音色を保存", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::InputText("名前", state.userPresetName, IM_ARRAYSIZE(state.userPresetName));
        ImGui::TextUnformatted("付属音色を残して、自分用のコピーを作ります。");
        if (ImGui::Button("保存"))
        {
            std::string error;
            if (SaveUserPresetFromState(state, error)) ImGui::CloseCurrentPopup();
            else RaiseGUIError(state, error, 0, true);
        }
        ImGui::SameLine();
        if (ImGui::Button("キャンセル")) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    if (ImGui::BeginPopupModal("曲を切り替える", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::TextUnformatted("今の曲に変更があります。切り替える前に保存しますか？");
        if (ImGui::Button("保存して開く") && saveSong(false)) { openPending(); ImGui::CloseCurrentPopup(); }
        ImGui::SameLine();
        if (ImGui::Button("変更を破棄して開く")) { openPending(); ImGui::CloseCurrentPopup(); }
        ImGui::SameLine();
        if (ImGui::Button("キャンセル")) { state.pendingOpenPath.clear(); ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }
    ImGui::BeginDisabled(state.running);
    if (ImGui::Button("曲を再生")) StartGUIRun(state, true);
    ImGui::SameLine();
    if (ImGui::Button("このchだけ再生")) StartGUIRun(state, true, true);
    ImGui::SameLine(); ImGui::Checkbox("繰り返す", &state.previewLoop);
    ImGui::EndDisabled();
    ImGui::SameLine();
    if (ImGui::Button("停止")) StopGUIRunAndPreview(state);
    ImGui::SameLine();
    ImGui::BeginDisabled(state.soundUndoStack.empty());
    if (ImGui::Button("音色を戻す")) UndoSound(state);
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(state.soundRedoStack.empty());
    if (ImGui::Button("やり直す")) RedoSound(state);
    ImGui::EndDisabled();
    if (state.hasUIError)
    {
        ImGui::TextWrapped("%s", state.UIErrorMessage.c_str());
        ImGui::SameLine(); if (ImGui::SmallButton("閉じる##error")) ClearGUIError(state);
    }
    if (ImGui::BeginTabBar("workspace_tabs"))
    {
        const int requestedMode = state.UIModeTab;
        const int modes[] = {1, 0, 3, 2};
        const char* labels[] = {"曲づくり", "音色を試す", "詳細", "書き出し"};
        for (int i = 0; i < 4; ++i)
        {
            const auto flags = requestedMode != lastFrameTab && requestedMode == modes[i]
                ? ImGuiTabItemFlags_SetSelected : ImGuiTabItemFlags_None;
            if (ImGui::BeginTabItem(labels[i], nullptr, flags))
            { state.UIModeTab = modes[i]; ImGui::EndTabItem(); }
        }
        ImGui::EndTabBar();
    }
    lastFrameTab = state.UIModeTab;
    const auto& io = ImGui::GetIO();
    if (!io.WantTextInput && !ImGui::IsAnyItemActive() && !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
    {
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false)) saveSong(io.KeyShift);
        if ((state.UIModeTab == 0 || state.UIModeTab == 3) && io.KeyCtrl)
        {
            if (ImGui::IsKeyPressed(ImGuiKey_Z, false)) UndoSound(state);
            if (ImGui::IsKeyPressed(ImGuiKey_Y, false)) RedoSound(state);
        }
    }
    static bool showLog = false;
    const float reserve = showLog ? 170.0f : 35.0f;
    ImGui::BeginChild("main_body", ImVec2(0, std::max(160.0f, ImGui::GetContentRegionAvail().y - reserve)), false);
    if (state.UIModeTab == 1) DrawComposeView(state, applyPreset, help);
    else if (state.UIModeTab == 0) DrawPlayView(state, applyPreset, help);
    else if (state.UIModeTab == 2) DrawExportView(state, help);
    else DrawAdvancedView(state, applyPreset, help, requestPreview);
    ImGui::EndChild();
    if (state.observedNotesVersion && state.observedNotesVersion != state.pianoRoll.notesVersion) state.presetDirty = true;
    state.observedNotesVersion = state.pianoRoll.notesVersion;
    showLog = ImGui::CollapsingHeader("ログ・表示設定");
    if (showLog)
    {
        const char* scales[] = {"100%", "125%", "150%"};
        ImGui::SetNextItemWidth(100); ImGui::Combo("大きさ", &state.UIScaleIndex, scales, 3);
        ImGui::SameLine();
        const char* themes[] = {"明るい", "暗い"};
        ImGui::SetNextItemWidth(100); ImGui::Combo("配色", &state.UIThemeIndex, themes, 2);
        ImGui::BeginChild("logs", ImVec2(0, 90), true);
        std::lock_guard<std::mutex> lock(state.logMutex);
        const auto& lines = state.UIModeTab == 1 ? state.musicLogs : state.UIModeTab == 2 ? state.exportLogs : state.soundLogs;
        for (const auto& line : lines) ImGui::TextUnformatted(line.c_str());
        ImGui::EndChild();
    }
    if (state.autoTonePreviewPending && !state.running && ImGui::GetTime() - state.autoTonePreviewLastEditSec > 0.4)
    { state.autoTonePreviewPending = false; StartGUISoundTonePreview(state); }
    ImGui::End();
}
