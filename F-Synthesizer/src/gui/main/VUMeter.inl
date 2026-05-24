// VUMeter.inl
// Tone Preview / Play Preview 再生中のリアルタイムレベルメーターを描画する。
// DrawVUMeter(state) を MainWindow.inl の左カラム末尾から呼び出す。
//
// GUIState への追加なし。静的変数で peak hold・clip 状態を管理する。

static void DrawVUMeter(GUIState& state)
{
    if (!state.previewRenderedSound && !state.playback.playing.load(std::memory_order_relaxed))
    {
        return;
    }

    // ---- 1. 今フレームの peak を計算 ----
    constexpr int kWindowFrames = 512;
    float instantPeak = GetPreviewPlaybackPeak(state.playback);
    bool clipInWindow = instantPeak > 1.0f;
    if (instantPeak <= 0.0f && state.previewRenderedSound)
    {
        const SoundData& sd = *state.previewRenderedSound;
        const auto& src = sd.dataL.empty() ? sd.data : sd.dataL;
        const int srcSize = static_cast<int>(src.size());
        int windowStart = 0;
        if (state.playback.playing.load(std::memory_order_relaxed))
        {
            const int cursor = static_cast<int>(
                state.playback.frameCursor.load(std::memory_order_relaxed));
            windowStart = std::clamp(cursor - kWindowFrames / 2, 0,
                std::max(0, srcSize - kWindowFrames));
        }
        const int windowEnd = std::min(windowStart + kWindowFrames, srcSize);

        for (int i = windowStart; i < windowEnd; ++i)
        {
            const float s = static_cast<float>(std::abs(src[i]));
            if (s > instantPeak)
            {
                instantPeak = s;
            }
            if (s > 1.0f)
            {
                clipInWindow = true;
            }
        }
    }

    // ---- 2. 静的状態の更新 ----
    static float vuLevel = 0.0f; // 表示レベル（ファストアタック/スローデケイ）
    static float vuPeakHold = 0.0f; // ピークホールド値
    static float vuPeakTimer = 0.0f; // ピークホールドカウントダウン(秒)
    static bool vuClipFlag = false;
    static uint64_t vuLastSession = 0; // クリップリセット用

    // 新しい sound がセットされたらクリップフラグをリセット
    const uint64_t session = state.playback.sessionGeneration.load(std::memory_order_relaxed);
    if (session != vuLastSession)
    {
        vuLevel = 0.0f;
        vuPeakHold = 0.0f;
        vuPeakTimer = 0.0f;
        vuClipFlag = false;
        vuLastSession = session;
    }

    const float dt = ImGui::GetIO().DeltaTime;

    if (state.playback.playing.load(std::memory_order_relaxed))
    {
        // ファストアタック: 即座にスナップ
        if (instantPeak > vuLevel)
        {
            vuLevel = instantPeak;
        }
        // スローデケイ: ~12 dB/sec (約 0.25 倍/sec)
        else
        {
            vuLevel = std::max(0.0f, vuLevel - dt * 0.8f);
        }

        // ピークホールド
        if (vuLevel > vuPeakHold)
        {
            vuPeakHold = vuLevel;
            vuPeakTimer = 1.5f; // 1.5 秒保持
        }
        else if (vuPeakTimer > 0.0f)
        {
            vuPeakTimer -= dt;
        }
        else
        {
            vuPeakHold = std::max(0.0f, vuPeakHold - dt * 0.8f);
        }

        if (clipInWindow)
        {
            vuClipFlag = true;
        }
    }
    else
    {
        // 停止後はフェードアウト
        vuLevel = std::max(0.0f, vuLevel - dt * 2.0f);
        vuPeakHold = std::max(0.0f, vuPeakHold - dt * 2.0f);
        if (vuPeakTimer > 0.0f)
        {
            vuPeakTimer -= dt;
        }
    }

    // ---- 3. 描画 ----
    ImGui::Separator();
    ImGui::TextDisabled("レベル");

    // バーの色: 0..0.7 = 緑、0.7..0.9 = 黄、0.9.. = 赤
    const ImVec4 barColor = (vuLevel > 0.9f) ? ImVec4(0.9f, 0.25f, 0.25f, 1.0f)
        : (vuLevel > 0.7f) ? ImVec4(0.9f, 0.8f, 0.1f, 1.0f)
        : ImVec4(0.2f, 0.75f, 0.3f, 1.0f);

    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
    ImGui::ProgressBar(vuLevel, ImVec2(-1.0f, 10.0f), "");
    ImGui::PopStyleColor();

    // ピークホールドマーカーをオーバーレイ（DrawList で縦線）
    if (vuPeakHold > 0.01f)
    {
        const ImVec2 barMin = ImGui::GetItemRectMin();
        const ImVec2 barMax = ImGui::GetItemRectMax();
        const float px = barMin.x + (barMax.x - barMin.x) * std::min(vuPeakHold, 1.0f);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddLine({ px, barMin.y }, { px, barMax.y },
            IM_COL32(255, 255, 255, 200), 2.0f);
    }

    // クリップ表示
    if (vuClipFlag)
    {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.95f, 0.3f, 0.3f, 1.0f), "CLIP");
    }
}
