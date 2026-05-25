// VirtualKeyboard.inl
// 仮想キーボード: C2(36)〜C6(84) の鍵盤を DrawList で描画し、
// クリック時に tonePreviewNoteNumber を更新して即時 Tone Preview を発火する。
// DrawVirtualKeyboard(state) を MainWindow.inl の Sound タブ内から呼び出す。
//
// 前提: GUIMain.cpp の using gui::StartGUISoundTonePreview; が参照可能であること。

static const char* DrumPadLabel(DrumType type)
{
    switch (type)
    {
    case DrumType::Kick: return "Kick";
    case DrumType::Snare: return "Snare";
    case DrumType::Hat: return "Hat";
    case DrumType::Tom: return "Tom";
    case DrumType::Rim: return "Rim";
    case DrumType::Clap: return "Clap";
    case DrumType::Crash: return "Crash";
    case DrumType::Ride: return "Ride";
    default: return "Drum";
    }
}

static void DrawDrumPadPreview(GUIState& state)
{
    const int slot = std::clamp(state.selectedSoundSlot, 0, 15);
    const auto* kit = std::get_if<DrumKitConfig>(&gui::ReadSoundSlot(state, slot).source);
    if (kit == nullptr)
    {
        return;
    }

    ImGui::Separator();
    ImGui::TextUnformatted("Drum Pads");
    ImGui::TextDisabled("鳴らしたい音を選んで試聴します。");

    const float padW = 112.0f;
    const float padH = 48.0f;
    const float spacing = ImGui::GetStyle().ItemSpacing.x;
    const float startX = ImGui::GetCursorPosX();
    const float availW = ImGui::GetContentRegionAvail().x;
    int visibleCount = 0;

    for (int note = 0; note < 128; ++note)
    {
        const DrumConfig& drum = kit->map[note];
        if (drum.type == DrumType::None)
        {
            continue;
        }

        if (visibleCount > 0)
        {
            const float nextX = ImGui::GetCursorPosX() + padW + spacing;
            if (nextX - startX <= availW)
            {
                ImGui::SameLine();
            }
        }

        ImGui::PushID(note);
        const bool selected = std::clamp(state.selectedDrumNote, 0, 127) == note;
        if (selected)
        {
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
        }
        const std::string label = std::string(DrumPadLabel(drum.type)) + "\nnote " + std::to_string(note);
        if (ImGui::Button(label.c_str(), ImVec2(padW, padH)))
        {
            state.selectedDrumNote = note;
            StartGUISoundTonePreview(state);
        }
        if (selected)
        {
            ImGui::PopStyleColor();
        }
        ImGui::PopID();
        ++visibleCount;
    }

    if (visibleCount == 0)
    {
        ImGui::TextDisabled("鳴らせるドラム音がありません。");
        if (ImGui::Button("Advancedで編集"))
        {
            state.UIModeTab = 3;
        }
    }
}

