    auto drawModulationEditor = [&](const char* idPrefix,
        ModulationConfig& modulation,
        bool allowFilterCutoff,
        bool allowFmIndex,
        bool allowPulseWidth) -> bool
    {
        bool localChanged = false;
        ImGui::Separator();
        ImGui::TextUnformatted("Modulation");

        const char* lfoWaves[] = { "Sine", "Triangle", "Square", "Saw", "S&H" };
        int lfoWaveIdx = 0;
        switch (modulation.lfo1.wave)
        {
        case LfoWave::Sine: lfoWaveIdx = 0; break;
        case LfoWave::Triangle: lfoWaveIdx = 1; break;
        case LfoWave::Square: lfoWaveIdx = 2; break;
        case LfoWave::Saw: lfoWaveIdx = 3; break;
        case LfoWave::SampleAndHold: lfoWaveIdx = 4; break;
        }
        ImGui::SetNextItemWidth(220.0f);
        if (ImGui::Combo("LFO1 Wave", &lfoWaveIdx, lfoWaves, IM_ARRAYSIZE(lfoWaves)))
        {
            switch (lfoWaveIdx)
            {
            case 0: modulation.lfo1.wave = LfoWave::Sine; break;
            case 1: modulation.lfo1.wave = LfoWave::Triangle; break;
            case 2: modulation.lfo1.wave = LfoWave::Square; break;
            case 3: modulation.lfo1.wave = LfoWave::Saw; break;
            case 4: modulation.lfo1.wave = LfoWave::SampleAndHold; break;
            default: modulation.lfo1.wave = LfoWave::Sine; break;
            }
            localChanged = true;
        }
        if (updateHoverHelp) updateHoverHelp("LFO1 の波形を選択します。", "変調の形が変わります。Sine=なめらか、Square=段階的、Saw=のこぎり、S&H=ランダムホールド。", nullptr);
        ImGui::SetNextItemWidth(220.0f);
        localChanged |= sliderWaveParam("LFO1 Rate (Hz)", modulation.lfo1.rateHz, 0.0f, 100.0f, "%.2f");
        if (updateHoverHelp) updateHoverHelp("LFO1 Rate を調整します。", "周期変調の速さが変わります。", nullptr);
        ImGui::SetNextItemWidth(220.0f);
        localChanged |= sliderWaveParam("LFO1 Depth", modulation.lfo1.depth, 0.0f, 1.0f, "%.3f");
        if (updateHoverHelp) updateHoverHelp("LFO1 Depth を調整します。", "LFOの変調量が変わります。", nullptr);
        localChanged |= ImGui::Checkbox("LFO1 Bipolar", &modulation.lfo1.bipolar);
        if (updateHoverHelp) updateHoverHelp("LFO1 Bipolar を切り替えます。", "LFO出力の極性レンジが変わります。", nullptr);
        localChanged |= ImGui::Checkbox("Key Sync", &modulation.lfo1.keySync);
        if (updateHoverHelp)
        {
            updateHoverHelp(
                "ノートオン時に LFO 位相を 0 にリセットします。",
                "オフの場合は free-run（位相を引き継ぐ）です。",
                "");
        }
        ImGui::SetNextItemWidth(220.0f);
        localChanged |= sliderWaveParam("LFO1 Delay (ms)", modulation.lfo1.delayMs, 0.0f, 2000.0f, "%.1f");
        if (updateHoverHelp) updateHoverHelp("LFO1 Delay を調整します。", "ノートオン後にLFOが有効になるまでの待機時間が変わります。", nullptr);
        ImGui::SetNextItemWidth(220.0f);
        localChanged |= sliderWaveParam("LFO1 Fade (ms)", modulation.lfo1.fadeMs, 0.0f, 2000.0f, "%.1f");
        if (updateHoverHelp) updateHoverHelp("LFO1 Fade を調整します。", "LFOが最大深さに達するまでの立ち上がり時間が変わります。", nullptr);

        ImGui::SetNextItemWidth(220.0f);
        localChanged |= sliderWaveParam("Env2 Attack", modulation.env2.attackSec, 0.0f, 10.0f, "%.3f");
        if (updateHoverHelp) updateHoverHelp("Env2 Attack を調整します。", "変調が最大値に達するまでの立ち上がり時間が変わります。", nullptr);
        ImGui::SetNextItemWidth(220.0f);
        localChanged |= sliderWaveParam("Env2 Decay", modulation.env2.decaySec, 0.0f, 10.0f, "%.3f");
        if (updateHoverHelp) updateHoverHelp("Env2 Decay を調整します。", "ピーク後にサステインレベルへ落ちるまでの減衰時間が変わります。", nullptr);
        ImGui::SetNextItemWidth(220.0f);
        localChanged |= sliderWaveParam("Env2 Sustain", modulation.env2.sustainLevel, 0.0f, 1.0f, "%.3f");
        if (updateHoverHelp) updateHoverHelp("Env2 Sustain を調整します。", "ノート押下中に維持する変調量が変わります (0〜1)。", nullptr);
        ImGui::SetNextItemWidth(220.0f);
        localChanged |= sliderWaveParam("Env2 Release", modulation.env2.releaseSec, 0.0f, 10.0f, "%.3f");
        if (updateHoverHelp) updateHoverHelp("Env2 Release を調整します。", "ノートオフ後に変調量がゼロに戻るまでのリリース時間が変わります。", nullptr);
        ImGui::SetNextItemWidth(220.0f);
        localChanged |= sliderWaveParam("Env2 Curve", modulation.env2.curve, 0.0f, 1.0f, "%.3f");
        if (updateHoverHelp) updateHoverHelp("Env2 Curve を調整します。", "変化の加速感が変わります。低いと急激に、高いとなだらかに変化します。", nullptr);

        const char* modSources[] = { "none", "lfo1", "env2", "velocity", "channelPressure", "polyPressure", "ModWheel" };
        struct DestinationChoice
        {
            const char* label;
            ModDestination value;
        };
        std::array<DestinationChoice, 7> destinationChoices{ {
            { "none", ModDestination::None },
            { "pitchMul", ModDestination::Pitch },
            { "amp", ModDestination::Amp },
            { "filterCutoffHz", ModDestination::FilterCutoff },
            { "filterResonance", ModDestination::FilterResonance },
            { "PulseWidth", ModDestination::PulseWidth },
            { "fm.index", ModDestination::FmIndex },
        } };
        int destinationCount = 3;
        if (allowFilterCutoff)
        {
            destinationCount += 2;
        }
        if (allowPulseWidth)
        {
            destinationChoices[destinationCount++] = { "PulseWidth", ModDestination::PulseWidth };
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
            case ModSource::ModWheel: srcIdx = 6; break;
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
                case 6: route.source = ModSource::ModWheel; break;
                default: route.source = ModSource::None; break;
                }
                localChanged = true;
            }
            if (updateHoverHelp) updateHoverHelp("Route Source を選択します。", "変調元が変わります。", nullptr);

            int dstIdx = destinationIndex(route.destination);
            const char* destinationLabels[7] = {};
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
