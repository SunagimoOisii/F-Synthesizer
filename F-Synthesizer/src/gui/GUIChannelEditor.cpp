#include "gui/GUIChannelEditor.h"

#include <algorithm>
#include <array>
#include <string>

#include <imgui.h>

#include "config/SourceRegistry.h"
#include "gui/GUIConfigUtils.h"
#include "gui/GUIMacroMapping.h"
#include "gui/GUIStateModel.h"

namespace
{
using HoverHelpFn = std::function<void(const char* what, const char* impact, const char* caution)>;

std::array<config::SourceKind, config::kSourceKindCount> BuildGuiSourceKindList(size_t& outCount)
{
    std::array<config::SourceKind, config::kSourceKindCount> kinds{};
    outCount = 0;
    for (int i = 0; i < config::kSourceKindCount; i++)
    {
        const config::SourceKind kind = config::SourceKindFromIndex(i);
        if (kind == config::SourceKind::Count)
        {
            continue;
        }

        const config::SourceCapability capability = config::SourceCapabilityOf(kind);
        // Soundタブでは one-shot 単発の Drum 直編集を避け、DrumKit を正規導線として残す。
        if (!capability.isPercussion || kind == config::SourceKind::DrumKit)
        {
            kinds[outCount++] = kind;
        }
    }
    if (outCount == 0)
    {
        kinds[outCount++] = config::SourceKind::Waveform;
    }
    return kinds;
}

void ApplyFmTemplateByAlgorithm(FmConfig& fm, int algorithm)
{
    fm.algorithm = std::clamp(algorithm, 0, 7);
    fm.filterMode = FilterMode::Bypass;
    fm.filterCutoffHz = 8000.0;
    fm.filterResonance = 0.707;
    fm.feedback = 0.0;

    for (auto& op : fm.ops)
    {
        op.wave = WaveType::Sine;
        op.ratio = 1.0;
        op.level = 1.0;
        op.index = 0.0;
    }

    switch (fm.algorithm)
    {
    case 1:
        // [M->C] + [M->C]
        fm.feedback = 0.14;
        fm.ops[0].ratio = 1.0; fm.ops[0].level = 1.0; fm.ops[0].index = 2.2;
        fm.ops[1].ratio = 1.0; fm.ops[1].level = 0.88; fm.ops[1].index = 0.0;
        fm.ops[2].ratio = 2.0; fm.ops[2].level = 0.95; fm.ops[2].index = 1.6;
        fm.ops[3].ratio = 1.0; fm.ops[3].level = 0.80; fm.ops[3].index = 0.0;
        break;
    case 2:
        // M -> [C + C + C]
        fm.feedback = 0.08;
        fm.ops[0].ratio = 2.0; fm.ops[0].level = 1.0; fm.ops[0].index = 2.8;
        fm.ops[1].ratio = 1.0; fm.ops[1].level = 0.84; fm.ops[1].index = 0.0;
        fm.ops[2].ratio = 2.0; fm.ops[2].level = 0.72; fm.ops[2].index = 0.0;
        fm.ops[3].ratio = 3.0; fm.ops[3].level = 0.66; fm.ops[3].index = 0.0;
        break;
    case 3:
        // M -> M -> M -> C
        fm.feedback = 0.22;
        fm.filterMode = FilterMode::LowPass;
        fm.filterCutoffHz = 3600.0;
        fm.filterResonance = 0.85;
        fm.ops[0].ratio = 1.0; fm.ops[0].level = 1.0; fm.ops[0].index = 3.2;
        fm.ops[1].ratio = 1.0; fm.ops[1].level = 1.0; fm.ops[1].index = 1.8;
        fm.ops[2].ratio = 1.0; fm.ops[2].level = 0.9; fm.ops[2].index = 0.9;
        fm.ops[3].ratio = 1.0; fm.ops[3].level = 0.82; fm.ops[3].index = 0.0;
        break;
    case 4:
        // [M->C] + [M->C] (2ペア)
        fm.feedback = 0.10;
        fm.ops[0].ratio = 1.0; fm.ops[0].level = 1.0; fm.ops[0].index = 2.0;
        fm.ops[1].ratio = 1.0; fm.ops[1].level = 0.85; fm.ops[1].index = 0.0;
        fm.ops[2].ratio = 2.0; fm.ops[2].level = 1.0; fm.ops[2].index = 1.6;
        fm.ops[3].ratio = 1.0; fm.ops[3].level = 0.82; fm.ops[3].index = 0.0;
        break;
    case 5:
        // M -> [C + C + C]
        fm.feedback = 0.12;
        fm.ops[0].ratio = 2.0; fm.ops[0].level = 1.0; fm.ops[0].index = 2.4;
        fm.ops[1].ratio = 1.0; fm.ops[1].level = 0.80; fm.ops[1].index = 0.0;
        fm.ops[2].ratio = 2.0; fm.ops[2].level = 0.74; fm.ops[2].index = 0.0;
        fm.ops[3].ratio = 3.0; fm.ops[3].level = 0.68; fm.ops[3].index = 0.0;
        break;
    case 6:
        // [M->C] + C + C
        fm.feedback = 0.10;
        fm.ops[0].ratio = 1.0; fm.ops[0].level = 1.0; fm.ops[0].index = 2.0;
        fm.ops[1].ratio = 1.0; fm.ops[1].level = 0.84; fm.ops[1].index = 0.0;
        fm.ops[2].ratio = 2.0; fm.ops[2].level = 0.72; fm.ops[2].index = 0.0;
        fm.ops[3].ratio = 3.0; fm.ops[3].level = 0.66; fm.ops[3].index = 0.0;
        break;
    case 7:
        // C + C + C + C
        fm.feedback = 0.08;
        fm.ops[0].ratio = 1.0; fm.ops[0].level = 0.82; fm.ops[0].index = 0.0;
        fm.ops[1].ratio = 2.0; fm.ops[1].level = 0.76; fm.ops[1].index = 0.0;
        fm.ops[2].ratio = 3.0; fm.ops[2].level = 0.70; fm.ops[2].index = 0.0;
        fm.ops[3].ratio = 4.0; fm.ops[3].level = 0.64; fm.ops[3].index = 0.0;
        break;
    case 0:
    default:
        // M -> C
        fm.feedback = 0.08;
        fm.ops[0].ratio = 2.0; fm.ops[0].level = 1.0; fm.ops[0].index = 2.4;
        fm.ops[1].ratio = 1.0; fm.ops[1].level = 0.88; fm.ops[1].index = 0.0;
        fm.ops[2].level = 0.0;
        fm.ops[3].level = 0.0;
        break;
    }

    fm.modulation = ModulationConfig{};
    fm.modulation.env2.attackSec = 0.0;
    fm.modulation.env2.decaySec = 0.12;
    fm.modulation.env2.sustainLevel = 0.0;
    fm.modulation.env2.releaseSec = 0.08;
    for (auto& route : fm.modulation.matrix.routes)
    {
        route = ModRoute{};
    }
    fm.modulation.matrix.routes[0] = ModRoute{
        ModSource::Env2,
        ModDestination::FmIndex,
        0.8,
        true
    };
}

bool DrawDrumConfigEditor(const char* IDPrefix, DrumConfig& d, const HoverHelpFn& updateHoverHelp)
{
    bool changed = false;
    int drumType = static_cast<int>(d.type);
    const char* drumTypes[] = { "none", "kick", "snare", "hat" };
    std::string key = std::string("Drum Type##") + IDPrefix;
    changed |= ImGui::Combo(key.c_str(), &drumType, drumTypes, IM_ARRAYSIZE(drumTypes));
    if (updateHoverHelp)
    {
        updateHoverHelp(
            "Drum Type を選択します。",
            "ドラム発音モデル（none/kick/snare/hat）が切り替わります。",
            nullptr);
    }
    ImGui::TextDisabled("DrumConfig: 0 = 未指定（内部デフォルト）");
    if (updateHoverHelp)
    {
        updateHoverHelp(
            "DrumConfig の未指定値ルールを確認します。",
            "数値パラメータを 0 にすると内部デフォルトが使われます。",
            "明示値に戻す場合は 0 以外の値を入力してください。");
    }
    d.type = static_cast<DrumType>(drumType);

    if (d.type == DrumType::None)
    {
        return changed;
    }

    key = std::string("Gain##") + IDPrefix;
    changed |= ImGui::InputDouble(key.c_str(), &d.gain, 0.01, 0.1, "%.3f");
    if (updateHoverHelp)
    {
        updateHoverHelp(
            "Drum Gain を調整します。",
            "該当ドラム音の音量が変わります。",
            "上げすぎるとクリップしやすくなります。");
    }

    if (d.type == DrumType::Kick)
    {
        key = std::string("Base Freq##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.baseFreq, 1.0, 10.0, "%.2f");
        if (updateHoverHelp) updateHoverHelp("Base Freq を調整します。", "キックの基音が変わります。", nullptr);
        key = std::string("Pitch Drop##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.pitchDrop, 0.1, 1.0, "%.3f");
        if (updateHoverHelp) updateHoverHelp("Pitch Drop を調整します。", "キックのピッチ下降量が変わります。", nullptr);
        key = std::string("Pitch Decay##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.pitchDecaySec, 0.01, 0.1, "%.3f");
        if (updateHoverHelp) updateHoverHelp("Pitch Decay を調整します。", "キックのピッチ変化速度が変わります。", nullptr);
    }
    else // Snare, Hat
    {
        key = std::string("Tone Freq##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.toneFreq, 10.0, 100.0, "%.2f");
        if (updateHoverHelp) updateHoverHelp("Tone Freq を調整します。", "スネア/ハットの有音成分周波数が変わります。", nullptr);
        key = std::string("Tone Level##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.toneLevel, 0.01, 0.1, "%.3f");
        if (updateHoverHelp) updateHoverHelp("Tone Level を調整します。", "有音成分の音量が変わります。", nullptr);
        key = std::string("Noise Level##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.noiseLevel, 0.01, 0.1, "%.3f");
        if (updateHoverHelp) updateHoverHelp("Noise Level を調整します。", "ノイズ成分の音量が変わります。", nullptr);
        key = std::string("HP Cut##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.hpCut, 10.0, 100.0, "%.2f");
        if (updateHoverHelp) updateHoverHelp("HP Cut を調整します。", "高域寄りに残す帯域が変わります。", nullptr);
        key = std::string("LP Cut##") + IDPrefix; changed |= ImGui::InputDouble(key.c_str(), &d.lpCut, 10.0, 100.0, "%.2f");
        if (updateHoverHelp) updateHoverHelp("LP Cut を調整します。", "低域寄りに残す帯域が変わります。", nullptr);

        int toneWave = d.toneWave;
        const char* waves[] = { "sine", "square", "saw", "triangle" };
        key = std::string("Tone Wave##") + IDPrefix;
        changed |= ImGui::Combo(key.c_str(), &toneWave, waves, IM_ARRAYSIZE(waves));
        if (updateHoverHelp) updateHoverHelp("Tone Wave を選択します。", "有音成分の波形キャラクターが変わります。", nullptr);
        d.toneWave = toneWave;

        int noiseType = d.noiseType;
        const char* noises[] = { "white", "pink", "brown", "blue" };
        key = std::string("Noise Type##") + IDPrefix;
        changed |= ImGui::Combo(key.c_str(), &noiseType, noises, IM_ARRAYSIZE(noises));
        if (updateHoverHelp) updateHoverHelp("Noise Type を選択します。", "ノイズの色（周波数傾向）が変わります。", nullptr);
        d.noiseType = noiseType;
    }
    return changed;
}
} // namespace

namespace gui
{
bool DrawChannelEditor(
    GUIState& state,
    bool showSourceTypeSelector,
    const std::function<void(const char* what, const char* impact, const char* caution)>& updateHoverHelp)
{
    bool changed = false;
    EnsureChannelConfigs(state);
    state.selectedSoundSlot = std::clamp(state.selectedSoundSlot, 0, 15);

    ImGui::TextUnformatted("Sound Slot");
    changed |= ImGui::InputInt("Selected Sound Slot (0-15)", &state.selectedSoundSlot);
    if (updateHoverHelp)
    {
        updateHoverHelp(
            "編集対象のSound Slotを指定します。",
            "この後の音色編集が適用されるスロットが変わります。",
            nullptr);
    }
    state.selectedSoundSlot = std::clamp(state.selectedSoundSlot, 0, 15);
    const int prChannel = std::clamp(state.pianoRoll.displayChannel, 0, 15);
    const int assignedSlot = std::clamp(state.channelAssignments[prChannel], 0, 15);
    ImGui::TextDisabled("PR Channel ch%d -> Assigned Slot s%d", prChannel, assignedSlot);
    ImGui::SameLine();
    if (ImGui::Button("Edit Assigned Slot of PR ch"))
    {
        state.selectedSoundSlot = assignedSlot;
    }
    if (updateHoverHelp)
    {
        updateHoverHelp(
            "PR ch に割り当てられたSound Slotへ編集対象を移します。",
            "Music側の表示chで鳴る音色を直接編集できます。",
            nullptr);
    }

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
#include "channeleditor/ChannelEditorModulation.inl"
    ImGui::TextDisabled("Sound tab edits sound definitions only. Channel mix/assign is in Music tab.");

    ImGui::Separator();
    ChannelConfig& chCfg = (*state.channelConfigs)[state.selectedSoundSlot];
    ImGui::Text("Selected Sound Slot: s%d", state.selectedSoundSlot);
    ImGui::TextDisabled("Tone preview uses selected sound slot.");

    if (ImGui::CollapsingHeader("Envelope / Gain", ImGuiTreeNodeFlags_DefaultOpen))
    {
        changed |= ImGui::InputDouble("Amp", &chCfg.amp, 0.01, 0.1, "%.3f");
        if (updateHoverHelp) updateHoverHelp("Amp を調整します。", "音量スケールが変わります。", "上げすぎるとクリップしやすくなります。");
        changed |= ImGui::InputDouble("Attack", &chCfg.attackSec, 0.01, 0.1, "%.3f");
        if (updateHoverHelp) updateHoverHelp("Attack を調整します。", "立ち上がり時間が変わります。", nullptr);
        changed |= ImGui::InputDouble("Decay", &chCfg.decaySec, 0.01, 0.1, "%.3f");
        if (updateHoverHelp) updateHoverHelp("Decay を調整します。", "サステインまでの減衰時間が変わります。", nullptr);
        changed |= ImGui::InputDouble("Sustain", &chCfg.sustainLevel, 0.01, 0.1, "%.3f");
        if (updateHoverHelp) updateHoverHelp("Sustain を調整します。", "押下維持中の音量が変わります。", nullptr);
        changed |= ImGui::InputDouble("Release", &chCfg.releaseSec, 0.01, 0.1, "%.3f");
        if (updateHoverHelp) updateHoverHelp("Release を調整します。", "ノートオフ後の余韻時間が変わります。", nullptr);
    }

    bool layer3Changed = false;
    if (ImGui::CollapsingHeader("Source Details", ImGuiTreeNodeFlags_DefaultOpen))
    {
        const bool changedBeforeSourceDetails = changed;
        if (showSourceTypeSelector)
        {
            const config::SourceKind selectedKind = config::SourceConfigKind(chCfg.source);
            size_t guiSourceKindCount = 0;
            const auto guiSourceKinds = BuildGuiSourceKindList(guiSourceKindCount);
            int srcType = 0;
            for (size_t i = 0; i < guiSourceKindCount; i++)
            {
                if (guiSourceKinds[i] == selectedKind)
                {
                    srcType = static_cast<int>(i);
                    break;
                }
            }
            if (ImGui::BeginCombo("Source Type", config::SourceKindToDisplayName(guiSourceKinds[static_cast<size_t>(srcType)])))
            {
                for (size_t i = 0; i < guiSourceKindCount; i++)
                {
                    const config::SourceKind candidate = guiSourceKinds[i];
                    const bool selected = (srcType == static_cast<int>(i));
                    if (ImGui::Selectable(config::SourceKindToDisplayName(candidate), selected))
                    {
                        srcType = static_cast<int>(i);
                        changed = true;
                        chCfg.source = DefaultSourceByType(config::SourceKindToIndex(candidate));
                    }
                    if (selected)
                    {
                        ImGui::SetItemDefaultFocus();
                    }
                }
                ImGui::EndCombo();
            }
            if (updateHoverHelp)
            {
                updateHoverHelp(
                    "音源タイプを切り替えます。",
                    "Waveform/Analog/Noise/FM/DrumKit/PSG の編集対象に切り替わります。",
                    "切替時は該当タイプの既定設定で初期化されます。");
            }
        }

#include "channeleditor/ChannelEditorCommon.inl"

#include "channeleditor/ChannelEditorWaveform.inl"
#include "channeleditor/ChannelEditorNoise.inl"
#include "channeleditor/ChannelEditorFm.inl"
#include "channeleditor/ChannelEditorDrum.inl"
        layer3Changed = (changed && !changedBeforeSourceDetails);
    }
    if (layer3Changed)
    {
        // Layer2 マクロスライダーを Layer3 編集に追従させる。
        const int ch = std::clamp(state.selectedSoundSlot, 0, 15);
        state.macroSliders[ch] = ReadMacroSliders((*state.channelConfigs)[ch], state.macroSliders[ch]);
    }
    return changed;
}
} // namespace gui
