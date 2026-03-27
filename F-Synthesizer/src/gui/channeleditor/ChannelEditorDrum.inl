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
