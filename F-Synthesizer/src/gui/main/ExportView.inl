// ExportView.inl
// 「書き出す」専用ビュー（UIModeTab == 2 用）。
// DrawExportView(state, updateHoverHelp) を MainWindow.inl の Export ブランチから呼び出す。
// GUIMain.cpp 匿名名前空間に VUMeter.inl の後に #include して使用する。

#include <cstdio>
#include <cstring>
#include <filesystem>
#include <functional>

static void DrawExportView(
    GUIState& state,
    const std::function<void(const char*, const char*, const char*)>& updateHoverHelp)
{
    ImGui::BeginDisabled(state.running);

    // ---- 出力先パス ----
    state.presetDirty |= ImGui::InputText("Output Path", state.wavPath, IM_ARRAYSIZE(state.wavPath));
    updateHoverHelp(
        "WAVの書き出し先パスを指定します。",
        "WAVの出力先が変わります。",
        "Serial Save が無効だと既存ファイルを上書きする場合があります。");
    ImGui::SameLine();
    if (ImGui::Button("Browse..."))
    {
        std::string selected;
        const wchar_t* wavFilter = L"WAV Files (*.wav)\0*.wav\0All Files (*.*)\0*.*\0";
        if (BrowseSavePath(state.wavPath, wavFilter, L"wav", selected))
        {
            strncpy_s(state.wavPath, sizeof(state.wavPath), selected.c_str(), _TRUNCATE);
            state.presetDirty = true;
        }
    }
    updateHoverHelp(
        "出力先WAVパスを選択するダイアログを開きます。",
        "Output Pathを更新します。",
        "保存権限のない場所は失敗します。");
    ImGui::SameLine();
    if (ImGui::Button("Copy##wavPath"))
    {
        ImGui::SetClipboardText(state.wavPath);
    }
    updateHoverHelp("Output Path をコピーします。", "パスをクリップボードへコピーします。", nullptr);
    {
        const std::string compact = CompactPathForUI(state.wavPath);
        ImGui::TextDisabled("%s", compact.c_str());
        if (ImGui::IsItemHovered() && std::strlen(state.wavPath) > 0)
        {
            ImGui::SetTooltip("%s", state.wavPath);
        }
    }

    // ---- フォーマットサマリー（読み取り専用） ----
    ImGui::Separator();
    ImGui::TextDisabled("Format: %d Hz / %dbit   Serial Save: %s",
        state.sampleRate, state.bits, state.serialSave ? "ON" : "OFF");
    updateHoverHelp(
        "現在のフォーマット設定のサマリーです。",
        "詳細設定は Music タブの Render Settings で変更できます。",
        nullptr);

    // ---- 出力対象 ----
    ImGui::Separator();
    ImGui::TextUnformatted("Output Target");
    const int outputMode = (state.targetChannel < 0) ? 0 : 1;
    if (ImGui::RadioButton("All Channels", outputMode == 0))
    {
        state.targetChannel = -1;
        state.presetDirty = true;
    }
    updateHoverHelp(
        "出力対象を All Channels にします。",
        "全MIDIチャンネルを出力します。",
        nullptr);
    ImGui::SameLine();
    if (ImGui::RadioButton("Single Channel", outputMode == 1))
    {
        state.targetChannel = std::clamp(state.targetChannel, 0, 15);
        if (state.targetChannel < 0)
        {
            state.targetChannel = 0;
        }
        state.presetDirty = true;
    }
    updateHoverHelp(
        "出力対象を Single Channel にします。",
        "指定chのみを出力します。",
        nullptr);
    if (state.targetChannel >= 0)
    {
        ImGui::SetNextItemWidth(220.0f);
        int singleTarget = std::clamp(state.targetChannel, 0, 15);
        if (ImGui::SliderInt("Target Ch##export", &singleTarget, 0, 15))
        {
            state.targetChannel = singleTarget;
            state.presetDirty = true;
        }
        updateHoverHelp(
            "書き出し対象のチャンネルを変更します。",
            "Export対象chが変わります。",
            nullptr);
    }

    ImGui::EndDisabled();

    // ---- ボタン ----
    ImGui::Separator();
    ImGui::BeginDisabled(state.running);
    if (ImGui::Button("Export WAV"))
    {
        StartGUIRun(state, false);
    }
    updateHoverHelp(
        "Export WAVを実行します。",
        "現在設定でWAVを書き出します。",
        "再生中の場合は停止してから書き出しを開始します。");
    ImGui::SameLine();
    if (ImGui::Button("Play Preview"))
    {
        StartGUIRun(state, true);
    }
    updateHoverHelp(
        "Play Previewを実行します。",
        "現在のMIDI設定でメモリ再生します。",
        "WAVファイルは出力しません。");
    ImGui::EndDisabled();
    ImGui::SameLine();
    const bool canStop = state.running || state.playback.playing.load(std::memory_order_relaxed);
    ImGui::BeginDisabled(!canStop);
    if (ImGui::Button("Stop##export"))
    {
        StopGUIRunAndPreview(state);
    }
    updateHoverHelp(
        "レンダまたは再生を停止します。",
        "処理を中断します。",
        "未実行時は無効です。");
    ImGui::EndDisabled();

    // ---- VU メーター（Phase 1-C の DrawVUMeter を流用） ----
    DrawVUMeter(state);

    // ---- 最近の書き出し ----
    if (!state.recentWavPaths.empty())
    {
        ImGui::Separator();
        ImGui::TextDisabled("最近の書き出し:");
        for (const std::string& path : state.recentWavPaths)
        {
            std::string label = CompactPathForUI(path);
            std::error_code ec;
            const auto sz = std::filesystem::file_size(path, ec);
            if (!ec && sz > 0)
            {
                const double mb = static_cast<double>(sz) / (1024.0 * 1024.0);
                char sizeBuf[32];
                snprintf(sizeBuf, sizeof(sizeBuf), " (%.1f MB)", mb);
                label += sizeBuf;
            }
            ImGui::TextUnformatted(label.c_str());
            if (ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s", path.c_str());
            }
        }
    }
}
