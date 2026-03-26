#include "gui/GUIChannelEditor.h"

#include <algorithm>
#include <array>
#include <string>

#include <imgui.h>

#include "config/SourceRegistry.h"
#include "gui/GUIConfigUtils.h"
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
    auto drawModulationEditor = [&](const char* idPrefix,
        ModulationConfig& modulation,
        bool allowFilterCutoff,
        bool allowFmIndex) -> bool
    {
        bool localChanged = false;
        ImGui::Separator();
        ImGui::TextUnformatted("Modulation");

        const char* lfoWaves[] = { "sine", "triangle" };
        int lfoWaveIdx = (modulation.lfo1.wave == LfoWave::Triangle) ? 1 : 0;
        ImGui::SetNextItemWidth(220.0f);
        if (ImGui::Combo("LFO1 Wave", &lfoWaveIdx, lfoWaves, IM_ARRAYSIZE(lfoWaves)))
        {
            modulation.lfo1.wave = (lfoWaveIdx == 1) ? LfoWave::Triangle : LfoWave::Sine;
            localChanged = true;
        }
        ImGui::SetNextItemWidth(220.0f);
        localChanged |= sliderWaveParam("LFO1 Rate (Hz)", modulation.lfo1.rateHz, 0.0f, 100.0f, "%.2f");
        if (updateHoverHelp) updateHoverHelp("LFO1 Rate を調整します。", "周期変調の速さが変わります。", nullptr);
        ImGui::SetNextItemWidth(220.0f);
        localChanged |= sliderWaveParam("LFO1 Depth", modulation.lfo1.depth, 0.0f, 1.0f, "%.3f");
        if (updateHoverHelp) updateHoverHelp("LFO1 Depth を調整します。", "LFOの変調量が変わります。", nullptr);
        localChanged |= ImGui::Checkbox("LFO1 Bipolar", &modulation.lfo1.bipolar);
        if (updateHoverHelp) updateHoverHelp("LFO1 Bipolar を切り替えます。", "LFO出力の極性レンジが変わります。", nullptr);

        ImGui::SetNextItemWidth(220.0f);
        localChanged |= sliderWaveParam("Env2 Attack", modulation.env2.attackSec, 0.0f, 10.0f, "%.3f");
        ImGui::SetNextItemWidth(220.0f);
        localChanged |= sliderWaveParam("Env2 Decay", modulation.env2.decaySec, 0.0f, 10.0f, "%.3f");
        ImGui::SetNextItemWidth(220.0f);
        localChanged |= sliderWaveParam("Env2 Sustain", modulation.env2.sustainLevel, 0.0f, 1.0f, "%.3f");
        ImGui::SetNextItemWidth(220.0f);
        localChanged |= sliderWaveParam("Env2 Release", modulation.env2.releaseSec, 0.0f, 10.0f, "%.3f");

        const char* modSources[] = { "none", "lfo1", "env2", "velocity", "channelPressure", "polyPressure" };
        struct DestinationChoice
        {
            const char* label;
            ModDestination value;
        };
        std::array<DestinationChoice, 6> destinationChoices{ {
            { "none", ModDestination::None },
            { "pitchMul", ModDestination::Pitch },
            { "amp", ModDestination::Amp },
            { "filterCutoffHz", ModDestination::FilterCutoff },
            { "filterResonance", ModDestination::FilterResonance },
            { "fm.index", ModDestination::FmIndex },
        } };
        int destinationCount = 3;
        if (allowFilterCutoff)
        {
            destinationCount += 2;
        }
        if (allowFmIndex)
        {
            destinationChoices[destinationCount++] = { "fm.index", ModDestination::FmIndex };
        }

        auto destinationLabel = [&](ModDestination destination) -> const char*
        {
            for (int i = 0; i < destinationCount; i++)
            {
                if (destinationChoices[i].value == destination)
                {
                    return destinationChoices[i].label;
                }
            }
            return "none";
        };
        auto destinationIndex = [&](ModDestination destination) -> int
        {
            for (int i = 0; i < destinationCount; i++)
            {
                if (destinationChoices[i].value == destination)
                {
                    return i;
                }
            }
            return 0;
        };

        ImGui::PushID(idPrefix);
        const int routeCount = static_cast<int>(modulation.matrix.routes.size());
        for (int routeIdx = 0; routeIdx < routeCount; routeIdx++)
        {
            ModRoute& route = modulation.matrix.routes[static_cast<size_t>(routeIdx)];
            ImGui::PushID(routeIdx);

            std::string summary;
            bool hasRoute = (route.source != ModSource::None && route.destination != ModDestination::None);
            if (hasRoute)
            {
                char amtBuf[16];
                snprintf(amtBuf, sizeof(amtBuf), "%+.2f", static_cast<float>(route.amount));
                summary = "Route " + std::to_string(routeIdx) + ": "
                    + modSources[static_cast<int>(route.source)]
                    + " -> "
                    + destinationLabel(route.destination)
                    + " (" + amtBuf + ")"
                    + (route.enabled ? "" : " [off]");
            }
            else
            {
                summary = "Route " + std::to_string(routeIdx) + ": (empty)";
            }

            if (!ImGui::CollapsingHeader(summary.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::PopID();
                continue;
            }

            localChanged |= ImGui::Checkbox("Enabled", &route.enabled);
            if (updateHoverHelp) updateHoverHelp("Route Enabled を切り替えます。", "このモジュレーション経路の有効/無効が変わります。", nullptr);

            int srcIdx = 0;
            switch (route.source)
            {
            case ModSource::None: srcIdx = 0; break;
            case ModSource::Lfo1: srcIdx = 1; break;
            case ModSource::Env2: srcIdx = 2; break;
            case ModSource::Velocity: srcIdx = 3; break;
            case ModSource::ChannelPressure: srcIdx = 4; break;
            case ModSource::PolyPressure: srcIdx = 5; break;
            }
            ImGui::SetNextItemWidth(220.0f);
            if (ImGui::Combo("Source", &srcIdx, modSources, IM_ARRAYSIZE(modSources)))
            {
                switch (srcIdx)
                {
                case 0: route.source = ModSource::None; break;
                case 1: route.source = ModSource::Lfo1; break;
                case 2: route.source = ModSource::Env2; break;
                case 3: route.source = ModSource::Velocity; break;
                case 4: route.source = ModSource::ChannelPressure; break;
                case 5: route.source = ModSource::PolyPressure; break;
                default: route.source = ModSource::None; break;
                }
                localChanged = true;
            }
            if (updateHoverHelp) updateHoverHelp("Route Source を選択します。", "変調元が変わります。", nullptr);

            int dstIdx = destinationIndex(route.destination);
            const char* destinationLabels[5] = {};
            for (int i = 0; i < destinationCount; i++)
            {
                destinationLabels[i] = destinationChoices[i].label;
            }
            ImGui::SetNextItemWidth(220.0f);
            if (ImGui::Combo("Destination", &dstIdx, destinationLabels, destinationCount))
            {
                route.destination = destinationChoices[dstIdx].value;
                localChanged = true;
            }
            if (updateHoverHelp) updateHoverHelp("Route Destination を選択します。", "変調先パラメータが変わります。", nullptr);
            ImGui::SetNextItemWidth(220.0f);
            localChanged |= sliderWaveParam("Amount", route.amount, -1.0f, 1.0f, "%.3f");
            if (updateHoverHelp) updateHoverHelp("Route Amount を調整します。", "変調量と極性が変わります。", nullptr);
            ImGui::Separator();
            ImGui::PopID();
        }
        ImGui::PopID();
        return localChanged;
    };

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

    if (ImGui::CollapsingHeader("Source Details", ImGuiTreeNodeFlags_DefaultOpen))
    {
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

        if (auto* wf = std::get_if<WaveformConfig>(&chCfg.source))
        {
            int idx = WaveToIndex(wf->wave);
            const char* waves[] = { "sine", "square", "saw", "triangle" };
            changed |= ImGui::Combo("Wave", &idx, waves, IM_ARRAYSIZE(waves));
            if (updateHoverHelp) updateHoverHelp("Wave を選択します。", "基本波形キャラクターが変わります。", nullptr);
            wf->wave = WaveFromIndex(idx);

            wf->unisonVoices = std::clamp(wf->unisonVoices, 1, 8);
            wf->unisonDetuneCents = std::clamp(wf->unisonDetuneCents, 0.0, 120.0);
            wf->unisonSpread = std::clamp(wf->unisonSpread, 0.0, 1.0);
            wf->subOscLevel = std::clamp(wf->subOscLevel, 0.0, 2.0);
            wf->filterCutoffHz = std::clamp(wf->filterCutoffHz, 10.0, 20000.0);
            wf->filterResonance = std::clamp(wf->filterResonance, 0.1, 18.0);
            wf->filterKeytrack = std::clamp(wf->filterKeytrack, 0.0, 1.0);
            wf->smoothing.ampTimeMs = std::clamp(wf->smoothing.ampTimeMs, 0.0, 1000.0);
            wf->smoothing.pitchTimeMs = std::clamp(wf->smoothing.pitchTimeMs, 0.0, 1000.0);
            wf->smoothing.filterCutoffTimeMs = std::clamp(wf->smoothing.filterCutoffTimeMs, 0.0, 1000.0);
            wf->modulation.lfo1.rateHz = std::clamp(wf->modulation.lfo1.rateHz, 0.0, 100.0);
            wf->modulation.lfo1.depth = std::clamp(wf->modulation.lfo1.depth, 0.0, 1.0);
            wf->modulation.env2.attackSec = std::clamp(wf->modulation.env2.attackSec, 0.0, 10.0);
            wf->modulation.env2.decaySec = std::clamp(wf->modulation.env2.decaySec, 0.0, 10.0);
            wf->modulation.env2.sustainLevel = std::clamp(wf->modulation.env2.sustainLevel, 0.0, 1.0);
            wf->modulation.env2.releaseSec = std::clamp(wf->modulation.env2.releaseSec, 0.0, 10.0);
            for (auto& route : wf->modulation.matrix.routes)
            {
                route.amount = std::clamp(route.amount, -1.0, 1.0);
            }

            int unisonVoices = wf->unisonVoices;
            ImGui::SetNextItemWidth(220.0f);
            if (ImGui::SliderInt("Unison Voices", &unisonVoices, 1, 8))
            {
                wf->unisonVoices = unisonVoices;
                changed = true;
            }
            if (updateHoverHelp) updateHoverHelp("Unison Voices を調整します。", "重ねる発音数が変わり厚みが変わります。", "増やすほどCPU負荷が上がります。");
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Unison Detune (cent)", wf->unisonDetuneCents, 0.0f, 120.0f, "%.1f");
            if (updateHoverHelp) updateHoverHelp("Unison Detune を調整します。", "重ね音のピッチ差が変わります。", nullptr);
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Unison Spread", wf->unisonSpread, 0.0f, 1.0f, "%.2f");
            if (updateHoverHelp) updateHoverHelp("Unison Spread を調整します。", "ステレオの広がりが変わります。", nullptr);
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Sub Osc Level", wf->subOscLevel, 0.0f, 2.0f, "%.2f");
            if (updateHoverHelp) updateHoverHelp("Sub Osc Level を調整します。", "低域補助成分の音量が変わります。", nullptr);

            const char* filterModes[] = { "bypass", "lowpass", "highpass", "bandpass" };
            int filterModeIdx = 0;
            switch (wf->filterMode)
            {
            case FilterMode::Bypass: filterModeIdx = 0; break;
            case FilterMode::LowPass: filterModeIdx = 1; break;
            case FilterMode::HighPass: filterModeIdx = 2; break;
            case FilterMode::BandPass: filterModeIdx = 3; break;
            }
            ImGui::SetNextItemWidth(220.0f);
            if (ImGui::Combo("Filter Mode", &filterModeIdx, filterModes, IM_ARRAYSIZE(filterModes)))
            {
                switch (filterModeIdx)
                {
                case 0: wf->filterMode = FilterMode::Bypass; break;
                case 1: wf->filterMode = FilterMode::LowPass; break;
                case 2: wf->filterMode = FilterMode::HighPass; break;
                case 3: wf->filterMode = FilterMode::BandPass; break;
                default: wf->filterMode = FilterMode::Bypass; break;
                }
                changed = true;
            }
            if (updateHoverHelp) updateHoverHelp("Filter Mode を選択します。", "フィルタ有効/種別が変わります。", nullptr);
            if (wf->filterMode != FilterMode::Bypass)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "(active)");
            }
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Filter Cutoff (Hz)", wf->filterCutoffHz, 10.0f, 20000.0f, "%.1f");
            if (updateHoverHelp) updateHoverHelp("Filter Cutoff を調整します。", "通過帯域の中心が変わります。", nullptr);
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Filter Resonance (Q)", wf->filterResonance, 0.1f, 18.0f, "%.2f");
            if (updateHoverHelp) updateHoverHelp("Filter Resonance を調整します。", "カットオフ付近の強調量が変わります。", nullptr);
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Filter Keytrack", wf->filterKeytrack, 0.0f, 1.0f, "%.2f");
            if (updateHoverHelp) updateHoverHelp("Filter Keytrack を調整します。", "ノート音程に連動してカットオフが動く量が変わります。基準は C4(60)。", nullptr);

            ImGui::Separator();
            ImGui::TextUnformatted("Smoothing");
            changed |= ImGui::Checkbox("Smoothing Enabled", &wf->smoothing.enabled);
            if (updateHoverHelp) updateHoverHelp("Smoothing Enabled を切り替えます。", "パラメータ変化の段差を抑えます。", nullptr);
            changed |= ImGui::Checkbox("Pitch Smoothing Enabled", &wf->smoothing.pitchEnabled);
            if (updateHoverHelp) updateHoverHelp("Pitch Smoothing Enabled を切り替えます。", "ピッチ変化の滑らかさが変わります。", nullptr);
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Amp Smoothing (ms)", wf->smoothing.ampTimeMs, 0.0f, 1000.0f, "%.1f");
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Pitch Smoothing (ms)", wf->smoothing.pitchTimeMs, 0.0f, 1000.0f, "%.1f");
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Filter Smoothing (ms)", wf->smoothing.filterCutoffTimeMs, 0.0f, 1000.0f, "%.1f");

            changed |= drawModulationEditor("waveform_modulation", wf->modulation, true, false);
        }
        else if (auto* analog = std::get_if<AnalogConfig>(&chCfg.source))
        {
            int idx = WaveToIndex(analog->wave);
            const char* waves[] = { "sine", "square", "saw", "triangle" };
            changed |= ImGui::Combo("Wave", &idx, waves, IM_ARRAYSIZE(waves));
            if (updateHoverHelp) updateHoverHelp("Wave を選択します。", "基本波形キャラクターが変わります。", nullptr);
            analog->wave = WaveFromIndex(idx);

            analog->unisonVoices = std::clamp(analog->unisonVoices, 1, 8);
            analog->unisonDetuneCents = std::clamp(analog->unisonDetuneCents, 0.0, 120.0);
            analog->unisonSpread = std::clamp(analog->unisonSpread, 0.0, 1.0);
            analog->subOscLevel = std::clamp(analog->subOscLevel, 0.0, 2.0);
            analog->filterCutoffHz = std::clamp(analog->filterCutoffHz, 10.0, 20000.0);
            analog->filterResonance = std::clamp(analog->filterResonance, 0.1, 18.0);
            analog->filterKeytrack = std::clamp(analog->filterKeytrack, 0.0, 1.0);
            analog->drive = std::clamp(analog->drive, 0.0, 1.0);
            analog->driftDepthCents = std::clamp(analog->driftDepthCents, 0.0, 20.0);
            analog->driftRateHz = std::clamp(analog->driftRateHz, 0.01, 2.0);
            analog->smoothing.ampTimeMs = std::clamp(analog->smoothing.ampTimeMs, 0.0, 1000.0);
            analog->smoothing.pitchTimeMs = std::clamp(analog->smoothing.pitchTimeMs, 0.0, 1000.0);
            analog->smoothing.filterCutoffTimeMs = std::clamp(analog->smoothing.filterCutoffTimeMs, 0.0, 1000.0);
            analog->modulation.lfo1.rateHz = std::clamp(analog->modulation.lfo1.rateHz, 0.0, 100.0);
            analog->modulation.lfo1.depth = std::clamp(analog->modulation.lfo1.depth, 0.0, 1.0);
            analog->modulation.env2.attackSec = std::clamp(analog->modulation.env2.attackSec, 0.0, 10.0);
            analog->modulation.env2.decaySec = std::clamp(analog->modulation.env2.decaySec, 0.0, 10.0);
            analog->modulation.env2.sustainLevel = std::clamp(analog->modulation.env2.sustainLevel, 0.0, 1.0);
            analog->modulation.env2.releaseSec = std::clamp(analog->modulation.env2.releaseSec, 0.0, 10.0);
            for (auto& route : analog->modulation.matrix.routes)
            {
                route.amount = std::clamp(route.amount, -1.0, 1.0);
            }

            int unisonVoices = analog->unisonVoices;
            ImGui::SetNextItemWidth(220.0f);
            if (ImGui::SliderInt("Unison Voices", &unisonVoices, 1, 8))
            {
                analog->unisonVoices = unisonVoices;
                changed = true;
            }
            if (updateHoverHelp) updateHoverHelp("Unison Voices を調整します。", "重ねる発音数が変わり厚みが変わります。", "増やすほどCPU負荷が上がります。");
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Unison Detune (cent)", analog->unisonDetuneCents, 0.0f, 120.0f, "%.1f");
            if (updateHoverHelp) updateHoverHelp("Unison Detune を調整します。", "重ね音のピッチ差が変わります。", nullptr);
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Unison Spread", analog->unisonSpread, 0.0f, 1.0f, "%.2f");
            if (updateHoverHelp) updateHoverHelp("Unison Spread を調整します。", "ステレオの広がりが変わります。", nullptr);
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Sub Osc Level", analog->subOscLevel, 0.0f, 2.0f, "%.2f");
            if (updateHoverHelp) updateHoverHelp("Sub Osc Level を調整します。", "低域補助成分の音量が変わります。", nullptr);

            const char* filterModes[] = { "bypass", "lowpass", "highpass", "bandpass" };
            int filterModeIdx = 0;
            switch (analog->filterMode)
            {
            case FilterMode::Bypass: filterModeIdx = 0; break;
            case FilterMode::LowPass: filterModeIdx = 1; break;
            case FilterMode::HighPass: filterModeIdx = 2; break;
            case FilterMode::BandPass: filterModeIdx = 3; break;
            }
            ImGui::SetNextItemWidth(220.0f);
            if (ImGui::Combo("Filter Mode", &filterModeIdx, filterModes, IM_ARRAYSIZE(filterModes)))
            {
                switch (filterModeIdx)
                {
                case 0: analog->filterMode = FilterMode::Bypass; break;
                case 1: analog->filterMode = FilterMode::LowPass; break;
                case 2: analog->filterMode = FilterMode::HighPass; break;
                case 3: analog->filterMode = FilterMode::BandPass; break;
                default: analog->filterMode = FilterMode::Bypass; break;
                }
                changed = true;
            }
            if (updateHoverHelp) updateHoverHelp("Filter Mode を選択します。", "フィルタ有効/種別が変わります。", nullptr);
            if (analog->filterMode != FilterMode::Bypass)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "(active)");
            }
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Filter Cutoff (Hz)", analog->filterCutoffHz, 10.0f, 20000.0f, "%.1f");
            if (updateHoverHelp) updateHoverHelp("Filter Cutoff を調整します。", "通過帯域の中心が変わります。", nullptr);
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Filter Resonance (Q)", analog->filterResonance, 0.1f, 18.0f, "%.2f");
            if (updateHoverHelp) updateHoverHelp("Filter Resonance を調整します。", "カットオフ付近の強調量が変わります。", nullptr);
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Filter Keytrack", analog->filterKeytrack, 0.0f, 1.0f, "%.2f");
            if (updateHoverHelp) updateHoverHelp("Filter Keytrack を調整します。", "ノート音程に連動してカットオフが動く量が変わります。基準は C4(60)。", nullptr);

            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Drive", analog->drive, 0.0f, 1.0f, "%.3f");
            if (updateHoverHelp) updateHoverHelp("Drive を調整します。", "ソフトクリップ量が変わります。", "上げすぎると飽和が強くなります。");

            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Drift Depth (cent)", analog->driftDepthCents, 0.0f, 20.0f, "%.2f");
            if (updateHoverHelp) updateHoverHelp("Drift Depth を調整します。", "ピッチ揺らぎ量が変わります。", nullptr);
            ImGui::SetNextItemWidth(220.0f);
            float driftRateHz = static_cast<float>(analog->driftRateHz);
            if (ImGui::SliderFloat("Drift Rate (Hz)", &driftRateHz, 0.01f, 2.0f, "%.3f", ImGuiSliderFlags_Logarithmic))
            {
                analog->driftRateHz = static_cast<double>(driftRateHz);
                changed = true;
            }
            if (updateHoverHelp) updateHoverHelp("Drift Rate を調整します。", "ドリフトLFOの速度が変わります。", nullptr);

            ImGui::Separator();
            ImGui::TextUnformatted("Smoothing");
            changed |= ImGui::Checkbox("Smoothing Enabled", &analog->smoothing.enabled);
            if (updateHoverHelp) updateHoverHelp("Smoothing Enabled を切り替えます。", "パラメータ変化の段差を抑えます。", nullptr);
            changed |= ImGui::Checkbox("Pitch Smoothing Enabled", &analog->smoothing.pitchEnabled);
            if (updateHoverHelp) updateHoverHelp("Pitch Smoothing Enabled を切り替えます。", "ピッチ変化の滑らかさが変わります。", nullptr);
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Amp Smoothing (ms)", analog->smoothing.ampTimeMs, 0.0f, 1000.0f, "%.1f");
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Pitch Smoothing (ms)", analog->smoothing.pitchTimeMs, 0.0f, 1000.0f, "%.1f");
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Filter Smoothing (ms)", analog->smoothing.filterCutoffTimeMs, 0.0f, 1000.0f, "%.1f");

            changed |= drawModulationEditor("analog_modulation", analog->modulation, true, false);
        }
        else if (auto* nz = std::get_if<NoiseConfig>(&chCfg.source))
        {
            nz->filterCutoffHz = std::clamp(nz->filterCutoffHz, 10.0, 20000.0);
            nz->filterResonance = std::clamp(nz->filterResonance, 0.1, 18.0);

            int idx = NoiseToIndex(nz->noise);
            const char* noises[] = { "white", "pink", "brown", "blue" };
            changed |= ImGui::Combo("Noise", &idx, noises, IM_ARRAYSIZE(noises));
            if (updateHoverHelp) updateHoverHelp("Noise を選択します。", "ノイズ種別（色）が変わります。", nullptr);
            nz->noise = NoiseFromIndex(idx);

            const char* filterModes[] = { "bypass", "lowpass", "highpass", "bandpass" };
            int filterModeIdx = 0;
            switch (nz->filterMode)
            {
            case FilterMode::Bypass: filterModeIdx = 0; break;
            case FilterMode::LowPass: filterModeIdx = 1; break;
            case FilterMode::HighPass: filterModeIdx = 2; break;
            case FilterMode::BandPass: filterModeIdx = 3; break;
            }
            ImGui::SetNextItemWidth(220.0f);
            if (ImGui::Combo("Filter Mode", &filterModeIdx, filterModes, IM_ARRAYSIZE(filterModes)))
            {
                switch (filterModeIdx)
                {
                case 0: nz->filterMode = FilterMode::Bypass; break;
                case 1: nz->filterMode = FilterMode::LowPass; break;
                case 2: nz->filterMode = FilterMode::HighPass; break;
                case 3: nz->filterMode = FilterMode::BandPass; break;
                default: nz->filterMode = FilterMode::Bypass; break;
                }
                changed = true;
            }
            if (updateHoverHelp) updateHoverHelp("Filter Mode を選択します。", "フィルタ有効/種別が変わります。", nullptr);
            if (nz->filterMode != FilterMode::Bypass)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "(active)");
            }
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Filter Cutoff (Hz)", nz->filterCutoffHz, 10.0f, 20000.0f, "%.1f");
            if (updateHoverHelp) updateHoverHelp("Filter Cutoff を調整します。", "通過帯域の中心が変わります。", nullptr);
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Filter Resonance (Q)", nz->filterResonance, 0.1f, 18.0f, "%.2f");
            if (updateHoverHelp) updateHoverHelp("Filter Resonance を調整します。", "カットオフ付近の強調量が変わります。", nullptr);
        }
        else if (auto* fm = std::get_if<FmConfig>(&chCfg.source))
        {
            fm->algorithm = std::clamp(fm->algorithm, 0, 7);
            fm->feedback = std::clamp(fm->feedback, 0.0, 1.0);
            for (auto& op : fm->ops)
            {
                op.ratio = std::clamp(op.ratio, 0.0, 32.0);
                op.level = std::clamp(op.level, 0.0, 1.0);
                op.index = std::clamp(op.index, 0.0, 32.0);
            }
            fm->filterCutoffHz = std::clamp(fm->filterCutoffHz, 10.0, 20000.0);
            fm->filterResonance = std::clamp(fm->filterResonance, 0.1, 18.0);
            fm->modulation.lfo1.rateHz = std::clamp(fm->modulation.lfo1.rateHz, 0.0, 100.0);
            fm->modulation.lfo1.depth = std::clamp(fm->modulation.lfo1.depth, 0.0, 1.0);
            fm->modulation.env2.attackSec = std::clamp(fm->modulation.env2.attackSec, 0.0, 10.0);
            fm->modulation.env2.decaySec = std::clamp(fm->modulation.env2.decaySec, 0.0, 10.0);
            fm->modulation.env2.sustainLevel = std::clamp(fm->modulation.env2.sustainLevel, 0.0, 1.0);
            fm->modulation.env2.releaseSec = std::clamp(fm->modulation.env2.releaseSec, 0.0, 10.0);
            for (auto& route : fm->modulation.matrix.routes)
            {
                route.amount = std::clamp(route.amount, -1.0, 1.0);
            }

            const char* algoLabels[] = {
                "0: M->C  (2op compat)",
                "1: [M->C]+[M->C]  (2-pair)",
                "2: M->[C+C+C]  (1mod 3car)",
                "3: M->M->M->C  (chain)",
                "4: [M->C]+[M->C]  (dual pair)",
                "5: M->[C+C+C]  (triple car)",
                "6: [M->C]+C+C  (hybrid)",
                "7: C+C+C+C  (all car)"
            };
            changed |= ImGui::Combo("FM Algorithm", &fm->algorithm, algoLabels, IM_ARRAYSIZE(algoLabels));
            if (updateHoverHelp) updateHoverHelp("FM アルゴリズムを選択します。", "オペレータの接続構造が変わります。", nullptr);
            ImGui::SameLine();
            if (ImGui::Button("テンプレートに戻す"))
            {
                ApplyFmTemplateByAlgorithm(*fm, fm->algorithm);
                changed = true;
            }
            if (updateHoverHelp)
            {
                updateHoverHelp(
                    "現在アルゴリズムの推奨テンプレートへ戻します。",
                    "FMオペレータ/フィルタ/変調の初期値を安全域へ復帰します。",
                    "現在の微調整値は上書きされます。");
            }

            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Feedback", fm->feedback, 0.0f, 1.0f, "%.2f");
            if (updateHoverHelp) updateHoverHelp("Op1 の自己フィードバック量です。", "大きくするとサチュレーション気味になります。", nullptr);

            const char* waves[] = { "sine", "square", "saw", "triangle" };
            const char* opHeadersAlgo0[] = { "Op 1 (Mod)", "Op 2 (Car)", "Op 3", "Op 4" };
            const char* opHeadersDefault[] = { "Op 1", "Op 2", "Op 3", "Op 4" };
            for (int opIdx = 0; opIdx < 4; opIdx++)
            {
                const char* header = (fm->algorithm == 0) ? opHeadersAlgo0[opIdx] : opHeadersDefault[opIdx];
                if (ImGui::CollapsingHeader(header))
                {
                    FmOperator& op = fm->ops[static_cast<size_t>(opIdx)];

                    int waveIdx = WaveToIndex(op.wave);
                    const std::string waveId = "Wave##op" + std::to_string(opIdx);
                    changed |= ImGui::Combo(waveId.c_str(), &waveIdx, waves, IM_ARRAYSIZE(waves));
                    if (updateHoverHelp) updateHoverHelp("Wave を選択します。", "このオペレータの波形が変わります。", nullptr);
                    op.wave = WaveFromIndex(waveIdx);

                    const std::string ratioId = "Ratio##op" + std::to_string(opIdx);
                    changed |= ImGui::InputDouble(ratioId.c_str(), &op.ratio, 0.01, 0.1, "%.3f");
                    if (updateHoverHelp) updateHoverHelp("Ratio を調整します。", "このオペレータの周波数比が変わります。", nullptr);

                    const std::string levelId = "Level##op" + std::to_string(opIdx);
                    changed |= ImGui::InputDouble(levelId.c_str(), &op.level, 0.01, 0.1, "%.3f");
                    if (updateHoverHelp) updateHoverHelp("Level を調整します。", "このオペレータの出力レベルが変わります。", nullptr);

                    const std::string indexId = "Index##op" + std::to_string(opIdx);
                    changed |= ImGui::InputDouble(indexId.c_str(), &op.index, 0.01, 0.1, "%.3f");
                    if (updateHoverHelp) updateHoverHelp("Index を調整します。", "このオペレータの変調深さが変わります。", nullptr);
                }
            }

            const char* filterModes[] = { "bypass", "lowpass", "highpass", "bandpass" };
            int filterModeIdx = 0;
            switch (fm->filterMode)
            {
            case FilterMode::Bypass: filterModeIdx = 0; break;
            case FilterMode::LowPass: filterModeIdx = 1; break;
            case FilterMode::HighPass: filterModeIdx = 2; break;
            case FilterMode::BandPass: filterModeIdx = 3; break;
            }
            ImGui::SetNextItemWidth(220.0f);
            if (ImGui::Combo("Filter Mode", &filterModeIdx, filterModes, IM_ARRAYSIZE(filterModes)))
            {
                switch (filterModeIdx)
                {
                case 0: fm->filterMode = FilterMode::Bypass; break;
                case 1: fm->filterMode = FilterMode::LowPass; break;
                case 2: fm->filterMode = FilterMode::HighPass; break;
                case 3: fm->filterMode = FilterMode::BandPass; break;
                default: fm->filterMode = FilterMode::Bypass; break;
                }
                changed = true;
            }
            if (updateHoverHelp) updateHoverHelp("Filter Mode を選択します。", "フィルタ有効/種別が変わります。", nullptr);
            if (fm->filterMode != FilterMode::Bypass)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "(active)");
            }
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Filter Cutoff (Hz)", fm->filterCutoffHz, 10.0f, 20000.0f, "%.1f");
            if (updateHoverHelp) updateHoverHelp("Filter Cutoff を調整します。", "通過帯域の中心が変わります。", nullptr);
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Filter Resonance (Q)", fm->filterResonance, 0.1f, 18.0f, "%.2f");
            if (updateHoverHelp) updateHoverHelp("Filter Resonance を調整します。", "カットオフ付近の強調量が変わります。", nullptr);

            changed |= drawModulationEditor("fm_modulation", fm->modulation, false, true);
        }
        else if (auto* psg = std::get_if<PsgConfig>(&chCfg.source))
        {
            psg->duty = std::clamp(psg->duty, 0, 7);
            psg->volumeSteps = std::clamp(psg->volumeSteps, 0, 15);
            psg->maxVoices = std::clamp(psg->maxVoices, 1, 8);

            int psgWaveIdx = 0;
            switch (psg->wave)
            {
            case PsgWaveType::Square: psgWaveIdx = 0; break;
            case PsgWaveType::Pulse: psgWaveIdx = 1; break;
            case PsgWaveType::Triangle: psgWaveIdx = 2; break;
            case PsgWaveType::Noise: psgWaveIdx = 3; break;
            }
            const char* psgWaves[] = { "Square", "Pulse", "Triangle", "Noise" };
            if (ImGui::Combo("Wave", &psgWaveIdx, psgWaves, IM_ARRAYSIZE(psgWaves)))
            {
                switch (psgWaveIdx)
                {
                case 0: psg->wave = PsgWaveType::Square; break;
                case 1: psg->wave = PsgWaveType::Pulse; break;
                case 2: psg->wave = PsgWaveType::Triangle; break;
                case 3: psg->wave = PsgWaveType::Noise; break;
                default: psg->wave = PsgWaveType::Square; break;
                }
                changed = true;
            }
            if (updateHoverHelp) updateHoverHelp("PSG Wave を選択します。", "PSG波形（Square/Pulse/Triangle/Noise）が切り替わります。", nullptr);

            if (psg->wave == PsgWaveType::Pulse)
            {
                int duty = psg->duty;
                ImGui::SetNextItemWidth(220.0f);
                if (ImGui::SliderInt("Duty", &duty, 0, 7))
                {
                    psg->duty = duty;
                    changed = true;
                }
                if (updateHoverHelp) updateHoverHelp("Duty を調整します。", "Pulse波のパルス幅が変わります。", nullptr);
            }

            int volumeSteps = psg->volumeSteps;
            ImGui::SetNextItemWidth(220.0f);
            if (ImGui::SliderInt("Volume Steps", &volumeSteps, 0, 15))
            {
                psg->volumeSteps = volumeSteps;
                changed = true;
            }
            if (updateHoverHelp) updateHoverHelp("Volume Steps を調整します。", "PSGの離散音量ステップ数が変わります。", nullptr);

            int maxVoices = psg->maxVoices;
            ImGui::SetNextItemWidth(220.0f);
            if (ImGui::SliderInt("Max Voices", &maxVoices, 1, 8))
            {
                psg->maxVoices = maxVoices;
                changed = true;
            }
            if (updateHoverHelp) updateHoverHelp("Max Voices を調整します。", "PSGの同時発音上限が変わります。", nullptr);
        }
        else if (auto* kit = std::get_if<DrumKitConfig>(&chCfg.source))
        {
            changed |= ImGui::InputInt("DrumKit Note (0-127)", &state.selectedDrumNote);
            if (updateHoverHelp)
            {
                updateHoverHelp(
                    "DrumKit Note を選択します。",
                    "編集対象ノートのドラム定義が切り替わります。",
                    nullptr);
            }
            state.selectedDrumNote = std::clamp(state.selectedDrumNote, 0, 127);
            DrumConfig& d = kit->map[state.selectedDrumNote];
            changed |= DrawDrumConfigEditor("drum_kit", d, updateHoverHelp);
        }
    }
    return changed;
}
} // namespace gui
