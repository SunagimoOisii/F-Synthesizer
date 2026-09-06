namespace
{
static constexpr const char* kPlayCategories[] = {
    "All", "Lead", "Guitar", "Bass", "Pad", "Keys", "Drums", "SFX", "Support"
};

static int PlayCategoryIndexByName(const char* category)
{
    if (category == nullptr || category[0] == '\0')
    {
        return 0;
    }
    for (int i = 0; i < static_cast<int>(IM_ARRAYSIZE(kPlayCategories)); ++i)
    {
        if (std::strcmp(kPlayCategories[i], category) == 0)
        {
            return i;
        }
    }
    return 0;
}

static const std::string& PresetDisplayName(const GUIState& state, int index)
{
    if (index >= 0 && index < static_cast<int>(state.presetItems.size())
        && !state.presetItems[static_cast<size_t>(index)].displayName.empty())
    {
        return state.presetItems[static_cast<size_t>(index)].displayName;
    }
    return state.presetItems[static_cast<size_t>(index)].name;
}

static std::string PresetCategory(const GUIState& state, int index)
{
    if (index >= 0 && index < static_cast<int>(state.presetItems.size())
        && !state.presetItems[static_cast<size_t>(index)].category.empty())
    {
        return state.presetItems[static_cast<size_t>(index)].category;
    }
    return "Keys";
}

static const char* ComposeGMCategoryLabel(const GUIState& state, int channel)
{
    channel = std::clamp(channel, 0, 15);
    if (channel == 9)
    {
        return "Drums";
    }
    if (!state.pianoRoll.hasProgramByChannel[channel])
    {
        return "-";
    }
    return GMProgramCategory(state.pianoRoll.programByChannel[channel], channel);
}

static bool CategoryHasPreset(const GUIState& state, const char* category)
{
    if (category == nullptr || category[0] == '\0' || std::strcmp(category, "All") == 0)
    {
        return !state.presetItems.empty();
    }
    for (int i = 0; i < static_cast<int>(state.presetItems.size()); ++i)
    {
        if (PresetCategory(state, i) == category)
        {
            return true;
        }
    }
    return false;
}

static std::string PresetDescription(const GUIState& state, int index)
{
    if (index >= 0 && index < static_cast<int>(state.presetItems.size()))
    {
        return state.presetItems[static_cast<size_t>(index)].description;
    }
    return {};
}

static std::string ChannelLabel(int channel)
{
    channel = std::clamp(channel, 0, 15);
    std::string label = "ch" + std::to_string(channel + 1);
    if (channel == 9)
    {
        label += " Drums";
    }
    return label;
}

static std::string SoundSourceLabel(const GUIState& state, int soundIndex)
{
    soundIndex = std::clamp(soundIndex, 0, 15);
    const config::SourceKind sourceKind = config::SourceConfigKind(gui::ReadSoundSlot(state, soundIndex).source);
    return config::SourceKindToDisplayName(sourceKind);
}

static std::string SoundSlotDisplayLabel(const GUIState& state, int soundIndex)
{
    soundIndex = std::clamp(soundIndex, 0, 15);
    const std::string& name = state.instruments[static_cast<size_t>(soundIndex)].displayName;
    if (!name.empty())
    {
        return name;
    }
    return "音色 " + std::to_string(soundIndex + 1) + " / " + SoundSourceLabel(state, soundIndex);
}

static std::string ChannelSoundLabel(const GUIState& state, int channel)
{
    channel = std::clamp(channel, 0, 15);
    const int soundIndex = gui::AssignedSoundSlot(state, channel);
    return SoundSlotDisplayLabel(state, soundIndex);
}

template <typename HelpFn>
static void DrawUseInComposeControl(GUIState& state, HelpFn&& updateHoverHelp)
{
    const int soundIndex = std::clamp(state.selectedSoundSlot, 0, 15);
    if (ImGui::Button("曲で使う"))
    {
        ImGui::OpenPopup("use_sound_slot_in_compose");
    }
    updateHoverHelp(
        "選択中の音色を曲で使います。",
        "使うMIDIチャンネルを選んでComposeへ移動します。",
        nullptr);

    if (ImGui::BeginPopup("use_sound_slot_in_compose"))
    {
        ImGui::Text("使う音色: %s", SoundSlotDisplayLabel(state, soundIndex).c_str());
        ImGui::TextDisabled("使うチャンネルを選択");
        ImGui::Separator();
        if (ImGui::BeginTable("use_sound_slot_channels", 4, ImGuiTableFlags_SizingStretchSame))
        {
            for (int ch = 0; ch < 16; ++ch)
            {
                ImGui::TableNextColumn();
                const std::string label = ChannelLabel(ch);
                if (ImGui::Button(label.c_str(), ImVec2(-1.0f, 0.0f)))
                {
                    gui::SetChannelAssignment(state, ch, soundIndex);
                    state.pianoRoll.displayChannel = ch;
                    state.playEditingChannel = -1;
                    state.UIModeTab = 1;
                    state.presetDirty = true;
                    ImGui::CloseCurrentPopup();
                }
            }
            ImGui::EndTable();
        }
        ImGui::EndPopup();
    }
}

template <typename ApplyPresetFn, typename HelpFn>
static void DrawPlayView(
    GUIState& state,
    ApplyPresetFn&& applyPresetByIndex,
    HelpFn&& updateHoverHelp)
{
    state.playCategoryIndex = std::clamp(state.playCategoryIndex, 0, static_cast<int>(IM_ARRAYSIZE(kPlayCategories)) - 1);
    if (state.playCategoryIndex > 0 && !CategoryHasPreset(state, kPlayCategories[state.playCategoryIndex]))
    {
        state.playCategoryIndex = 0;
    }

    ImGui::TextUnformatted("プリセット");
    const int editingChannel = std::clamp(state.playEditingChannel, -1, 15);
    if (editingChannel >= 0)
    {
        ImGui::TextDisabled("%s の音色を選択中", ChannelLabel(editingChannel).c_str());
    }
    else
    {
        ImGui::TextDisabled("選んで、鳴らして、曲で使うチャンネルを決める。");
    }
    ImGui::Spacing();

    for (int i = 0; i < static_cast<int>(IM_ARRAYSIZE(kPlayCategories)); ++i)
    {
        if (i > 0 && !CategoryHasPreset(state, kPlayCategories[i]))
        {
            continue;
        }
        if (i > 0)
        {
            ImGui::SameLine();
        }
        const bool selected = state.playCategoryIndex == i;
        if (selected)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        }
        if (ImGui::Button(kPlayCategories[i]))
        {
            state.playCategoryIndex = i;
        }
        if (selected)
        {
            ImGui::PopStyleColor();
        }
    }
    updateHoverHelp("プリセットの用途を選びます。", "表示する音色カードを絞り込みます。", nullptr);

    ImGui::Separator();
    const bool showInspector = state.playInspectorOpen;
    const float inspectorWidth = showInspector ? 320.0f : 0.0f;
    if (ImGui::BeginTable("play_sound_cards_layout", showInspector ? 2 : 1, ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("cards", ImGuiTableColumnFlags_WidthStretch, 1.0f);
        if (showInspector)
        {
            ImGui::TableSetupColumn("inspector", ImGuiTableColumnFlags_WidthFixed, inspectorWidth);
        }
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);

        ImGui::BeginChild("play_sound_cards", ImVec2(0.0f, 260.0f), true);
        bool anyVisible = false;
        for (int i = 0; i < static_cast<int>(state.presetItems.size()); ++i)
        {
            const std::string category = PresetCategory(state, i);
            if (state.playCategoryIndex > 0 && category != kPlayCategories[state.playCategoryIndex])
            {
                continue;
            }
            anyVisible = true;
            ImGui::PushID(i);
            const bool selected = state.presetIndex == i;
            if (selected)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            }
            if (ImGui::Button(PresetDisplayName(state, i).c_str(), ImVec2(220.0f, 44.0f)))
            {
                applyPresetByIndex(i);
            }
            if (selected)
            {
                ImGui::PopStyleColor();
            }
            ImGui::SameLine();
            ImGui::BeginGroup();
            ImGui::TextDisabled("%s", category.c_str());
            const std::string desc = PresetDescription(state, i);
            if (!desc.empty())
            {
                ImGui::TextWrapped("%s", desc.c_str());
            }
            ImGui::EndGroup();
            ImGui::PopID();
        }
        if (!anyVisible)
        {
            ImGui::TextDisabled("このカテゴリのプリセットはありません。");
        }
        ImGui::EndChild();
        updateHoverHelp("プリセットを選びます。", "選択した音色を現在の編集対象へ読み込みます。", nullptr);

        DrawLayer2Macros(state);
        const int slot = std::clamp(state.selectedSoundSlot, 0, 15);
        if (config::UsesDrumKitNoteSelection(gui::ReadSoundSlot(state, slot).source))
        {
            DrawDrumPadPreview(state);
        }
        else
        {
            DrawVirtualKeyboard(state);
        }
        DrawUseInComposeControl(
            state,
            [&](const char* what, const char* impact, const char* caution)
            {
                updateHoverHelp(what, impact, caution);
            });

        if (showInspector)
        {
            ImGui::TableSetColumnIndex(1);
            ImGui::BeginChild("play_inspector", ImVec2(0.0f, 0.0f), true);
            if (ImGui::Button("Inspectorを閉じる"))
            {
                state.playInspectorOpen = false;
            }
            updateHoverHelp("Inspectorを閉じます。", "カード一覧とマクロを広く使えます。", nullptr);
            ImGui::Separator();
            {
                const auto& instrument = state.instruments[std::clamp(state.selectedSoundSlot, 0, 15)];
                ImGui::TextWrapped("%s", instrument.displayName.empty() ? "編集中の音色" : instrument.displayName.c_str());
                ImGui::TextDisabled("%s", instrument.category.c_str());
                if (!instrument.description.empty())
                {
                    ImGui::TextWrapped("%s", instrument.description.c_str());
                }
                ImGui::Separator();
                if (ImGui::Button("試聴"))
                {
                    state.tonePreviewNoteNumber = instrument.recommendedRange.preview;
                    StartGUIRun(state, true);
                }
                updateHoverHelp("選択中の音を試聴します。", "WAVを書き出さずに再生します。", nullptr);
                ImGui::SameLine();
                if (ImGui::Button("詳細を編集"))
                {
                    state.UIModeTab = 3;
                }
                updateHoverHelp("詳細編集へ移動します。", "専門的な音色パラメータを開きます。", nullptr);
                if (!instrument.displayName.empty())
                {
                    ImGui::Separator();
                    ImGui::TextDisabled(
                        "推奨音域: %d-%d / Preview %d",
                        instrument.recommendedRange.low,
                        instrument.recommendedRange.high,
                        instrument.recommendedRange.preview);
                }
            }
            ImGui::EndChild();
        }
        ImGui::EndTable();
    }
    if (!state.playInspectorOpen)
    {
        if (ImGui::Button("説明を開く"))
        {
            state.playInspectorOpen = true;
        }
        updateHoverHelp("Inspectorを開きます。", "選択中カードの説明と詳細導線を表示します。", nullptr);
    }
}

