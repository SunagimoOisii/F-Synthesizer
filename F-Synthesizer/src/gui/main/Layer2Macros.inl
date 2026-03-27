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

    bool changed = false;
    if (labels.brightness != nullptr)
    {
        changed |= ImGui::SliderFloat(labels.brightness, &sliders.brightness, 0.0f, 1.0f);
    }

    if (labels.roughness != nullptr)
    {
        if (ImGui::SliderFloat(labels.roughness, &sliders.roughness, 0.0f, 1.0f))
        {
            sliders.lastLayer2Roughness = sliders.roughness;
            changed = true;
        }
    }

    if (labels.movement != nullptr)
    {
        if (ImGui::SliderFloat(labels.movement, &sliders.movement, 0.0f, 1.0f))
        {
            sliders.lastLayer2Movement = sliders.movement;
            changed = true;
        }
    }

    if (labels.envelope != nullptr)
    {
        if (ImGui::SliderFloat(labels.envelope, &sliders.envelope, 0.0f, 1.0f))
        {
            sliders.lastLayer2Envelope = sliders.envelope;
            changed = true;
        }
    }

    if (!changed)
    {
        return;
    }

    if (std::holds_alternative<DrumKitConfig>(cfg.source))
    {
        DrumKitConfig& drumKit = std::get<DrumKitConfig>(cfg.source);
        const int drumNote = std::clamp(state.selectedDrumNote, 0, 127);
        ChannelConfig drumChannel = cfg;
        drumChannel.source = drumKit.map[drumNote];
        ApplyMacroSliders(drumChannel, sliders);
        drumKit.map[drumNote] = std::get<DrumConfig>(drumChannel.source);
        cfg.attackSec = drumChannel.attackSec;
        cfg.releaseSec = drumChannel.releaseSec;
    }
    else
    {
        ApplyMacroSliders(cfg, sliders);
    }

    state.presetDirty = true;
    state.autoTonePreviewPending = true;
}
} // namespace
