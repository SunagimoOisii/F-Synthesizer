#include "gui/GUIChannelEditor.h"

#include <algorithm>
#include <string>

#include <imgui.h>

#include "config/SourceRegistry.h"
#include "gui/GUIConfigUtils.h"
#include "gui/GUIStateModel.h"

namespace
{
constexpr config::SourceKind kGuiSourceKinds[] = {
    config::SourceKind::Waveform,
    config::SourceKind::Noise,
    config::SourceKind::Fm,
    config::SourceKind::DrumKit,
};

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
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Unison Detune (cent)", wf->unisonDetuneCents, 0.0f, 120.0f, "%.1f");
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Unison Spread", wf->unisonSpread, 0.0f, 1.0f, "%.2f");
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Sub Osc Level", wf->subOscLevel, 0.0f, 2.0f, "%.2f");

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
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Filter Cutoff (Hz)", wf->filterCutoffHz, 10.0f, 20000.0f, "%.1f");
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Filter Resonance (Q)", wf->filterResonance, 0.1f, 18.0f, "%.2f");

            ImGui::Separator();
            ImGui::TextUnformatted("Smoothing");
            changed |= ImGui::Checkbox("Smoothing Enabled", &wf->smoothing.enabled);
            changed |= ImGui::Checkbox("Pitch Smoothing Enabled", &wf->smoothing.pitchEnabled);
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
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("LFO1 Depth", wf->modulation.lfo1.depth, 0.0f, 1.0f, "%.3f");
            changed |= ImGui::Checkbox("LFO1 Bipolar", &wf->modulation.lfo1.bipolar);

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
                std::string label = "Route " + std::to_string(routeIdx);
                ImGui::TextUnformatted(label.c_str());
                changed |= ImGui::Checkbox("Enabled", &route.enabled);

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
                ImGui::SetNextItemWidth(220.0f);
                changed |= sliderWaveParam("Amount", route.amount, -1.0f, 1.0f, "%.3f");
                ImGui::Separator();
                ImGui::PopID();
            }
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
