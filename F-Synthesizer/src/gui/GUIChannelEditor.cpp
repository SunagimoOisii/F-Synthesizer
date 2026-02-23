#include "gui/GUIChannelEditor.h"

#include <algorithm>
#include <string>

#include <imgui.h>

#include "config/SourceRegistry.h"
#include "gui/GUIConfigUtils.h"
#include "gui/GUIStateModel.h"

namespace
{
bool DrawDrumConfigEditor(const char* idPrefix, DrumConfig& d)
{
    bool changed = false;
    int drumType = static_cast<int>(d.type);
    const char* drumTypes[] = { "none", "kick", "snare", "hat" };
    std::string key = std::string("Drum Type##") + idPrefix;
    changed |= ImGui::Combo(key.c_str(), &drumType, drumTypes, IM_ARRAYSIZE(drumTypes));
    d.type = static_cast<DrumType>(drumType);

    key = std::string("Gain##") + idPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.gain, 0.01, 0.1, "%.3f");
    key = std::string("Base Freq##") + idPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.baseFreq, 1.0, 10.0, "%.2f");
    key = std::string("Pitch Drop##") + idPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.pitchDrop, 0.1, 1.0, "%.3f");
    key = std::string("Pitch Decay##") + idPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.pitchDecaySec, 0.01, 0.1, "%.3f");
    key = std::string("Tone Freq##") + idPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.toneFreq, 10.0, 100.0, "%.2f");
    key = std::string("Tone Level##") + idPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.toneLevel, 0.01, 0.1, "%.3f");
    key = std::string("Noise Level##") + idPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.noiseLevel, 0.01, 0.1, "%.3f");
    key = std::string("HP Cut##") + idPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.hpCut, 10.0, 100.0, "%.2f");
    key = std::string("LP Cut##") + idPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.lpCut, 10.0, 100.0, "%.2f");

    int toneWave = d.toneWave >= 0 ? d.toneWave : 0;
    const char* waves[] = { "sine", "square", "saw", "triangle" };
    key = std::string("Tone Wave##") + idPrefix;
    changed |= ImGui::Combo(key.c_str(), &toneWave, waves, IM_ARRAYSIZE(waves));
    d.toneWave = toneWave;

    int noiseType = d.noiseType >= 0 ? d.noiseType : 0;
    const char* noises[] = { "white", "pink", "brown", "blue" };
    key = std::string("Noise Type##") + idPrefix;
    changed |= ImGui::Combo(key.c_str(), &noiseType, noises, IM_ARRAYSIZE(noises));
    d.noiseType = noiseType;
    return changed;
}
} // namespace