static void DrawVirtualKeyboard(GUIState& state)
{
    // DrumKit の場合は非表示（既存 Preview Note スライダーと同じ条件）
    const int slot = std::clamp(state.selectedSoundSlot, 0, 15);
    if (config::UsesDrumKitNoteSelection(gui::ReadSoundSlot(state, slot).source))
    {
        return;
    }

    constexpr int kFirstNote = 36;    // C2
    constexpr int kLastNote = 84;     // C6 （49鍵：4オクターブ + C）
    constexpr float kWhiteW = 14.0f;  // 白鍵の幅
    constexpr float kWhiteH = 48.0f;  // 白鍵の高さ
    constexpr float kBlackW = 9.0f;   // 黒鍵の幅
    constexpr float kBlackH = 30.0f;  // 黒鍵の高さ

    // semitone % 12 が白鍵か
    // 白鍵: C=0, D=2, E=4, F=5, G=7, A=9, B=11
    auto isWhite = [](int n) -> bool
    {
        const int s = n % 12;
        return s == 0 || s == 2 || s == 4 || s == 5 || s == 7 || s == 9 || s == 11;
    };

    // note より前（kFirstNote 以降）の白鍵数を返す。
    // 黒鍵の x 座標 = origin + whitesBefore(n) * kWhiteW - kBlackW/2
    // （左隣の白鍵の右端を基準とした中央揃え）
    auto whitesBefore = [&](int note) -> int
    {
        int c = 0;
        for (int n = kFirstNote; n < note; ++n)
        {
            if (isWhite(n))
            {
                ++c;
            }
        }
        return c;
    };

    int totalWhite = 0;
    for (int n = kFirstNote; n <= kLastNote; ++n)
    {
        if (isWhite(n))
        {
            ++totalWhite;
        }
    }

    // ---- Chord UI ----
    ImGui::Separator();
    const char* chordTypes[] = { "メジャー", "マイナー", "セブンス", "マイナー7th", "サス4" };
    ImGui::Checkbox("コード", &state.chordModeEnabled);
    if (state.chordModeEnabled)
    {
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100.0f);
        ImGui::Combo("##chordType", &state.chordType, chordTypes, IM_ARRAYSIZE(chordTypes));
    }

    // ---- コード構成音 ハイライト判定 ----
    constexpr int kChordOffsets[5][4] = {
        { 0,  4,  7, -1 }, // Major
        { 0,  3,  7, -1 }, // Minor
        { 0,  4,  7, 10 }, // 7th
        { 0,  3,  7, 10 }, // Minor7th
        { 0,  5,  7, -1 }, // Sus4
    };
    constexpr int kChordSizes[5] = { 3, 3, 4, 4, 3 };
    auto isChordMember = [&](int n) -> bool
    {
        if (!state.chordModeEnabled || n == state.tonePreviewNoteNumber)
        {
            return false;
        }
        const int ct = std::clamp(state.chordType, 0, 4);
        for (int i = 0; i < kChordSizes[ct]; ++i)
        {
            if (kChordOffsets[ct][i] >= 0 &&
                n == state.tonePreviewNoteNumber + kChordOffsets[ct][i])
            {
                return true;
            }
        }
        return false;
    };

    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float totalW = static_cast<float>(totalWhite) * kWhiteW;

    // 鍵盤全体を覆う InvisibleButton でクリック受付
    ImGui::InvisibleButton("##vkb", ImVec2(totalW, kWhiteH));
    const bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    const ImVec2 mpos = ImGui::GetMousePos();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImU32 cWhite = IM_COL32(235, 235, 235, 255);
    const ImU32 cBlack = IM_COL32(30, 30, 30, 255);
    const ImU32 cSel = IM_COL32(100, 170, 255, 255);   // ルート（青）
    const ImU32 cChord = IM_COL32(100, 200, 140, 255); // 構成音（緑）
    const ImU32 cBorder = IM_COL32(80, 80, 80, 255);

    // 白鍵を先に描画
    for (int n = kFirstNote; n <= kLastNote; ++n)
    {
        if (!isWhite(n))
        {
            continue;
        }
        const float x0 = origin.x + static_cast<float>(whitesBefore(n)) * kWhiteW;
        const float x1 = x0 + kWhiteW - 1.0f; // 1px 隙間でセパレータ代わり
        const float y1 = origin.y + kWhiteH;
        const ImU32 fill = (state.tonePreviewNoteNumber == n) ? cSel
            : isChordMember(n) ? cChord
            : cWhite;
        dl->AddRectFilled(
            { x0, origin.y },
            { x1, y1 },
            fill);
        dl->AddRect({ x0, origin.y }, { x1, y1 }, cBorder);
    }

    // 黒鍵を白鍵の上に重ねて描画
    for (int n = kFirstNote; n <= kLastNote; ++n)
    {
        if (isWhite(n))
        {
            continue;
        }
        const float cx = origin.x + static_cast<float>(whitesBefore(n)) * kWhiteW;
        const float x0 = cx - kBlackW * 0.5f;
        const float x1 = x0 + kBlackW;
        const float y1 = origin.y + kBlackH;
        const ImU32 fill = (state.tonePreviewNoteNumber == n) ? cSel
            : isChordMember(n) ? cChord
            : cBlack;
        dl->AddRectFilled(
            { x0, origin.y },
            { x1, y1 },
            fill);
        dl->AddRect({ x0, origin.y }, { x1, y1 }, cBorder);
    }

    // クリック判定: 黒鍵優先でヒットテスト
    if (clicked)
    {
        int hit = -1;

        for (int n = kFirstNote; n <= kLastNote && hit < 0; ++n)
        {
            if (isWhite(n))
            {
                continue;
            }
            const float cx = origin.x + static_cast<float>(whitesBefore(n)) * kWhiteW;
            const float x0 = cx - kBlackW * 0.5f;
            if (mpos.x >= x0 && mpos.x < x0 + kBlackW &&
                mpos.y >= origin.y && mpos.y < origin.y + kBlackH)
            {
                hit = n;
            }
        }

        if (hit < 0)
        {
            for (int n = kFirstNote; n <= kLastNote && hit < 0; ++n)
            {
                if (!isWhite(n))
                {
                    continue;
                }
                const float x0 = origin.x + static_cast<float>(whitesBefore(n)) * kWhiteW;
                if (mpos.x >= x0 && mpos.x < x0 + kWhiteW &&
                    mpos.y >= origin.y && mpos.y < origin.y + kWhiteH)
                {
                    hit = n;
                }
            }
        }

        if (hit >= 0)
        {
            state.tonePreviewNoteNumber = hit;
            StartGUISoundTonePreview(state); // Auto Tone Preview のデバウンスを介さず即時再生
        }
    }
}
