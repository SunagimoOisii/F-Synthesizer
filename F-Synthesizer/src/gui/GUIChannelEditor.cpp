#include "gui/GUIChannelEditor.h"

#include <algorithm>
#include <string>

#include <imgui.h>

#include "config/SourceRegistry.h"
#include "gui/GUIConfigUtils.h"
#include "gui/GUIStateModel.h"

namespace
{
using HoverHelpFn = std::function<void(const char* what, const char* impact, const char* caution)>;

constexpr config::SourceKind kGuiSourceKinds[] = {
    config::SourceKind::Waveform,
    config::SourceKind::Noise,
    config::SourceKind::Fm,
    config::SourceKind::DrumKit,
};

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

        int toneWave = d.toneWave >= 0 ? d.toneWave : 0;
        const char* waves[] = { "sine", "square", "saw", "triangle" };
        key = std::string("Tone Wave##") + IDPrefix;
        changed |= ImGui::Combo(key.c_str(), &toneWave, waves, IM_ARRAYSIZE(waves));
        if (updateHoverHelp) updateHoverHelp("Tone Wave を選択します。", "有音成分の波形キャラクターが変わります。", nullptr);
        d.toneWave = toneWave;

        int noiseType = d.noiseType >= 0 ? d.noiseType : 0;
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
        if (auto* legacyDrum = std::get_if<DrumConfig>(&chCfg.source))
        {
            // Soundタブでは drumkit を正規運用とし、旧 drum 設定は自動移行する。
            DrumKitConfig kit{};
            for (auto& d : kit.map)
            {
                d.type = DrumType::None;
            }
            const int dstNote = std::clamp(state.selectedDrumNote, 0, 127);
            kit.map[dstNote] = *legacyDrum;
            chCfg.source = kit;
            changed = true;
            ImGui::TextDisabled("Legacy drum source was converted to drumkit (note %d).", dstNote);
        }

