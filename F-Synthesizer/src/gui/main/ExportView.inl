// ExportView.inl
// Export 専用ビュー（UIModeTab == 2 用）。
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
    state.presetDirty |= ImGui::InputText("出力パス", state.wavPath, IM_ARRAYSIZE(state.wavPath));
    updateHoverHelp(
        "WAVの書き出し先パスを指定します。",
        "WAVの出力先が変わります。",
        "Serial Save が無効だと既存ファイルを上書きする場合があります。");
    ImGui::SameLine();
    if (ImGui::Button("参照..."))
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
        "出力パスを更新します。",
        "保存権限のない場所は失敗します。");
    ImGui::SameLine();
    if (ImGui::Button("コピー##wavPath"))
    {
        ImGui::SetClipboardText(state.wavPath);
    }
    updateHoverHelp("出力パス をコピーします。", "パスをクリップボードへコピーします。", nullptr);
    {
        const std::string compact = CompactPathForUI(state.wavPath);
        ImGui::TextDisabled("%s", compact.c_str());
        if (ImGui::IsItemHovered() && std::strlen(state.wavPath) > 0)
        {
            ImGui::SetTooltip("%s", state.wavPath);
        }
    }

    // ---- フォーマットサマリー + 詳細 ----
    ImGui::Separator();
    ImGui::TextDisabled("形式: %d Hz / %dbit   連番保存: %s",
        state.sampleRate, state.bits, state.serialSave ? "ON" : "OFF");
    updateHoverHelp(
        "現在のフォーマット設定のサマリーです。",
        "詳細設定を開くと形式と余韻を変更できます。",
        nullptr);
    if (ImGui::CollapsingHeader("詳細設定"))
    {
        state.presetDirty |= ImGui::InputInt("サンプルレート", &state.sampleRate);
        updateHoverHelp(
            "出力サンプルレートを設定します。",
            "音質・負荷・サイズが変わります。",
            "高すぎる値は処理時間を増やします。");
        state.presetDirty |= ImGui::InputInt("最小秒数", &state.initialSeconds);
        updateHoverHelp(
            "最低レンダ秒数を設定します。",
            "出力の最小長さを確保します。",
            "短くしすぎると余韻が切れやすくなります。");
        state.presetDirty |= ImGui::InputInt("ビット深度", &state.bits);
        updateHoverHelp(
            "出力ビット深度を設定します。",
            "ダイナミックレンジと互換性が変わります。",
            "現行実装は16bitのみ有効です。");
        state.presetDirty |= ImGui::InputFloat("追加リリース (秒)", &state.extraReleaseSec, 0.01f, 0.1f, "%.2f");
        updateHoverHelp(
            "ノート終端後の追加リリース時間を設定します。",
            "尻切れを抑え、余韻を確保します。",
            "長くしすぎると書き出し時間とサイズが増えます。");
        state.presetDirty |= ImGui::Checkbox("連番保存", &state.serialSave);
        updateHoverHelp(
            "連番保存を切り替えます。",
            "同名出力の上書きを避けます。",
            "無効時は同名ファイルを上書きします。");
    }

    // ---- 出力対象 ----
    ImGui::Separator();
    ImGui::TextUnformatted("出力対象");
    const int outputMode = (state.targetChannel < 0) ? 0 : 1;
    if (ImGui::RadioButton("全チャンネル", outputMode == 0))
    {
        state.targetChannel = -1;
        state.presetDirty = true;
    }
    updateHoverHelp(
        "出力対象を 全チャンネル にします。",
        "全MIDIチャンネルを出力します。",
        nullptr);
    ImGui::SameLine();
    if (ImGui::RadioButton("単一チャンネル", outputMode == 1))
    {
        state.targetChannel = std::clamp(state.targetChannel, 0, 15);
        if (state.targetChannel < 0)
        {
            state.targetChannel = 0;
        }
        state.presetDirty = true;
    }
    updateHoverHelp(
        "出力対象を 単一チャンネル にします。",
        "指定chのみを出力します。",
        nullptr);
    if (state.targetChannel >= 0)
    {
        ImGui::SetNextItemWidth(220.0f);
        int singleTarget = std::clamp(state.targetChannel, 0, 15);
        if (ImGui::SliderInt("対象ch##export", &singleTarget, 0, 15))
        {
            state.targetChannel = singleTarget;
            state.presetDirty = true;
        }
        updateHoverHelp(
            "書き出し対象のチャンネルを変更します。",
            "書き出し対象chが変わります。",
            nullptr);
    }

    ImGui::EndDisabled();

    // ---- ボタン ----
    ImGui::Separator();
    ImGui::BeginDisabled(state.running);
    if (ImGui::Button("WAV書き出し"))
    {
        StartGUIRun(state, false);
    }
    updateHoverHelp(
        "WAV書き出しを実行します。",
        "現在設定でWAVを書き出します。",
        "再生中の場合は停止してから書き出しを開始します。");
    ImGui::SameLine();
    if (ImGui::Button("プレビュー再生"))
    {
        StartGUIRun(state, true);
    }
    updateHoverHelp(
        "プレビュー再生を実行します。",
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
