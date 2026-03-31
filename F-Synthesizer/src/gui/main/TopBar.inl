void SetupImGuiFont()
{
    ImGuiIO& io = ImGui::GetIO();
    const ImWchar* ranges = io.Fonts->GetGlyphRangesJapanese();
    const char* candidates[] = {
        "C:\\Windows\\Fonts\\meiryo.ttc",
        "C:\\Windows\\Fonts\\YuGothM.ttc",
        "C:\\Windows\\Fonts\\msgothic.ttc"
    };

    // 日本語表示を崩さないため、Windows既定フォント候補を順に試す。
    for (const char* fontPath : candidates)
    {
        std::error_code ec;
        if (!std::filesystem::exists(fontPath, ec))
        {
            continue;
        }
        ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath, 20.0f, nullptr, ranges);
        if (font != nullptr)
        {
            io.FontDefault = font;
            return;
        }
    }
}

float UIScaleFromIndex(int idx)
{
    switch (idx)
    {
    case 0: return 1.0f;
    case 1: return 1.25f;
    case 2: return 1.5f;
    default: return 1.25f;
    }
}

const char* UIScaleLabelFromIndex(int idx)
{
    switch (idx)
    {
    case 0: return "100%";
    case 1: return "125%";
    case 2: return "150%";
    default: return "125%";
    }
}

void DrawStatusBadge(const GUIState& state)
{
    ImVec4 color = ImVec4(0.55f, 0.55f, 0.55f, 1.0f);
    const char* label = "待機中";

    if (state.running)
    {
        color = ImVec4(0.95f, 0.78f, 0.2f, 1.0f);
        label = "実行中";
    }
    else if (state.playback.playing.load(std::memory_order_relaxed))
    {
        color = ImVec4(0.28f, 0.82f, 0.95f, 1.0f);
        label = state.previewLoop ? "試聴中（ループ）" : "試聴中";
    }
    else if (state.hasRun && state.lastRunExitCode == 2)
    {
        color = ImVec4(0.95f, 0.70f, 0.25f, 1.0f);
        label = "キャンセル";
    }
    else if (state.hasRun && state.lastRunExitCode == 0)
    {
        color = ImVec4(0.30f, 0.82f, 0.40f, 1.0f);
        label = "成功";
    }
    else if (state.hasRun)
    {
        color = ImVec4(0.95f, 0.35f, 0.35f, 1.0f);
        label = "失敗";
    }

    ImGui::TextUnformatted("状態:");
    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::TextUnformatted(label);
    ImGui::PopStyleColor();
}
