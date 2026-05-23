        else if (auto* kit = std::get_if<DrumKitConfig>(&chCfg.source))
        {
            if (ImGui::CollapsingHeader("Drum Bus", ImGuiTreeNodeFlags_DefaultOpen))
            {
                DrumBusConfig& bus = kit->drumBus;
                changed |= ImGui::Checkbox("Enabled##drumBus", &bus.enabled);
                if (updateHoverHelp)
                {
                    updateHoverHelp(
                        "Drum Bus を有効化します。",
                        "キット全体の前後感、まとまり、刺さりをまとめて調整します。",
                        "単発の音量ではなく合算後の配置を変えます。");
                }
                changed |= ImGui::InputDouble("Level##drumBus", &bus.level, 0.01, 0.05, "%.3f");
                changed |= ImGui::InputDouble("Attack Trim##drumBus", &bus.attackTrim, 0.01, 0.05, "%.3f");
                changed |= ImGui::InputDouble("Sustain Lift##drumBus", &bus.sustainLift, 0.01, 0.05, "%.3f");
                changed |= ImGui::InputDouble("Glue##drumBus", &bus.glue, 0.01, 0.05, "%.3f");
                changed |= ImGui::InputDouble("Presence Cut##drumBus", &bus.presenceCut, 0.01, 0.05, "%.3f");
                changed |= ImGui::InputDouble("Low Tighten##drumBus", &bus.lowTighten, 0.01, 0.05, "%.3f");
                changed |= ImGui::InputDouble("Room Send##drumBus", &bus.roomSend, 0.01, 0.05, "%.3f");
                changed |= ImGui::InputDouble("Drive Trim##drumBus", &bus.driveTrim, 0.01, 0.05, "%.3f");
                changed |= ImGui::InputDouble("Velocity Ceiling##drumBus", &kit->velocityCeiling, 0.01, 0.05, "%.3f");
                changed |= ImGui::InputDouble("Velocity Curve##drumBus", &kit->velocityCurve, 0.01, 0.05, "%.3f");

                bus.level = std::clamp(bus.level, 0.0, 2.0);
                bus.attackTrim = std::clamp(bus.attackTrim, 0.0, 1.0);
                bus.sustainLift = std::clamp(bus.sustainLift, 0.0, 1.0);
                bus.glue = std::clamp(bus.glue, 0.0, 1.0);
                bus.presenceCut = std::clamp(bus.presenceCut, 0.0, 1.0);
                bus.lowTighten = std::clamp(bus.lowTighten, 0.0, 1.0);
                bus.roomSend = std::clamp(bus.roomSend, 0.0, 1.0);
                bus.driveTrim = std::clamp(bus.driveTrim, 0.0, 1.0);
                kit->velocityCeiling = std::clamp(kit->velocityCeiling, 0.0, 1.0);
                kit->velocityCurve = std::clamp(kit->velocityCurve, 0.2, 3.0);
            }

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
