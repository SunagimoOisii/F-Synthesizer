        auto drawWaveformLikeCoreEditor = [&](auto& src, const char* hardSyncTag, const char* arpeggioTag) -> bool
        {
            bool localChanged = false;
            int idx = WaveToIndex(src.wave);
            const char* waves[] = { "sine", "square", "saw", "triangle" };
            localChanged |= ImGui::Combo("Wave", &idx, waves, IM_ARRAYSIZE(waves));
            if (updateHoverHelp) updateHoverHelp("Wave を選択します。", "基本波形キャラクターが変わります。", nullptr);
            src.wave = WaveFromIndex(idx);
            if (src.wave == WaveType::Square)
            {
                ImGui::SetNextItemWidth(220.0f);
                localChanged |= sliderWaveParam("Pulse Width", src.pulseWidth, 0.05f, 0.95f, "%.2f");
                if (updateHoverHelp) updateHoverHelp("Pulse Width を調整します。", "Square波のデューティ幅が変わります。", nullptr);
            }

            src.unisonVoices = std::clamp(src.unisonVoices, 1, 8);
            src.unisonDetuneCents = std::clamp(src.unisonDetuneCents, 0.0, 120.0);
            src.unisonSpread = std::clamp(src.unisonSpread, 0.0, 1.0);
            src.subOscLevel = std::clamp(src.subOscLevel, 0.0, 2.0);
            src.pulseWidth = std::clamp(src.pulseWidth, 0.05, 0.95);
            src.hardSyncRatio = std::clamp(src.hardSyncRatio, 0.5, 8.0);
            src.ringModRatio = std::clamp(src.ringModRatio, 0.125, 16.0);
            src.ringModMix = std::clamp(src.ringModMix, 0.0, 1.0);
            src.arpeggio.rateHz = std::clamp(src.arpeggio.rateHz, 0.5, 40.0);
            src.arpeggio.steps = std::clamp(src.arpeggio.steps, 1, 8);
            for (int& semitone : src.arpeggio.semitones)
            {
                semitone = std::clamp(semitone, -24, 24);
            }
            src.filterCutoffHz = std::clamp(src.filterCutoffHz, 10.0, 20000.0);
            src.filterResonance = std::clamp(src.filterResonance, 0.1, 18.0);
            src.filterKeytrack = std::clamp(src.filterKeytrack, 0.0, 1.0);

            int unisonVoices = src.unisonVoices;
            ImGui::SetNextItemWidth(220.0f);
            if (ImGui::SliderInt("Unison Voices", &unisonVoices, 1, 8))
            {
                src.unisonVoices = unisonVoices;
                localChanged = true;
            }
            if (updateHoverHelp) updateHoverHelp("Unison Voices を調整します。", "重ねる発音数が変わり厚みが変わります。", "増やすほどCPU負荷が上がります。");
            ImGui::SetNextItemWidth(220.0f);
            localChanged |= sliderWaveParam("Unison Detune (cent)", src.unisonDetuneCents, 0.0f, 120.0f, "%.1f");
            if (updateHoverHelp) updateHoverHelp("Unison Detune を調整します。", "重ね音のピッチ差が変わります。", nullptr);
            ImGui::SetNextItemWidth(220.0f);
            localChanged |= sliderWaveParam("Unison Spread", src.unisonSpread, 0.0f, 1.0f, "%.2f");
            if (updateHoverHelp) updateHoverHelp("Unison Spread を調整します。", "ステレオの広がりが変わります。", nullptr);
            ImGui::SetNextItemWidth(220.0f);
            localChanged |= sliderWaveParam("Sub Osc Level", src.subOscLevel, 0.0f, 2.0f, "%.2f");
            if (updateHoverHelp) updateHoverHelp("Sub Osc Level を調整します。", "低域補助成分の音量が変わります。", nullptr);

            std::string hardSyncLabel = std::string("Hard Sync##") + hardSyncTag;
            localChanged |= ImGui::Checkbox(hardSyncLabel.c_str(), &src.hardSyncEnabled);
            if (src.hardSyncEnabled)
            {
                std::string syncRatioLabel = std::string("Sync Ratio##") + hardSyncTag;
                ImGui::SetNextItemWidth(220.0f);
                localChanged |= sliderWaveParam(syncRatioLabel.c_str(), src.hardSyncRatio, 0.5f, 8.0f, "%.3f");
            }
            localChanged |= ImGui::Checkbox("Ring Mod", &src.ringModEnabled);
            if (src.ringModEnabled)
            {
                ImGui::SetNextItemWidth(220.0f);
                localChanged |= sliderWaveParam("Ring Ratio", src.ringModRatio, 0.125f, 16.0f, "%.3f");
                ImGui::SetNextItemWidth(220.0f);
                localChanged |= sliderWaveParam("Ring Mix", src.ringModMix, 0.0f, 1.0f, "%.3f");
            }

            ImGui::Separator();
            ImGui::TextUnformatted("Arpeggio");
            std::string arpEnabledLabel = std::string("Enabled##") + arpeggioTag;
            std::string arpRateLabel = std::string("Rate Hz##") + arpeggioTag;
            std::string arpStepsLabel = std::string("Steps##") + arpeggioTag;
            localChanged |= ImGui::Checkbox(arpEnabledLabel.c_str(), &src.arpeggio.enabled);
            ImGui::SetNextItemWidth(220.0f);
            localChanged |= sliderWaveParam(arpRateLabel.c_str(), src.arpeggio.rateHz, 0.5f, 40.0f, "%.2f");
            int arpSteps = src.arpeggio.steps;
            ImGui::SetNextItemWidth(220.0f);
            if (ImGui::SliderInt(arpStepsLabel.c_str(), &arpSteps, 1, 8))
            {
                src.arpeggio.steps = arpSteps;
                localChanged = true;
            }
            for (int k = 0; k < src.arpeggio.steps; k++)
            {
                char label[64];
                snprintf(label, sizeof(label), "Note %d##%s", k + 1, arpeggioTag);
                ImGui::SetNextItemWidth(220.0f);
                localChanged |= ImGui::SliderInt(label, &src.arpeggio.semitones[static_cast<size_t>(k)], -24, 24);
            }

            const char* filterModes[] = { "bypass", "lowpass", "highpass", "bandpass" };
            int filterModeIdx = 0;
            switch (src.filterMode)
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
                case 0: src.filterMode = FilterMode::Bypass; break;
                case 1: src.filterMode = FilterMode::LowPass; break;
                case 2: src.filterMode = FilterMode::HighPass; break;
                case 3: src.filterMode = FilterMode::BandPass; break;
                default: src.filterMode = FilterMode::Bypass; break;
                }
                localChanged = true;
            }
            if (updateHoverHelp) updateHoverHelp("Filter Mode を選択します。", "フィルタ有効/種別が変わります。", nullptr);
            if (src.filterMode != FilterMode::Bypass)
            {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "(active)");
            }
            ImGui::SetNextItemWidth(220.0f);
            localChanged |= sliderWaveParam("Filter Cutoff (Hz)", src.filterCutoffHz, 10.0f, 20000.0f, "%.1f");
            if (updateHoverHelp) updateHoverHelp("Filter Cutoff を調整します。", "通過帯域の中心が変わります。", nullptr);
            ImGui::SetNextItemWidth(220.0f);
            localChanged |= sliderWaveParam("Filter Resonance (Q)", src.filterResonance, 0.1f, 18.0f, "%.2f");
            if (updateHoverHelp)
            {
                updateHoverHelp(
                    "Filter Resonance を調整します。",
                    "カットオフ付近の強調量が変わります。Layer2「荒さ」スライダーの書き込み範囲は 0.5〜6.0 です。",
                    nullptr);
            }
            ImGui::SetNextItemWidth(220.0f);
            localChanged |= sliderWaveParam("Filter Keytrack", src.filterKeytrack, 0.0f, 1.0f, "%.2f");
            if (updateHoverHelp) updateHoverHelp("Filter Keytrack を調整します。", "ノート音程に連動してカットオフが動く量が変わります。基準は C4(60)。", nullptr);
            return localChanged;
        };

        auto drawWaveformLikeSmoothingAndModulation = [&](auto& src, const char* modulationId) -> bool
        {
            bool localChanged = false;
            src.smoothing.ampTimeMs = std::clamp(src.smoothing.ampTimeMs, 0.0, 1000.0);
            src.smoothing.pitchTimeMs = std::clamp(src.smoothing.pitchTimeMs, 0.0, 1000.0);
            src.smoothing.filterCutoffTimeMs = std::clamp(src.smoothing.filterCutoffTimeMs, 0.0, 1000.0);
            src.modulation.lfo1.rateHz = std::clamp(src.modulation.lfo1.rateHz, 0.0, 100.0);
            src.modulation.lfo1.depth = std::clamp(src.modulation.lfo1.depth, 0.0, 1.0);
            src.modulation.lfo1.delayMs = std::clamp(src.modulation.lfo1.delayMs, 0.0, 2000.0);
            src.modulation.lfo1.fadeMs = std::clamp(src.modulation.lfo1.fadeMs, 0.0, 2000.0);
            src.modulation.env2.attackSec = std::clamp(src.modulation.env2.attackSec, 0.0, 10.0);
            src.modulation.env2.decaySec = std::clamp(src.modulation.env2.decaySec, 0.0, 10.0);
            src.modulation.env2.sustainLevel = std::clamp(src.modulation.env2.sustainLevel, 0.0, 1.0);
            src.modulation.env2.releaseSec = std::clamp(src.modulation.env2.releaseSec, 0.0, 10.0);
            src.modulation.env2.curve = std::clamp(src.modulation.env2.curve, 0.0, 1.0);
            for (auto& route : src.modulation.matrix.routes)
            {
                route.amount = std::clamp(route.amount, -1.0, 1.0);
            }

            ImGui::Separator();
            ImGui::TextUnformatted("Smoothing");
            localChanged |= ImGui::Checkbox("Smoothing Enabled", &src.smoothing.enabled);
            if (updateHoverHelp) updateHoverHelp("Smoothing Enabled を切り替えます。", "パラメータ変化の段差を抑えます。", nullptr);
            localChanged |= ImGui::Checkbox("Pitch Smoothing Enabled", &src.smoothing.pitchEnabled);
            if (updateHoverHelp) updateHoverHelp("Pitch Smoothing Enabled を切り替えます。", "ピッチ変化の滑らかさが変わります。", nullptr);
            ImGui::SetNextItemWidth(220.0f);
            localChanged |= sliderWaveParam("Amp Smoothing (ms)", src.smoothing.ampTimeMs, 0.0f, 1000.0f, "%.1f");
            ImGui::SetNextItemWidth(220.0f);
            localChanged |= sliderWaveParam("Pitch Smoothing (ms)", src.smoothing.pitchTimeMs, 0.0f, 1000.0f, "%.1f");
            ImGui::SetNextItemWidth(220.0f);
            localChanged |= sliderWaveParam("Filter Smoothing (ms)", src.smoothing.filterCutoffTimeMs, 0.0f, 1000.0f, "%.1f");
            localChanged |= drawModulationEditor(modulationId, src.modulation, true, false, true);
            return localChanged;
        };
