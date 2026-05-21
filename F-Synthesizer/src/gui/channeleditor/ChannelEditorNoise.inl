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
            fm->modulation.lfo1.delayMs = std::clamp(fm->modulation.lfo1.delayMs, 0.0, 2000.0);
            fm->modulation.lfo1.fadeMs = std::clamp(fm->modulation.lfo1.fadeMs, 0.0, 2000.0);
            fm->modulation.env2.attackSec = std::clamp(fm->modulation.env2.attackSec, 0.0, 10.0);
            fm->modulation.env2.decaySec = std::clamp(fm->modulation.env2.decaySec, 0.0, 10.0);
            fm->modulation.env2.sustainLevel = std::clamp(fm->modulation.env2.sustainLevel, 0.0, 1.0);
            fm->modulation.env2.releaseSec = std::clamp(fm->modulation.env2.releaseSec, 0.0, 10.0);
            fm->modulation.env2.curve = std::clamp(fm->modulation.env2.curve, 0.0, 1.0);
            for (auto& route : fm->modulation.matrix.routes)
            {
                route.amount = std::clamp(route.amount, -1.0, 1.0);
            }

            const char* algoLabels[] = {
                "0: M->C  (classic pair)",
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

            changed |= drawModulationEditor("fm_modulation", fm->modulation, false, true, false);
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
