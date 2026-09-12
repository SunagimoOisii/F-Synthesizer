#include "StudioWindow.h"
#include "StudioPanels.h"
#include "StudioFiles.h"
#include "StepSequencer.h"
#include "gui/GUIActions.h"
#include "io/PlatformPaths.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
namespace studio
{
void timeline(GUIState &s, float width)
{
    const float x = 24, y = 99, w = width - 48;
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

void DrawMainWindowFrame(GUIState &s, FileActions &files)
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
    box(24, 26, 5, 26, accent);
    box(35, 21, 9, 31, accent);
    text(56, 23, "F-Synthesizer", fg, GetFonts().heading);
    line(259, 21, 259, 60);
    const std::string name = s.activeProjectPath.empty() ? (s.songMidiName.empty() ? "名前のない曲" : s.songMidiName)
                                                         : PathToUtf8(Utf8ToPath(s.activeProjectPath).stem());
    clipped(281, 20, width - 1038, (name + (s.presetDirty ? " *" : "")).c_str(), fg, GetFonts().heading);
    const int pending = gui::PendingToneCount(s);
    const std::string status = pending ? "未採用 " + std::to_string(pending) + " ch" : "";
    text(281, 49, status.c_str(), pendingColor, GetFonts().fontSmall);
    const int seconds = static_cast<int>(gui::SongSecondsAtTick(s, s.songCursorTick)),
              duration = static_cast<int>(gui::SongSecondsAtTick(s, s.pianoRoll.maxTick));
    char timer[40];
    std::snprintf(timer, sizeof(timer), "%02d:%02d / %02d:%02d", seconds / 60, seconds % 60, duration / 60,
                  duration % 60);
    text(width - 739, 30, timer, fg, GetFonts().heading);
    const bool exporting = s.running && !s.runIsPreview;
    ImGui::BeginDisabled(exporting);
    if (button(gui::SongIsPlaying(s) ? "一時停止" : "再生", width - 560, 21, 115, 45, false, true,
               gui::SongIsPlaying(s) ? PauseIcon : PlayIcon))
    {
        if (gui::SongIsPlaying(s))
            gui::PauseSongPlayback(s);
        else
            gui::RequestSongPlayback(s);
    }
    if (button("区間リピート", width - 435, 21, 139, 45, s.previewLoop))
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
    if (button("保存", width - 278, 21, 95, 45, false, false, SaveIcon))
        files.requestOperation(s, 1);
    if (button("WAV", width - 173, 21, 84, 45))
        files.requestOperation(s, 3);
    ImGui::EndDisabled();
    if (button("…", width - 77, 21, 53, 45))
        ImGui::OpenPopup("song_menu");
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
    timeline(s, width);
    channelStrip(s, width);
    const float x = 24, w = width - 48, sideW = std::min(410.f, width * .285f), gap = 24, leftW = w - sideW - gap,
                rightX = x + leftW + gap;
    const float top = 312, favoritesY = height - 196, contentH = favoritesY - 18 - top;
    const int ch = s.pianoRoll.displayChannel;
    auto &part = s.tones[ch];
    const auto &audible = gui::AudibleInstrument(s, ch);
    const int category = categoryIndex(part.category);
    icon(categoryGlyphs[category], x + 2, 225, 32);
    const std::string label = "ch " + std::to_string(ch + 1) + " / " + categoryLabels[category];
    text(x + 50, 206, label.c_str(), muted, GetFonts().fontSmall);
    icon(DownIcon, x + 192, 211, 12, muted);
    at(x + 48, 205);
    ImGui::InvisibleButton("part_category", {170, 24});
    if (ImGui::IsItemClicked())
        ImGui::OpenPopup("part_category");
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
    clipped(x + 48, 228, leftW - 304, toneName(audible).c_str(), fg, GetFonts().title);
    if (button("音色", x + leftW - 238, 218, 95, 38, !s.toneNotesOpen))
        s.toneNotesOpen = false;
    if (button("音符を編集", x + leftW - 133, 218, 133, 38, s.toneNotesOpen))
        s.toneNotesOpen = true;
    mixControls(s, x, 266, leftW);
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
        ImGui::BeginChild("notes", {leftW, contentH}, false);
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
        const float waveH = contentH - 124 - (s.toneExtraOpen ? 94 : 0);
        waveform(s, x, top, leftW, waveH);
        toneControls(s, x, top + waveH + 8, leftW);
        if (s.toneExtraOpen)
            extraControls(s, x, top + contentH - 92, leftW);
    }
    presetList(s, rightX, 220, sideW, top + contentH - 220);
    files.favorites(s, x, favoritesY, w);
    const float fy = height - 72;
    box(0, fy, width, 72, panel);
    line(24, fy, width - 24, fy);
    box(24, fy + 15, 3, 40, gui::TonePending(s, ch) ? pendingColor : accent);
    const std::string selectedState = "ch " + std::to_string(ch + 1) + "  " +
                                      (part.compare              ? "採用前と比較中"
                                       : gui::TonePending(s, ch) ? "試聴中"
                                                                 : "採用済み");
    text(42, fy + 12, exporting ? "WAVを書き出し中…" : selectedState.c_str(),
         gui::TonePending(s, ch) ? pendingColor : fg, GetFonts().fontSmall);
    clipped(42, fy + 37, width - 650, ("採用前：" + toneName(part.adopted.instrument)).c_str(), muted,
            GetFonts().fontSmall);
    ImGui::BeginDisabled(exporting || s.toneAuditionActive || s.transportAction == gui::TransportAction::Audition);
    const bool drum = std::holds_alternative<DrumKitConfig>(audible.sound.source);
    if (button(s.toneAuditionActive ? "試聴中" : drum ? "ビートを試聴" : "一音鳴らす", width - 575, fy + 15, 155, 42))
        gui::RequestToneAudition(s);
    if (button("", width - 412, fy + 15, 34, 42, false, false, DownIcon))
        ImGui::OpenPopup("preview_settings");
    ImGui::EndDisabled();
    if (ImGui::BeginPopup("preview_settings"))
    {
        if (drum)
            ImGui::TextUnformatted("キック・スネア・ハイハット\n2小節 / 120 BPM");
        else
        {
            bool automatic = part.auditionNote < 0;
            if (ImGui::Checkbox("曲に合う高さを自動選択", &automatic))
                part.auditionNote = automatic ? -1 : gui::ChooseAuditionNote(s, ch);
            int pitch = gui::ChooseAuditionNote(s, ch);
            ImGui::BeginDisabled(automatic);
            ImGui::SetNextItemWidth(260);
            if (ImGui::SliderInt("高さ", &pitch, 0, 127, pitchName(pitch).c_str()))
                part.auditionNote = pitch;
            ImGui::EndDisabled();
            ImGui::SetNextItemWidth(260);
            ImGui::SliderFloat("長さ", &s.auditionLengthSec, .2f, 3.f, "%.1f 秒");
        }
        ImGui::EndPopup();
    }
    ImGui::BeginDisabled(!gui::TonePending(s, ch) || exporting);
    if (button("取り消す", width - 364, fy + 15, 126, 42))
        gui::CancelTone(s, ch);
    ImGui::BeginDisabled(part.compare);
    if (button("このchに採用", width - 222, fy + 15, 198, 42, false, true))
        gui::AdoptTone(s, ch);
    ImGui::EndDisabled();
    ImGui::EndDisabled();
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
