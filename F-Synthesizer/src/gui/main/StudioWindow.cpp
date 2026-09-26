#include "StudioWindow.h"
#include "StudioPanels.h"
#include "StudioFiles.h"
#include "StudioFrame.h"
#include "StepSequencer.h"
#include "gui/GUIActions.h"
#include "io/PlatformPaths.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
namespace studio
{
void timeline(GUIState &s, float x, float w)
{
    const float y = 99;
    const auto bars = gui::SongBarTicks(s);
    const int end = std::max(1, bars.back());
    auto barAt = [&](int tick) {
        return std::max(0, static_cast<int>(std::upper_bound(bars.begin(), bars.end(), tick) - bars.begin()) - 1);
    };
    text(x, y - 23, "曲の進行", muted, GetFonts().fontSmall);
    if (s.pianoRoll.previewRangeEnabled)
    {
        const std::string range = std::to_string(barAt(s.pianoRoll.previewRangeStartTick) + 1) + "–" +
                                  std::to_string(barAt(s.pianoRoll.previewRangeEndTick - 1) + 1) + " 小節";
        text(x + 140, y - 23, range.c_str(), accent, GetFonts().fontSmall);
    }
    box(x, y, w, 17, scope);
    if (s.pianoRoll.previewRangeEnabled)
        box(x + w * s.pianoRoll.previewRangeStartTick / end, y,
            w * (s.pianoRoll.previewRangeEndTick - s.pianoRoll.previewRangeStartTick) / end, 17, raised);
    const int step = std::max(1, static_cast<int>(bars.size() / 80));
    for (size_t i = 0; i < bars.size(); i += step)
        line(x + w * bars[i] / end, y, x + w * bars[i] / end, y + 17, edge);
    const float position = x + w * s.songCursorTick / end;
    line(position, y - 3, position, y + 20, accent, 2);
    static float startX = 0;
    static int startTick = 0;
    static bool dragged = false;
    at(x, y - 3);
    ImGui::InvisibleButton("timeline", {w, 25});
    const auto tickAt = [&](float px) { return std::clamp(static_cast<int>((px - x) / w * end), 0, end); };
    if (ImGui::IsItemActivated())
    {
        startX = ImGui::GetIO().MousePos.x;
        startTick = tickAt(startX);
        dragged = false;
    }
    if (ImGui::IsItemActive() && std::abs(ImGui::GetIO().MousePos.x - startX) > 5)
    {
        dragged = true;
        const int target = tickAt(ImGui::GetIO().MousePos.x);
        s.pianoRoll.previewRangeStartTick =
            bars[std::min(barAt(std::min(startTick, target)), static_cast<int>(bars.size()) - 2)];
        s.pianoRoll.previewRangeEndTick =
            bars[std::min(barAt(std::max(startTick, target)) + 1, static_cast<int>(bars.size()) - 1)];
    }
    if (ImGui::IsItemDeactivated())
    {
        if (dragged)
        {
            const bool playing = gui::SongIsPlaying(s);
            s.pianoRoll.previewRangeEnabled = s.previewLoop = true;
            s.songCursorTick = s.pianoRoll.previewRangeStartTick;
            if (playing)
                gui::RequestSongPlayback(s);
        }
        else
            gui::SeekSong(s, startTick);
    }
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
        ImGui::SetTooltip("クリックで移動 / ドラッグで小節単位の繰り返し範囲を選択");
}

void DrawMainWindowFrame(GUIState &s, FileActions &files, WindowFrame &frame)
{
    using namespace studio;
    studio::style();
    gui::InitializeToneWorkspace(s);
    const float width = ImGui::GetIO().DisplaySize.x, height = ImGui::GetIO().DisplaySize.y;
    ImGui::SetNextWindowPos({0, 0});
    ImGui::SetNextWindowSize({width, height});
    ImGui::Begin("F-Synthesizer", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoScrollbar);
    line(0, 72, width, 72, separator);
    box(24, 26, 5, 26, accent);
    box(35, 21, 9, 31, accent);
    text(56, 23, "F-Synthesizer", fg, GetFonts().heading);
    line(224, 21, 224, 60);
    const std::string name = s.activeProjectPath.empty() ? (s.songMidiName.empty() ? "名前のない曲" : s.songMidiName)
                                                         : PathToUtf8(Utf8ToPath(s.activeProjectPath).stem());
    clipped(244, 20, width - 1049, (name + (s.presetDirty ? " *" : "")).c_str(), fg, GetFonts().heading);
    const int pending = gui::PendingToneCount(s);
    const std::string status = pending ? "未採用 " + std::to_string(pending) + " ch" : "";
    text(244, 49, status.c_str(), pendingColor, GetFonts().fontSmall);
    const int seconds = static_cast<int>(gui::SongSecondsAtTick(s, s.songCursorTick)),
              duration = static_cast<int>(gui::SongSecondsAtTick(s, s.pianoRoll.maxTick));
    char timer[40];
    std::snprintf(timer, sizeof(timer), "%02d:%02d / %02d:%02d", seconds / 60, seconds % 60, duration / 60,
                  duration % 60);
    text(width - 785, 21 + (45 - GetFonts().body->FontSize) / 2, timer, fg);
    const bool exporting = s.running && !s.runIsPreview;
    ImGui::BeginDisabled(exporting);
    if (button(gui::SongIsPlaying(s) ? "一時停止" : "再生", width - 628, 21, 100, 45, false, true,
               gui::SongIsPlaying(s) ? PauseIcon : PlayIcon))
    {
        if (gui::SongIsPlaying(s))
            gui::PauseSongPlayback(s);
        else
            gui::RequestSongPlayback(s);
    }
    if (button("区間リピート", width - 516, 21, 128, 45, s.previewLoop))
    {
        const bool playing = gui::SongIsPlaying(s);
        s.previewLoop = !s.previewLoop;
        if (s.previewLoop && s.pianoRoll.previewRangeEndTick <= s.pianoRoll.previewRangeStartTick)
        {
            s.pianoRoll.previewRangeStartTick = 0;
            s.pianoRoll.previewRangeEndTick = std::max(1, s.pianoRoll.maxTick);
        }
        s.pianoRoll.previewRangeEnabled = s.previewLoop;
        if (playing)
            gui::RequestSongPlayback(s);
    }
    if (button("保存", width - 376, 21, 84, 45, false, false, SaveIcon))
        files.requestOperation(s, 1);
    if (button(exporting ? "書出中" : "WAV", width - 282, 21, 78, 45))
        files.requestOperation(s, 3);
    ImGui::EndDisabled();
    if (button("…", width - 194, 21, 44, 45))
        ImGui::OpenPopup("song_menu");
    frame.drawControls(width);
    bool settings = false, help = false;
    if (ImGui::BeginPopup("song_menu"))
    {
        ImGui::BeginDisabled(s.running || s.playback.playing.load() || s.transportAction != gui::TransportAction::None);
        if (ImGui::MenuItem("MIDIを開く"))
            files.chooseFile(s, false);
        if (ImGui::MenuItem("曲を開く"))
            files.chooseFile(s, true);
        ImGui::EndDisabled();
        if (ImGui::MenuItem("別名で保存"))
            files.requestOperation(s, 2);
        settings = ImGui::MenuItem("設定");
        help = ImGui::MenuItem("操作方法");
        ImGui::EndPopup();
    }
    if (settings)
        ImGui::OpenPopup("設定");
    if (help)
        ImGui::OpenPopup("操作方法");
    constexpr float x = 24, gap = 32;
    const float w = width - 48;
    // 一覧は読みやすい幅に保ち、広い画面は波形・音符編集へ割り当てる。
    const float sideW = std::clamp(width / 3, 440.f, 560.f), leftW = w - sideW - gap,
                rightX = x + leftW + gap;
    timeline(s, x, w);
    channelStrip(s, x, w);
    line(x, 200, x + w, 200, separator);
    box(rightX - 12, 208, sideW + 24, height - 216, sidebar);
    line(rightX - gap / 2, 208, rightX - gap / 2, height - 8, separator);
    // 操作部はまとまりごと広げる。低いウィンドウでは追加調整と波形の高さを優先する。
    const float controlsGrowth = std::clamp(std::min((width - 1440.f) / 480, (height - 720.f) / 180), 0.f, 1.f);
    const float controlsScale = 1 + .2f * controlsGrowth, controlsW = 720 * controlsScale;
    float actionsY = height - 14 - 52 * controlsScale;
    constexpr float top = 324;
    const int ch = s.pianoRoll.displayChannel;
    auto &part = s.tones[ch];
    const auto &audible = gui::AudibleInstrument(s, ch);
    const int category = categoryIndex(part.category);
    const std::string label = "ch " + std::to_string(ch + 1) + " / " + categoryLabels[category];
    at(x, 217);
    if (ImGui::InvisibleButton("part_category", {170, 24}, ImGuiButtonFlags_EnableNav))
        ImGui::OpenPopup("part_category");
    const bool categoryHovered = ImGui::IsItemHovered();
    const bool categoryHighlighted = categoryHovered || ImGui::IsItemFocused() || ImGui::IsPopupOpen("part_category");
    if (categoryHighlighted)
        box(x, 217, 170, 24, panel);
    text(x, 218, label.c_str(), categoryHighlighted ? fg : muted, GetFonts().fontSmall);
    icon(DownIcon, x + 144, 223, 12, categoryHighlighted ? fg : muted);
    if (categoryHovered)
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
    if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal))
        ImGui::SetTooltip("パートの分類を変更\nプリセット一覧の初期絞り込みに使います");
    if (ImGui::BeginPopup("part_category"))
    {
        ImGui::TextUnformatted("パートの分類");
        ImGui::Separator();
        for (int i = 0; i < 8; ++i)
            if (ImGui::Selectable(categoryLabels[i], category == i))
            {
                part.category = categories[i];
                s.presetDirty = true;
            }
        ImGui::EndPopup();
    }
    clipped(x, 236, leftW - 256, toneName(audible).c_str(), fg, GetFonts().heading);
    if (tab("音色", x + leftW - 238, 230, 95, 38, !s.toneNotesOpen))
        s.toneNotesOpen = false;
    if (tab("音符を編集", x + leftW - 133, 230, 133, 38, s.toneNotesOpen))
        s.toneNotesOpen = true;
    mixControls(s, x, 278);
    if (s.toneNotesOpen)
    {
        static bool firstNotes = true;
        if (firstNotes)
        {
            s.pianoRoll.pixelsPerQuarter = std::max(96.f, s.pianoRoll.pixelsPerQuarter);
            s.pianoRoll.tickOffset = std::max(0, s.songCursorTick - s.pianoRoll.ticksPerQuarter);
            firstNotes = false;
        }
        at(x, top);
        ImGui::BeginChild("notes", {leftW, actionsY - top}, false);
        if (ch == 9 && ImGui::Checkbox("16ステップで編集", &s.stepSeq.viewActive) && s.stepSeq.viewActive)
        {
            const int span = std::max(1, s.pianoRoll.ticksPerQuarter * 4);
            s.stepSeq.startTick = s.songCursorTick / span * span;
            LoadStepSeqFromPianoRoll(s.stepSeq, s.pianoRoll);
        }
        if (ch == 9 && s.stepSeq.viewActive)
        {
            ImGui::PushFont(GetFonts().fontSmall);
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {6, 2});
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {8, 4});
            DrawStepSeqPanel(s);
            ImGui::PopStyleVar(2);
            ImGui::PopFont();
        }
        else
            gui::DrawPianoRollPanel(
                s.pianoRoll, s.midiPath, s.toneAuditionActive ? nullptr : &s.playback,
                [&](const std::string &log) { gui::AppendGUILog(s, log); }, [&] { gui::RequestSongPlayback(s); },
                [&] { gui::PauseSongPlayback(s); });
        ImGui::EndChild();
    }
    else
    {
        actionsY = toneEditor(s, x, top, leftW, controlsW, actionsY - top - 12 * controlsScale);
    }
    presetList(s, rightX, 232, sideW, height - 252);
    toneActions(s, x, actionsY, controlsW);
    const auto &io = ImGui::GetIO();
    if (!io.WantTextInput && !ImGui::IsAnyItemActive() &&
        !ImGui::IsPopupOpen(nullptr, ImGuiPopupFlags_AnyPopupId | ImGuiPopupFlags_AnyPopupLevel))
    {
        if (!s.toneNotesOpen && !exporting && ImGui::IsKeyPressed(ImGuiKey_Space, false))
        {
            if (gui::SongIsPlaying(s))
                gui::PauseSongPlayback(s);
            else
                gui::RequestSongPlayback(s);
        }
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false))
            files.requestOperation(s, io.KeyShift ? 2 : 1);
        if (!s.toneNotesOpen && io.KeyCtrl)
        {
            if (ImGui::IsKeyPressed(ImGuiKey_Z, false))
                gui::UndoToneEdit(s);
            if (ImGui::IsKeyPressed(ImGuiKey_Y, false))
                gui::UndoToneEdit(s, true);
        }
    }
    files.dialogs(s);
    files.finishOperation(s);
    if (s.observedNotesVersion && s.observedNotesVersion != s.pianoRoll.notesVersion)
        s.presetDirty = true;
    s.observedNotesVersion = s.pianoRoll.notesVersion;
    ImGui::End();
}

} // namespace studio
