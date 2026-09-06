        else if (auto* fm = std::get_if<FmConfig>(&chCfg.source))
        {
            const char* chips[] = { "YM2151 / X68000", "YM2612 / Mega Drive" };
            changed |= ImGui::Combo("音源", &fm->chip, chips, IM_ARRAYSIZE(chips));
            const char* algorithms[] = {
                "0: 1 -> 2 -> 3 -> 4",
                "1: (1 + 2) -> 3 -> 4",
                "2: (1 + (2 -> 3)) -> 4",
                "3: ((1 -> 2) + 3) -> 4",
                "4: (1 -> 2) + (3 -> 4)",
                "5: 1 -> (2 + 3 + 4)",
                "6: (1 -> 2) + 3 + 4",
                "7: 1 + 2 + 3 + 4"
            };
            changed |= ImGui::Combo("音の組み合わせ", &fm->algorithm, algorithms, IM_ARRAYSIZE(algorithms));
            changed |= sliderWaveParam("フィードバック", fm->feedback, 0.0f, 1.0f, "%.2f");
            ImGui::TextWrapped("4つの正弦波を組み合わせます。周波数比は0.5または整数です。鳴り方の秒数は目安で、音源内では段階的な値になります。");
            constexpr int carriers[] = { 8, 8, 8, 8, 10, 14, 14, 15 };
            for (int i = 0; i < 4; ++i)
            {
                ImGui::PushID(i);
                const bool carrier = (carriers[std::clamp(fm->algorithm, 0, 7)] & (1 << i)) != 0;
                const std::string label = "波 " + std::to_string(i + 1) + (carrier ? " / 音量" : " / 音色を変える");
                if (ImGui::TreeNode(label.c_str()))
                {
                    auto& op = fm->ops[i];
                    int ratio = op.ratio < 0.75 ? 0 : std::clamp(static_cast<int>(std::lround(op.ratio)), 1, 15);
                    if (ImGui::SliderInt("周波数比", &ratio, 0, 15, ratio == 0 ? "0.5" : "%d"))
                    { op.ratio = ratio == 0 ? 0.5 : ratio; changed = true; }
                    changed |= sliderWaveParam("レベル", op.level, 0.0f, 1.0f, "%.2f");
                    if (!carrier) changed |= sliderWaveParam("変調量", op.index, 0.0f, 4.0f, "%.2f");
                    changed |= sliderWaveParam("立ち上がり (秒)", op.levelEnv.attackSec, 0.001f, 2.0f, "%.3f");
                    changed |= sliderWaveParam("減衰 (秒)", op.levelEnv.decaySec, 0.01f, 4.0f, "%.2f");
                    changed |= sliderWaveParam("伸ばした時の量", op.levelEnv.sustainLevel, 0.0f, 1.0f, "%.2f");
                    changed |= sliderWaveParam("余韻 (秒)", op.levelEnv.releaseSec, 0.01f, 3.0f, "%.2f");
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
            const char* filterModes[] = { "bypass", "lowpass", "highpass", "bandpass", "ladderLowpass" };
            int filterModeIdx = 0;
            switch (fm->filterMode)
            {
            case FilterMode::Bypass: filterModeIdx = 0; break;
            case FilterMode::LowPass: filterModeIdx = 1; break;
            case FilterMode::HighPass: filterModeIdx = 2; break;
            case FilterMode::BandPass: filterModeIdx = 3; break;
            case FilterMode::LadderLowPass: filterModeIdx = 4; break;
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
                case 4: fm->filterMode = FilterMode::LadderLowPass; break;
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
            ImGui::SetNextItemWidth(220.0f);
            changed |= sliderWaveParam("Filter Drive", fm->filterDrive, 0.0f, 1.0f, "%.2f");
            if (updateHoverHelp) updateHoverHelp("Filter Drive を調整します。", "ladderLowpass の入力段で太さと潰れが増えます。", nullptr);

            changed |= drawModulationEditor("fm_modulation", fm->modulation, false, true, false);
        }