template <typename ApplyPresetFn, typename HelpFn>
static void DrawComposeView(GUIState& state, ApplyPresetFn&& applyPresetByIndex, HelpFn&& updateHoverHelp)
{
    if (state.midiPath[0] == '\0')
    {
        ImGui::TextWrapped("「MIDIを開く」で曲を選んでください。左でチャンネルを選び、右で音色を変えます。");
        return;
    }
    gui::LoadPianoRollMIDI(state.pianoRoll, Utf8ToPath(state.midiPath));
    if (state.pianoRoll.hasLoadError)
    {
        ImGui::TextWrapped("%s", state.pianoRoll.lastError.c_str());
        return;
    }
    const float height = ImGui::GetContentRegionAvail().y;
    const float topHeight = std::clamp(height * 0.50f, 180.0f, 330.0f);
    if (ImGui::BeginTable("song_layout", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp))
    {
        ImGui::TableSetupColumn("channels", ImGuiTableColumnFlags_WidthStretch, 0.36f);
        ImGui::TableSetupColumn("sound", ImGuiTableColumnFlags_WidthStretch, 0.64f);
        ImGui::TableNextRow();
        ImGui::TableSetColumnIndex(0);
        ImGui::BeginChild("song_channels", ImVec2(0, topHeight), true);
        ImGui::TextUnformatted("チャンネル");
        for (int ch = 0; ch < 16; ++ch)
        {
            ImGui::PushID(ch);
            const std::string label = ChannelLabel(ch) + "  " + ChannelSoundLabel(state, ch) +
                "  (" + std::to_string(state.pianoRoll.noteCountByChannel[ch]) + ")";
            if (ImGui::Selectable(label.c_str(), state.pianoRoll.displayChannel == ch))
            {
                state.pianoRoll.displayChannel = ch;
                state.selectedSoundSlot = gui::AssignedSoundSlot(state, ch);
                state.playEditingChannel = ch;
                int total = 0, count = 0;
                for (const auto& note : state.pianoRoll.notes) if (note.channel == ch) { total += note.note; ++count; }
                if (count) state.pianoRoll.noteOffset = std::clamp(total / count - state.pianoRoll.visibleNoteCount / 2, 0, 115);
                state.pianoRoll.visibleNoteIndexCacheValid = false;
            }
            ImGui::PopID();
        }
        ImGui::EndChild();
        ImGui::TableSetColumnIndex(1);
        ImGui::BeginChild("song_sound", ImVec2(0, topHeight), true);
        const int ch = std::clamp(state.pianoRoll.displayChannel, 0, 15);
        state.selectedSoundSlot = gui::AssignedSoundSlot(state, ch);
        state.playEditingChannel = ch;
        ImGui::Text("%s の音色", ChannelLabel(ch).c_str());
        if (state.pianoRoll.hasProgramByChannel[ch])
            ImGui::TextDisabled("元の楽器: %s", GMProgramDisplayLabel(state.pianoRoll.programByChannel[ch]).c_str());
        if (ImGui::BeginCombo("プリセット", ChannelSoundLabel(state, ch).c_str()))
        {
            for (int i = 0; i < static_cast<int>(state.presetItems.size()); ++i)
            {
                const auto& preset = state.presetItems[i];
                if (ch == 9 && state.drumChannelSpecialHandling && preset.category != "Drums") continue;
                const std::string label = preset.category + " / " + preset.displayName + "##" + preset.name;
                if (ImGui::Selectable(label.c_str(), state.presetIndex == i)) applyPresetByIndex(i);
                if (ImGui::IsItemHovered() && !preset.description.empty()) ImGui::SetTooltip("%s", preset.description.c_str());
            }
            ImGui::EndCombo();
        }
        const auto& instrument = state.instruments[state.selectedSoundSlot];
        if (!instrument.description.empty()) ImGui::TextWrapped("%s", instrument.description.c_str());
        auto& mix = gui::MutableChannelMix(state, ch);
        state.presetDirty |= ImGui::Checkbox("消音", &mix.mute);
        ImGui::SameLine();
        state.presetDirty |= ImGui::Checkbox("このchをソロ", &mix.solo);
        auto slider = [&](const char* label, double& value, float low, float high)
        {
            float v = static_cast<float>(value);
            if (ImGui::SliderFloat(label, &v, low, high, "%.2f")) { value = v; state.presetDirty = true; }
        };
        slider("音量", mix.level, 0, 2);
        slider("左右", mix.pan, -1, 1);
        DrawLayer2Macros(state);
        ImGui::EndChild();
        ImGui::EndTable();
    }
    ImGui::Separator();
    const bool drums = state.pianoRoll.displayChannel == 9 && state.drumChannelSpecialHandling;
    if (drums)
    {
        if (ImGui::Checkbox("16ステップで入力", &state.stepSeq.viewActive) && state.stepSeq.viewActive)
            LoadStepSeqFromPianoRoll(state.stepSeq, state.pianoRoll);
    }
    ImGui::BeginChild("song_notes", ImVec2(0, 0), true);
    if (drums && state.stepSeq.viewActive) DrawStepSeqPanel(state);
    else DrawPianoRollPanel(state.pianoRoll, state.midiPath, &state.playback,
        [&](const std::string& line) { AppendGUILog(state, line); },
        [&]() { StartGUIRun(state, true); }, [&]() { StopGUIRunAndPreview(state); });
    ImGui::EndChild();
}