namespace gui
{
bool DrawChannelEditor(GUIState& state)
{
    bool changed = false;
    EnsureChannelConfigs(state);
    state.selectedSoundSlot = std::clamp(state.selectedSoundSlot, 0, 15);

    ImGui::TextUnformatted("Sound Slot");
    changed |= ImGui::InputInt("Selected Sound Slot (0-15)", &state.selectedSoundSlot);
    state.selectedSoundSlot = std::clamp(state.selectedSoundSlot, 0, 15);

    auto sliderWaveParam = [&](const char* label, double& value, float minV, float maxV, const char* fmt = "%.3f") -> bool
    {
        float v = static_cast<float>(value);
        bool edited = ImGui::SliderFloat(label, &v, minV, maxV, fmt);
        if (edited)
        {
            value = static_cast<double>(v);
        }
        return edited;
    };

    ImGui::TextDisabled("Sound tab edits sound definitions only. Channel mix/assign is in Music tab.");
    ImGui::BeginChild("sound_slot_summary", ImVec2(0, 210), true);
    if (ImGui::BeginTable("sound_slot_table", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_SizingStretchSame))
    {
        ImGui::TableSetupColumn("slot", ImGuiTableColumnFlags_WidthFixed, 58.0f);
        ImGui::TableSetupColumn("edit");
        ImGui::TableHeadersRow();

        for (int slot = 0; slot < 16; slot++)
        {
            ImGui::TableNextRow();
            ImGui::PushID(slot);

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("s%d", slot);

            ImGui::TableSetColumnIndex(1);
            bool selected = (state.selectedSoundSlot == slot);
            if (ImGui::Selectable("Edit", selected))
            {
                state.selectedSoundSlot = slot;
            }

            ImGui::PopID();
        }
        ImGui::EndTable();
    }
    ImGui::EndChild();

    ImGui::Separator();
    ChannelConfig& chCfg = (*state.channelConfigs)[state.selectedSoundSlot];
    ImGui::Text("Selected s%d", state.selectedSoundSlot);
    ImGui::TextDisabled("Tone preview uses selected sound slot.");

    if (ImGui::CollapsingHeader("Envelope / Gain", ImGuiTreeNodeFlags_DefaultOpen))
    {
        changed |= ImGui::InputDouble("Ch Amp", &chCfg.amp, 0.01, 0.1, "%.3f");
        changed |= ImGui::InputDouble("Ch Attack", &chCfg.attackSec, 0.01, 0.1, "%.3f");
        changed |= ImGui::InputDouble("Ch Decay", &chCfg.decaySec, 0.01, 0.1, "%.3f");
        changed |= ImGui::InputDouble("Ch Sustain", &chCfg.sustainLevel, 0.01, 0.1, "%.3f");
        changed |= ImGui::InputDouble("Ch Release", &chCfg.releaseSec, 0.01, 0.1, "%.3f");
    }

    if (ImGui::CollapsingHeader("Source Details", ImGuiTreeNodeFlags_DefaultOpen))
    {
        int srcType = SourceTypeIndex(chCfg.source);
        const config::SourceKind selectedKind = config::SourceKindFromIndex(srcType);
        if (ImGui::BeginCombo("Source Type", config::SourceKindToDisplayName(selectedKind)))
        {
            for (int i = 0; i < config::kSourceKindCount; i++)
            {
                const config::SourceKind candidate = config::SourceKindFromIndex(i);
                const bool selected = (srcType == i);
                if (ImGui::Selectable(config::SourceKindToDisplayName(candidate), selected))
                {
                    srcType = i;
                    changed = true;
                    chCfg.source = DefaultSourceByType(srcType);
                }
                if (selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }

        if (auto* wf = std::get_if<WaveformConfig>(&chCfg.source))
        {
            int idx = WaveToIndex(wf->wave);
            const char* waves[] = { "sine", "square", "saw", "triangle" };
            changed |= ImGui::Combo("Wave", &idx, waves, IM_ARRAYSIZE(waves));
            wf->wave = WaveFromIndex(idx);

            wf->unisonVoices = std::clamp(wf->unisonVoices, 1, 8);
            wf->unisonDetuneCents = std::clamp(wf->unisonDetuneCents, 0.0, 120.0);
            wf->unisonSpread = std::clamp(wf->unisonSpread, 0.0, 1.0);
            wf->subOscLevel = std::clamp(wf->subOscLevel, 0.0, 2.0);

            int unisonVoices = wf->unisonVoices;
            ImGui::SetNextItemWidth(220.0f);
            if (ImGui::SliderInt("Unison Voices", &unisonVoices, 1, 8))
            {
                wf->unisonVoices = unisonVoices;
                changed = true;
            }
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Unison Detune (cent)", wf->unisonDetuneCents, 0.0f, 120.0f, "%.1f");
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Unison Spread", wf->unisonSpread, 0.0f, 1.0f, "%.2f");
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Sub Osc Level", wf->subOscLevel, 0.0f, 2.0f, "%.2f");
        }
        else if (auto* nz = std::get_if<NoiseConfig>(&chCfg.source))
        {
            int idx = NoiseToIndex(nz->noise);
            const char* noises[] = { "white", "pink", "brown", "blue" };
            changed |= ImGui::Combo("Noise", &idx, noises, IM_ARRAYSIZE(noises));
            nz->noise = NoiseFromIndex(idx);
        }
        else if (auto* fm = std::get_if<FmConfig>(&chCfg.source))
        {
            int cIdx = WaveToIndex(fm->carrierWave);
            int mIdx = WaveToIndex(fm->modWave);
            const char* waves[] = { "sine", "square", "saw", "triangle" };
            changed |= ImGui::Combo("Carrier Wave", &cIdx, waves, IM_ARRAYSIZE(waves));
            changed |= ImGui::Combo("Mod Wave", &mIdx, waves, IM_ARRAYSIZE(waves));
            fm->carrierWave = WaveFromIndex(cIdx);
            fm->modWave = WaveFromIndex(mIdx);
            changed |= ImGui::InputDouble("Carrier Ratio", &fm->carrierRatio, 0.01, 0.1, "%.3f");
            changed |= ImGui::InputDouble("Mod Ratio", &fm->modRatio, 0.01, 0.1, "%.3f");
            changed |= ImGui::InputDouble("FM Index", &fm->index, 0.01, 0.1, "%.3f");
            changed |= ImGui::InputDouble("FM OutLevel", &fm->outLevel, 0.01, 0.1, "%.3f");
        }
        else if (auto* drum = std::get_if<DrumConfig>(&chCfg.source))
        {
            changed |= DrawDrumConfigEditor("drum_single", *drum);
        }
        else if (auto* kit = std::get_if<DrumKitConfig>(&chCfg.source))
        {
            changed |= ImGui::InputInt("DrumKit Note (0-127)", &state.selectedDrumNote);
            state.selectedDrumNote = std::clamp(state.selectedDrumNote, 0, 127);
            DrumConfig& d = kit->map[state.selectedDrumNote];
            changed |= DrawDrumConfigEditor("drum_kit", d);
        }
    }
    return changed;
}
} // namespace gui
