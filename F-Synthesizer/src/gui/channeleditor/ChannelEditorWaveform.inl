        if (auto* wf = std::get_if<WaveformConfig>(&chCfg.source))
        {
            changed |= drawWaveformLikeCoreEditor(*wf, "wave", "wave_arp");
            changed |= drawWaveformLikeSmoothingAndModulation(*wf, "waveform_modulation");
        }
        else if (auto* analog = std::get_if<AnalogConfig>(&chCfg.source))
        {
            changed |= drawWaveformLikeCoreEditor(*analog, "analog", "analog_arp");

            analog->drive = std::clamp(analog->drive, 0.0, 1.0);
            analog->driftDepthCents = std::clamp(analog->driftDepthCents, 0.0, 20.0);
            analog->driftRateHz = std::clamp(analog->driftRateHz, 0.01, 2.0);

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

            changed |= drawWaveformLikeSmoothingAndModulation(*analog, "analog_modulation");
        }