template <typename ApplyPresetFn, typename HelpFn, typename PreviewFn>
static void DrawAdvancedView(
    GUIState& state,
    ApplyPresetFn&& applyPresetByIndex,
    HelpFn&& updateHoverHelp,
    PreviewFn&& requestAutoTonePreview)
{

    ImGui::TextUnformatted("Advanced");
    ImGui::TextDisabled("専門的な音色、FX、ミックス設定をまとめて扱います。");
    ImGui::Separator();

    if (ImGui::CollapsingHeader("Preset Library", ImGuiTreeNodeFlags_DefaultOpen))
    {
        ImGui::BeginChild("advanced_presets", ImVec2(0.0f, 150.0f), true);
        for (int i = 0; i < static_cast<int>(state.presetItems.size()); ++i)
        {
            const bool selected = state.presetIndex == i;
            std::string label = PresetDisplayName(state, i);
            if (state.presetItems[static_cast<size_t>(i)].internalOnly)
            {
                label += "  [demo]";
            }
            if (ImGui::Selectable(label.c_str(), selected))
            {
                applyPresetByIndex(i);
            }
        }
        ImGui::EndChild();
    }

    const bool channelEditorChanged = DrawChannelEditor(
        state,
        true,
        [&](const char* what, const char* impact, const char* caution)
        {
            updateHoverHelp(what, impact, caution);
        });
    state.presetDirty |= channelEditorChanged;
    if (channelEditorChanged)
    {
        requestAutoTonePreview();
    }

    if (ImGui::CollapsingHeader("Master FX"))
    {
        auto sliderFxDouble = [&](const char* label, double& value, float minV, float maxV, const char* fmt) -> bool
        {
            float v = static_cast<float>(value);
            const bool edited = ImGui::SliderFloat(label, &v, minV, maxV, fmt);
            if (edited)
            {
                value = static_cast<double>(v);
                state.presetDirty = true;
            }
            return edited;
        };
        state.presetDirty |= ImGui::Checkbox("Chorus", &state.masterEffects.chorus.enabled);
        sliderFxDouble("Chorus Mix", state.masterEffects.chorus.mix, 0.0f, 1.0f, "%.3f");
        sliderFxDouble("Chorus Base Delay (ms)", state.masterEffects.chorus.baseDelayMs, 2.0f, 40.0f, "%.2f");
        sliderFxDouble("Chorus Depth (ms)", state.masterEffects.chorus.depthMs, 0.0f, 20.0f, "%.2f");
        sliderFxDouble("Chorus Rate (Hz)", state.masterEffects.chorus.rateHz, 0.05f, 8.0f, "%.2f");
        sliderFxDouble("Chorus Feedback", state.masterEffects.chorus.feedback, 0.0f, 0.9f, "%.3f");
        ImGui::Separator();
        state.presetDirty |= ImGui::Checkbox("Flanger", &state.masterEffects.flanger.enabled);
        sliderFxDouble("Flanger Mix", state.masterEffects.flanger.mix, 0.0f, 1.0f, "%.3f");
        sliderFxDouble("Flanger Base Delay (ms)", state.masterEffects.flanger.baseDelayMs, 0.1f, 8.0f, "%.2f");
        sliderFxDouble("Flanger Depth (ms)", state.masterEffects.flanger.depthMs, 0.0f, 5.0f, "%.2f");
        sliderFxDouble("Flanger Rate (Hz)", state.masterEffects.flanger.rateHz, 0.05f, 8.0f, "%.2f");
        sliderFxDouble("Flanger Feedback", state.masterEffects.flanger.feedback, 0.0f, 0.95f, "%.3f");
        ImGui::Separator();
        state.presetDirty |= ImGui::Checkbox("Delay", &state.masterEffects.delay.enabled);
        sliderFxDouble("Delay Mix", state.masterEffects.delay.mix, 0.0f, 1.0f, "%.3f");
        sliderFxDouble("Delay Feedback", state.masterEffects.delay.feedback, 0.0f, 0.95f, "%.3f");
        state.presetDirty |= ImGui::Checkbox("Delay Tempo Sync", &state.masterEffects.delay.tempoSync);
        if (state.masterEffects.delay.tempoSync)
        {
            sliderFxDouble("Delay Sync Beats", state.masterEffects.delay.syncBeats, 0.125f, 4.0f, "%.3f");
        }
        else
        {
            sliderFxDouble("Delay Time (sec)", state.masterEffects.delay.timeSec, 0.01f, 2.0f, "%.3f");
        }
        ImGui::Separator();
        state.presetDirty |= ImGui::Checkbox("Reverb", &state.masterEffects.reverb.enabled);
        sliderFxDouble("Reverb Mix", state.masterEffects.reverb.mix, 0.0f, 1.0f, "%.3f");
        sliderFxDouble("Reverb Room Size", state.masterEffects.reverb.roomSize, 0.1f, 1.0f, "%.3f");
        sliderFxDouble("Reverb Damping", state.masterEffects.reverb.damping, 0.0f, 1.0f, "%.3f");
        ImGui::Separator();
        int bits = std::clamp(state.masterEffects.bitCrusher.bits, 1, 16);
        if (ImGui::SliderInt("BitCrusher Bits", &bits, 1, 16))
        {
            state.masterEffects.bitCrusher.bits = bits;
            state.presetDirty = true;
        }
        sliderFxDouble("SampleRate Ratio", state.masterEffects.sampleRateReducer.ratio, 0.0f, 1.0f, "%.3f");
    }

    if (ImGui::CollapsingHeader("Mixer / Assign"))
    {
        if (ImGui::BeginTable("advanced_mixer", 7,
                ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_ScrollY,
                ImVec2(0.0f, 300.0f)))
        {
            ImGui::TableSetupColumn("ch", ImGuiTableColumnFlags_WidthFixed, 42.0f);
            ImGui::TableSetupColumn("音色", ImGuiTableColumnFlags_WidthFixed, 90.0f);
            ImGui::TableSetupColumn("M", ImGuiTableColumnFlags_WidthFixed, 32.0f);
            ImGui::TableSetupColumn("S", ImGuiTableColumnFlags_WidthFixed, 32.0f);
            ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthStretch, 0.34f);
            ImGui::TableSetupColumn("Pan", ImGuiTableColumnFlags_WidthStretch, 0.33f);
            ImGui::TableSetupColumn("Gain", ImGuiTableColumnFlags_WidthStretch, 0.33f);
            ImGui::TableHeadersRow();
            auto sliderMix = [&](const char* label, double& value, float minV, float maxV) -> bool
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
                ChannelMixState& mix = gui::MutableChannelMix(state, ch);
                ImGui::TableNextRow();
                ImGui::PushID(ch);
                ImGui::TableSetColumnIndex(0);
                ImGui::TextUnformatted(ChannelLabel(ch).c_str());
                ImGui::TableSetColumnIndex(1);
                int assigned = gui::AssignedSoundSlot(state, ch);
                int assignedSoundNumber = assigned + 1;
                ImGui::SetNextItemWidth(-FLT_MIN);
                if (ImGui::SliderInt("##assign", &assignedSoundNumber, 1, 16, "%d"))
                {
                    gui::SetChannelAssignment(state, ch, assignedSoundNumber - 1);
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
                ImGui::PopID();
            }
            ImGui::EndTable();
        }
    }
}
} // namespace
