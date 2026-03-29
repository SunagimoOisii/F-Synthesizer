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
        ImGui::TextDisabled("LFO1 Wave");
        DrawLfo1WavePreview(
            (std::string("##lfo1_preview_") + idPrefix).c_str(),
            modulation.lfo1.wave);

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
        ImGui::TextDisabled("Env2 Envelope");
        DrawADSRPreview(
            (std::string("##env2_preview_") + idPrefix).c_str(),
            static_cast<float>(modulation.env2.attackSec),
            static_cast<float>(modulation.env2.decaySec),
            static_cast<float>(modulation.env2.sustainLevel),
            static_cast<float>(modulation.env2.releaseSec),
            static_cast<float>(modulation.env2.curve));

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

        ImGui::Separator();
        ImGui::TextDisabled("Modulation Routing View");
        const ImVec2 canvasSize = ImVec2(0.0f, 190.0f);
        ImGui::BeginChild((std::string("##mod_route_view_") + idPrefix).c_str(), canvasSize, true);
        {
            struct SourceNode
            {
                ModSource source;
                const char* label;
            };
            constexpr std::array<SourceNode, 6> sourceNodes{ {
                { ModSource::Lfo1, "LFO1" },
                { ModSource::Env2, "Env2" },
                { ModSource::Velocity, "Velocity" },
                { ModSource::ChannelPressure, "ChPressure" },
                { ModSource::PolyPressure, "PolyPressure" },
                { ModSource::ModWheel, "ModWheel" },
            } };

            std::array<ModDestination, 7> destinationNodes{};
            int destinationNodeCount = 0;
            for (int i = 0; i < destinationCount; i++)
            {
                const ModDestination destination = destinationChoices[i].value;
                if (destination == ModDestination::None)
                {
                    continue;
                }
                destinationNodes[static_cast<size_t>(destinationNodeCount++)] = destination;
            }
            if (destinationNodeCount <= 0)
            {
                destinationNodes[0] = ModDestination::Amp;
                destinationNodeCount = 1;
            }

            auto sourceIndexOf = [&](ModSource source) -> int
            {
                for (int i = 0; i < static_cast<int>(sourceNodes.size()); i++)
                {
                    if (sourceNodes[static_cast<size_t>(i)].source == source)
                    {
                        return i;
                    }
                }
                return -1;
            };
            auto destinationIndexOf = [&](ModDestination destination) -> int
            {
                for (int i = 0; i < destinationNodeCount; i++)
                {
                    if (destinationNodes[static_cast<size_t>(i)] == destination)
                    {
                        return i;
                    }
                }
                return -1;
            };

            ImDrawList* drawList = ImGui::GetWindowDrawList();
            const ImVec2 topLeft = ImGui::GetCursorScreenPos();
            const float width = (std::max)(220.0f, ImGui::GetContentRegionAvail().x - 8.0f);
            const float leftX = topLeft.x + 8.0f;
            const float rightX = topLeft.x + width - 144.0f;
            const float nodeW = 132.0f;
            const float nodeH = 22.0f;
            const float yTop = topLeft.y + 8.0f;
            const float leftSpan = (sourceNodes.size() > 1) ? 148.0f / static_cast<float>(sourceNodes.size() - 1) : 0.0f;
            const float rightSpan = (destinationNodeCount > 1) ? 148.0f / static_cast<float>(destinationNodeCount - 1) : 0.0f;

            std::array<ImVec2, sourceNodes.size()> srcAnchors{};
            for (int i = 0; i < static_cast<int>(sourceNodes.size()); i++)
            {
                const float y = yTop + leftSpan * static_cast<float>(i);
                const ImVec2 p0 = ImVec2(leftX, y);
                const ImVec2 p1 = ImVec2(leftX + nodeW, y + nodeH);
                drawList->AddRectFilled(p0, p1, IM_COL32(56, 72, 96, 190), 4.0f);
                drawList->AddRect(p0, p1, IM_COL32(110, 130, 170, 220), 4.0f);
                drawList->AddText(ImVec2(p0.x + 8.0f, p0.y + 4.0f), IM_COL32(230, 236, 245, 255), sourceNodes[static_cast<size_t>(i)].label);
                srcAnchors[static_cast<size_t>(i)] = ImVec2(p1.x, p0.y + nodeH * 0.5f);
            }

            std::array<ImVec2, 7> dstAnchors{};
            for (int i = 0; i < destinationNodeCount; i++)
            {
                const float y = yTop + rightSpan * static_cast<float>(i);
                const ImVec2 p0 = ImVec2(rightX, y);
                const ImVec2 p1 = ImVec2(rightX + nodeW, y + nodeH);
                drawList->AddRectFilled(p0, p1, IM_COL32(72, 88, 62, 190), 4.0f);
                drawList->AddRect(p0, p1, IM_COL32(120, 170, 120, 220), 4.0f);
                drawList->AddText(ImVec2(p0.x + 8.0f, p0.y + 4.0f), IM_COL32(236, 245, 230, 255), destinationLabel(destinationNodes[static_cast<size_t>(i)]));
                dstAnchors[static_cast<size_t>(i)] = ImVec2(p0.x, p0.y + nodeH * 0.5f);
            }

            bool hasAnyRoute = false;
            for (const ModRoute& route : modulation.matrix.routes)
            {
                if (route.source == ModSource::None || route.destination == ModDestination::None)
                {
                    continue;
                }
                const int srcIdx = sourceIndexOf(route.source);
                const int dstIdx = destinationIndexOf(route.destination);
                if (srcIdx < 0 || dstIdx < 0)
                {
                    continue;
                }
                const ImVec2 p0 = srcAnchors[static_cast<size_t>(srcIdx)];
                const ImVec2 p3 = dstAnchors[static_cast<size_t>(dstIdx)];
                const float cdx = (p3.x - p0.x) * 0.4f;
                const ImVec2 p1 = ImVec2(p0.x + cdx, p0.y);
                const ImVec2 p2 = ImVec2(p3.x - cdx, p3.y);
                const ImU32 lineColor = route.enabled
                    ? (route.amount >= 0.0 ? IM_COL32(110, 220, 140, 235) : IM_COL32(220, 140, 110, 235))
                    : IM_COL32(120, 120, 120, 150);
                const float thickness = route.enabled ? 2.4f : 1.2f;
                drawList->AddBezierCubic(p0, p1, p2, p3, lineColor, thickness);
                hasAnyRoute = true;
            }

            if (!hasAnyRoute)
            {
                drawList->AddText(
                    ImVec2(topLeft.x + 12.0f, topLeft.y + 166.0f),
                    IM_COL32(180, 180, 180, 220),
                    "No active routes");
            }

            ImGui::Dummy(ImVec2(width, 170.0f));
        }
        ImGui::EndChild();
        ImGui::PopID();
        return localChanged;
    };
