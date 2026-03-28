// VirtualKeyboard.inl
// 仮想キーボード: C2(36)〜C6(84) の鍵盤を DrawList で描画し、
// クリック時に tonePreviewNoteNumber を更新して即時 Tone Preview を発火する。
// DrawVirtualKeyboard(state) を MainWindow.inl の Sound タブ内から呼び出す。
//
// 前提: GUIMain.cpp の using gui::StartGUISoundTonePreview; が参照可能であること。

static void DrawVirtualKeyboard(GUIState& state)
{
    // DrumKit の場合は非表示（既存 Preview Note スライダーと同じ条件）
    if (state.channelConfigs)
    {
        const int slot = std::clamp(state.selectedSoundSlot, 0, 15);
        if (std::holds_alternative<DrumKitConfig>((*state.channelConfigs)[slot].source))
        {
            return;
        }
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

    ImGui::Separator();
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    const float totalW = static_cast<float>(totalWhite) * kWhiteW;

    // 鍵盤全体を覆う InvisibleButton でクリック受付
    ImGui::InvisibleButton("##vkb", ImVec2(totalW, kWhiteH));
    const bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
    const ImVec2 mpos = ImGui::GetMousePos();

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImU32 cWhite = IM_COL32(235, 235, 235, 255);
    const ImU32 cBlack = IM_COL32(30, 30, 30, 255);
    const ImU32 cSel = IM_COL32(100, 170, 255, 255);
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
        dl->AddRectFilled(
            { x0, origin.y },
            { x1, y1 },
            (state.tonePreviewNoteNumber == n) ? cSel : cWhite);
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
        dl->AddRectFilled(
            { x0, origin.y },
            { x1, y1 },
            (state.tonePreviewNoteNumber == n) ? cSel : cBlack);
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
