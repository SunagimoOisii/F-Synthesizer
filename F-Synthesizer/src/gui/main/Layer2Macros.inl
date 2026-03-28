namespace
{
struct MacroLabels
{
    const char* brightness = nullptr;
    const char* roughness = nullptr;
    const char* movement = nullptr;
    const char* envelope = nullptr;
};

MacroLabels GetMacroLabels(const SourceConfig& src)
{
    if (std::holds_alternative<FmConfig>(src))
    {
        return { "明るさ (FB)", "荒さ (Index)", "揺れ (LFO)", "鳴り方 (ADSR)" };
    }
    if (std::holds_alternative<NoiseConfig>(src))
    {
        return { "明るさ (Filter)", "荒さ (Res)", nullptr, "鳴り方 (ADSR)" };
    }
    if (std::holds_alternative<DrumConfig>(src) || std::holds_alternative<DrumKitConfig>(src))
    {
        return { "明るさ (Pitch)", "荒さ (Noise)", "揺れ (Sweep)", "鳴り方 (Decay)" };
    }
    if (std::holds_alternative<PsgConfig>(src))
    {
        return { "明るさ (Duty)", nullptr, nullptr, "鳴り方 (ADSR)" };
    }
    return { "明るさ (Filter)", "荒さ (Res+Drive)", "揺れ (LFO)", "鳴り方 (ADSR)" };
}

// マクロスライダーをパラメータへ反映し、Auto Tone Preview を要求する。
// DrumKit の場合は selectedDrumNote の DrumConfig に適用する。
// DrawLayer2Macros 内のスライダー変更と Randomize の両方から呼ばれる。
void ApplyAndTriggerPreview(GUIState& state, int ch)
{
    gui::EnsureChannelConfigs(state);
    ChannelConfig& cfg = (*state.channelConfigs)[ch];
    const MacroSliderState& sl = state.macroSliders[ch];

    if (std::holds_alternative<DrumKitConfig>(cfg.source))
    {
        DrumKitConfig& drumKit = std::get<DrumKitConfig>(cfg.source);
        const int drumNote = std::clamp(state.selectedDrumNote, 0, 127);
        ChannelConfig drumChannel = cfg;
        drumChannel.source = drumKit.map[drumNote];
        ApplyMacroSliders(drumChannel, sl);
        drumKit.map[drumNote] = std::get<DrumConfig>(drumChannel.source);
        cfg.attackSec = drumChannel.attackSec;
        cfg.releaseSec = drumChannel.releaseSec;
    }
    else
    {
        ApplyMacroSliders(cfg, sl);
    }

    state.presetDirty = true;
    if (state.autoTonePreviewEnabled)
    {
        state.autoTonePreviewPending = true;
        state.autoTonePreviewLastEditSec = ImGui::GetTime();
    }
}

void DrawLayer2Macros(GUIState& state)
{
    ImGui::SetNextItemOpen(state.layer2Expanded, ImGuiCond_Once);
    if (!ImGui::CollapsingHeader("調整  ( マクロスライダー )"))
    {
        state.layer2Expanded = false;
        return;
    }
    state.layer2Expanded = true;

    gui::EnsureChannelConfigs(state);
    const int ch = std::clamp(state.selectedSoundSlot, 0, 15);
    ChannelConfig& cfg = (*state.channelConfigs)[ch];
    MacroSliderState& sliders = state.macroSliders[ch];
    const MacroLabels labels = GetMacroLabels(cfg.source);

    // --- Undo 用 before スナップショット（スライダーのドラッグ開始時にキャプチャ） ---
    static MacroSliderState undoBeforeSliders{};
    static ChannelConfig undoBeforeConfig{};
    static int undoBeforeSlot = -1;

    // --- マクロスライダー ---
    bool changed = false;
    if (labels.brightness != nullptr)
    {
        const float bv = sliders.brightness; // ドラッグ開始前の値
        changed |= ImGui::SliderFloat(labels.brightness, &sliders.brightness, 0.0f, 1.0f);
        if (ImGui::IsItemActivated())
        {
            undoBeforeSliders = sliders;
            undoBeforeSliders.brightness = bv; // SliderFloat が更新する前の値に補正
            undoBeforeConfig = cfg;
            undoBeforeSlot = ch;
        }
        if (ImGui::IsItemDeactivatedAfterEdit() && undoBeforeSlot >= 0)
        {
            PushSoundHistoryEntry(state, undoBeforeSlot, undoBeforeConfig, undoBeforeSliders);
        }
    }
    if (labels.roughness != nullptr)
    {
        const float bv = sliders.roughness;
        if (ImGui::SliderFloat(labels.roughness, &sliders.roughness, 0.0f, 1.0f))
        {
            sliders.lastLayer2Roughness = sliders.roughness;
            changed = true;
        }
        if (ImGui::IsItemActivated())
        {
            undoBeforeSliders = sliders;
            undoBeforeSliders.roughness = bv;
            undoBeforeSliders.lastLayer2Roughness = bv;
            undoBeforeConfig = cfg;
            undoBeforeSlot = ch;
        }
        if (ImGui::IsItemDeactivatedAfterEdit() && undoBeforeSlot >= 0)
        {
            PushSoundHistoryEntry(state, undoBeforeSlot, undoBeforeConfig, undoBeforeSliders);
        }
    }
    if (labels.movement != nullptr)
    {
        const float bv = sliders.movement;
        if (ImGui::SliderFloat(labels.movement, &sliders.movement, 0.0f, 1.0f))
        {
            sliders.lastLayer2Movement = sliders.movement;
            changed = true;
        }
        if (ImGui::IsItemActivated())
        {
            undoBeforeSliders = sliders;
            undoBeforeSliders.movement = bv;
            undoBeforeSliders.lastLayer2Movement = bv;
            undoBeforeConfig = cfg;
            undoBeforeSlot = ch;
        }
        if (ImGui::IsItemDeactivatedAfterEdit() && undoBeforeSlot >= 0)
        {
            PushSoundHistoryEntry(state, undoBeforeSlot, undoBeforeConfig, undoBeforeSliders);
        }
    }
    if (labels.envelope != nullptr)
    {
        const float bv = sliders.envelope;
        if (ImGui::SliderFloat(labels.envelope, &sliders.envelope, 0.0f, 1.0f))
        {
            sliders.lastLayer2Envelope = sliders.envelope;
            changed = true;
        }
        if (ImGui::IsItemActivated())
        {
            undoBeforeSliders = sliders;
            undoBeforeSliders.envelope = bv;
            undoBeforeSliders.lastLayer2Envelope = bv;
            undoBeforeConfig = cfg;
            undoBeforeSlot = ch;
        }
        if (ImGui::IsItemDeactivatedAfterEdit() && undoBeforeSlot >= 0)
        {
            PushSoundHistoryEntry(state, undoBeforeSlot, undoBeforeConfig, undoBeforeSliders);
        }
    }
    if (changed)
    {
        ApplyAndTriggerPreview(state, ch);
    }

    // --- Randomize ---
    ImGui::Separator();

    // 強度選択ラジオボタン
    static const char* kStrengthLabels[] = { "Subtle", "Medium", "Wild" };
    for (int i = 0; i < 3; ++i)
    {
        if (i > 0)
        {
            ImGui::SameLine();
        }
        if (ImGui::RadioButton(kStrengthLabels[i], state.macroRandomizeStrength == i))
        {
            state.macroRandomizeStrength = i;
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Randomize"))
    {
        // Undo スタックに積む（Ctrl+Z 対応）
        PushSoundHistoryEntry(state, ch, cfg, sliders);
        // 元に戻すボタン用スナップショットも更新（既存の [元に戻す] ボタンを存続）
        state.macroRandomizeSnapshot[ch] = sliders;
        state.macroRandomizeHasSnapshot[ch] = true;

        static std::mt19937 rng{ std::random_device{}() };

        // Subtle/Medium: 現在値を基準にオフセット付加。Wild: 0..1 の完全ランダム
        const float halfRange = (state.macroRandomizeStrength == 0) ? 0.1f : 0.3f;
        auto randVal = [&](float base) -> float
        {
            if (state.macroRandomizeStrength == 2)
            {
                return std::uniform_real_distribution<float>{ 0.0f, 1.0f }(rng);
            }
            return std::clamp(
                base + std::uniform_real_distribution<float>{ -halfRange, halfRange }(rng),
                0.0f,
                1.0f);
        };

        // nullptr のスロットはソース非対応のため変更しない
        if (labels.brightness != nullptr)
        {
            sliders.brightness = randVal(sliders.brightness);
        }
        if (labels.roughness != nullptr)
        {
            sliders.roughness = randVal(sliders.roughness);
            sliders.lastLayer2Roughness = sliders.roughness;
        }
        if (labels.movement != nullptr)
        {
            sliders.movement = randVal(sliders.movement);
            sliders.lastLayer2Movement = sliders.movement;
        }
        if (labels.envelope != nullptr)
        {
            sliders.envelope = randVal(sliders.envelope);
            sliders.lastLayer2Envelope = sliders.envelope;
        }

        ApplyAndTriggerPreview(state, ch);
    }

    // スナップショットがある場合のみ「元に戻す」を表示
    if (state.macroRandomizeHasSnapshot[ch])
    {
        ImGui::SameLine();
        if (ImGui::Button("元に戻す##rand"))
        {
            sliders = state.macroRandomizeSnapshot[ch];
            state.macroRandomizeHasSnapshot[ch] = false;
            ApplyAndTriggerPreview(state, ch);
        }
    }
}
} // namespace