        const config::SourceKind selectedKind = config::SourceConfigKind(chCfg.source);
        int srcType = 0;
        for (int i = 0; i < IM_ARRAYSIZE(kGuiSourceKinds); i++)
        {
            if (kGuiSourceKinds[i] == selectedKind)
            {
                srcType = i;
                break;
            }
        }
        if (ImGui::BeginCombo("Source Type", config::SourceKindToDisplayName(kGuiSourceKinds[srcType])))
        {
            for (int i = 0; i < IM_ARRAYSIZE(kGuiSourceKinds); i++)
            {
                const config::SourceKind candidate = kGuiSourceKinds[i];
                const bool selected = (srcType == i);
                if (ImGui::Selectable(config::SourceKindToDisplayName(candidate), selected))
                {
                    srcType = i;
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
                "Waveform/Noise/FM/DrumKit の編集対象に切り替わります。",
                "切替時は該当タイプの既定設定で初期化されます。");
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

            ImGui::Separator();
            ImGui::TextUnformatted("Modulation");

            const char* lfoWaves[] = { "sine", "triangle" };
            int lfoWaveIdx = (wf->modulation.lfo1.wave == LfoWave::Triangle) ? 1 : 0;
            ImGui::SetNextItemWidth(220.0f);
            if (ImGui::Combo("LFO1 Wave", &lfoWaveIdx, lfoWaves, IM_ARRAYSIZE(lfoWaves)))
            {
                wf->modulation.lfo1.wave = (lfoWaveIdx == 1) ? LfoWave::Triangle : LfoWave::Sine;
                changed = true;
            }
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("LFO1 Rate (Hz)", wf->modulation.lfo1.rateHz, 0.0f, 100.0f, "%.2f");
            if (updateHoverHelp) updateHoverHelp("LFO1 Rate を調整します。", "周期変調の速さが変わります。", nullptr);
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("LFO1 Depth", wf->modulation.lfo1.depth, 0.0f, 1.0f, "%.3f");
            if (updateHoverHelp) updateHoverHelp("LFO1 Depth を調整します。", "LFOの変調量が変わります。", nullptr);
            changed |= ImGui::Checkbox("LFO1 Bipolar", &wf->modulation.lfo1.bipolar);
            if (updateHoverHelp) updateHoverHelp("LFO1 Bipolar を切り替えます。", "LFO出力の極性レンジが変わります。", nullptr);

            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Env2 Attack", wf->modulation.env2.attackSec, 0.0f, 10.0f, "%.3f");
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Env2 Decay", wf->modulation.env2.decaySec, 0.0f, 10.0f, "%.3f");
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Env2 Sustain", wf->modulation.env2.sustainLevel, 0.0f, 1.0f, "%.3f");
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Env2 Release", wf->modulation.env2.releaseSec, 0.0f, 10.0f, "%.3f");

            const char* modSources[] = { "none", "lfo1", "env2" };
            const char* modDestinations[] = { "none", "pitch", "amp", "filterCutoff" };
            for (int routeIdx = 0; routeIdx < 4; routeIdx++)
            {
                ModRoute& route = wf->modulation.matrix.routes[static_cast<size_t>(routeIdx)];
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
                        + modDestinations[static_cast<int>(route.destination)]
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

                changed |= ImGui::Checkbox("Enabled", &route.enabled);
                if (updateHoverHelp) updateHoverHelp("Route Enabled を切り替えます。", "このモジュレーション経路の有効/無効が変わります。", nullptr);

                int srcIdx = 0;
                switch (route.source)
                {
                case ModSource::None: srcIdx = 0; break;
                case ModSource::Lfo1: srcIdx = 1; break;
                case ModSource::Env2: srcIdx = 2; break;
                }
                ImGui::SetNextItemWidth(220.0f);
                if (ImGui::Combo("Source", &srcIdx, modSources, IM_ARRAYSIZE(modSources)))
                {
                    switch (srcIdx)
                    {
                    case 0: route.source = ModSource::None; break;
                    case 1: route.source = ModSource::Lfo1; break;
                    case 2: route.source = ModSource::Env2; break;
                    default: route.source = ModSource::None; break;
                    }
                    changed = true;
                }
                if (updateHoverHelp) updateHoverHelp("Route Source を選択します。", "変調元が変わります。", nullptr);

                int dstIdx = 0;
                switch (route.destination)
                {
                case ModDestination::None: dstIdx = 0; break;
                case ModDestination::Pitch: dstIdx = 1; break;
                case ModDestination::Amp: dstIdx = 2; break;
                case ModDestination::FilterCutoff: dstIdx = 3; break;
                }
                ImGui::SetNextItemWidth(220.0f);
                if (ImGui::Combo("Destination", &dstIdx, modDestinations, IM_ARRAYSIZE(modDestinations)))
                {
                    switch (dstIdx)
                    {
                    case 0: route.destination = ModDestination::None; break;
                    case 1: route.destination = ModDestination::Pitch; break;
                    case 2: route.destination = ModDestination::Amp; break;
                    case 3: route.destination = ModDestination::FilterCutoff; break;
                    default: route.destination = ModDestination::None; break;
                    }
                    changed = true;
                }
                if (updateHoverHelp) updateHoverHelp("Route Destination を選択します。", "変調先パラメータが変わります。", nullptr);
                ImGui::SetNextItemWidth(220.0f);
                changed |= sliderWaveParam("Amount", route.amount, -1.0f, 1.0f, "%.3f");
                if (updateHoverHelp) updateHoverHelp("Route Amount を調整します。", "変調量と極性が変わります。", nullptr);
                ImGui::Separator();
                ImGui::PopID();
            }
        }
        else if (auto* nz = std::get_if<NoiseConfig>(&chCfg.source))
        {
            int idx = NoiseToIndex(nz->noise);
            const char* noises[] = { "white", "pink", "brown", "blue" };
            changed |= ImGui::Combo("Noise", &idx, noises, IM_ARRAYSIZE(noises));
            if (updateHoverHelp) updateHoverHelp("Noise を選択します。", "ノイズ種別（色）が変わります。", nullptr);
            nz->noise = NoiseFromIndex(idx);
        }
        else if (auto* fm = std::get_if<FmConfig>(&chCfg.source))
        {
            int cIdx = WaveToIndex(fm->carrierWave);
            int mIdx = WaveToIndex(fm->modWave);
            const char* waves[] = { "sine", "square", "saw", "triangle" };
            changed |= ImGui::Combo("Carrier Wave", &cIdx, waves, IM_ARRAYSIZE(waves));
            if (updateHoverHelp) updateHoverHelp("Carrier Wave を選択します。", "FMの主音波形が変わります。", nullptr);
            changed |= ImGui::Combo("Mod Wave", &mIdx, waves, IM_ARRAYSIZE(waves));
            if (updateHoverHelp) updateHoverHelp("Mod Wave を選択します。", "FMの変調波形が変わります。", nullptr);
            fm->carrierWave = WaveFromIndex(cIdx);
            fm->modWave = WaveFromIndex(mIdx);
            changed |= ImGui::InputDouble("Carrier Ratio", &fm->carrierRatio, 0.01, 0.1, "%.3f");
            if (updateHoverHelp) updateHoverHelp("Carrier Ratio を調整します。", "主発振周波数比が変わります。", nullptr);
            changed |= ImGui::InputDouble("Mod Ratio", &fm->modRatio, 0.01, 0.1, "%.3f");
            if (updateHoverHelp) updateHoverHelp("Mod Ratio を調整します。", "変調発振周波数比が変わります。", nullptr);
            changed |= ImGui::InputDouble("FM Index", &fm->index, 0.01, 0.1, "%.3f");
            if (updateHoverHelp) updateHoverHelp("FM Index を調整します。", "倍音の強さが変わります。", nullptr);
            changed |= ImGui::InputDouble("FM OutLevel", &fm->outLevel, 0.01, 0.1, "%.3f");
            if (updateHoverHelp) updateHoverHelp("FM OutLevel を調整します。", "FM経路の出力音量が変わります。", nullptr);
        }
        else if (auto* drum = std::get_if<DrumConfig>(&chCfg.source))
        {
            changed |= DrawDrumConfigEditor("drum_single", *drum, updateHoverHelp);
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
