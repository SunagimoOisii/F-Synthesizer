#include "gui/GUIPianoRoll.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include <imgui.h>

#include "midi/MIDIParser.h"
#include "io/PlatformPaths.h"

namespace gui
{
namespace
{
#include "pianoroll/PianoRollTempo.inl"
#include "pianoroll/PianoRollEdit.inl"
#include "pianoroll/PianoRollRender.inl"
#include "pianoroll/PianoRollInput.inl"
} // namespace
void DrawPianoRollPanel(
    PianoRollState& state,
    const char* midiPathUtf8,
    const PreviewPlaybackState* playback,
    const std::function<void(const std::string&)>& appendLog,
    const std::function<void()>& requestPreviewPlay,
    const std::function<void()>& requestPreviewStop)
{
    const std::filesystem::path midiPath = (midiPathUtf8 != nullptr) ? Utf8ToPath(midiPathUtf8) : std::filesystem::path{};
    EnsureModelLoaded(state, midiPath, appendLog);
    EnsureSelectionSize(state);

    ImGui::TextUnformatted("ピアノロール");
    ImGui::SetNextItemWidth(140.0f);
    int displayChannelOneBased = ClampChannel(state.displayChannel) + 1;
    if (ImGui::SliderInt("表示チャンネル", &displayChannelOneBased, 1, 16, "ch%d"))
    {
        state.displayChannel = ClampChannel(displayChannelOneBased - 1);
    }
    ImGui::SameLine();
    ImGui::Text("スナップ: %s", SnapLabel(state.snapIndex));
    ImGui::SameLine();
    if (ImGui::SmallButton("OFF"))
    {
        SetSnapIndex(state, 0, appendLog);
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("1/4"))
    {
        SetSnapIndex(state, 1, appendLog);
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("1/8"))
    {
        SetSnapIndex(state, 2, appendLog);
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("1/16"))
    {
        SetSnapIndex(state, 3, appendLog);
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("1/32"))
    {
        SetSnapIndex(state, 4, appendLog);
    }
    ImGui::SameLine();
    ImGui::Checkbox("再生位置に追従", &state.followPreviewPlayback);
    ImGui::SameLine();
    if (state.previewRangeEnabled)
    {
        ImGui::Text("Preview範囲=%d-%d", state.previewRangeStartTick, state.previewRangeEndTick);
    }
    else
    {
        ImGui::TextUnformatted("Preview対象=全体");
    }
    ImGui::TextDisabled("操作: 左ドラッグ=移動 / 端ドラッグ=長さ / 空白ドラッグ=追加 / Shift+空白=任意範囲Preview / Delete=削除 / ルーラD&D=Preview範囲 / Space=再生停止 / Q,1-4=スナップ");

    if (state.hasLoadError)
    {
        ImGui::TextColored(ImVec4(0.95f, 0.35f, 0.35f, 1.0f), "ピアノロールの読み込みに失敗: %s", state.lastError.c_str());
    }
    else
    {
        const bool hasSelection = AnySelected(state);
        ImGui::Text("ノート数=%zu, 選択=%s, 四分音符あたりTick=%d, 最大Tick=%d",
            state.notes.size(),
            hasSelection ? "あり" : "なし",
            state.ticksPerQuarter,
            state.maxTick);
        ImGui::SameLine();
        ImGui::BeginDisabled(state.undoStack.empty());
        if (ImGui::SmallButton("元に戻す"))
        {
            ExecuteUndo(state);
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled(state.redoStack.empty());
        if (ImGui::SmallButton("やり直し"))
        {
            ExecuteRedo(state);
        }
        ImGui::EndDisabled();
    }

    const float rowHeight = (std::max)(12.0f, ImGui::GetTextLineHeight() + 4.0f);
    const float rulerHeight = (std::max)(14.0f, ImGui::GetTextLineHeight() + 2.0f);
    const float pianoWidth = (std::max)(56.0f, ImGui::CalcTextSize("127").x + 12.0f);
    const float panelHeight = (std::max)(220.0f, ImGui::GetContentRegionAvail().y - 2.0f);
    const int autoVisibleCount = std::clamp(
        static_cast<int>(std::floor((panelHeight - rulerHeight - 2.0f) / rowHeight)),
        12,
        128);
    state.visibleNoteCount = autoVisibleCount;
    state.noteOffset = std::clamp(state.noteOffset, 0, MaxNoteOffset(state.visibleNoteCount));
    const ImVec2 canvasSize((std::max)(120.0f, ImGui::GetContentRegionAvail().x - 4.0f), panelHeight);
    ImGui::InvisibleButton("piano_roll_canvas", canvasSize);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 canvasMin = ImGui::GetItemRectMin();
    const ImVec2 canvasMax = ImGui::GetItemRectMax();
    const float gridMinX = canvasMin.x + pianoWidth;
    drawList->AddRectFilled(canvasMin, canvasMax, IM_COL32(20, 20, 24, 255));
    drawList->AddRect(canvasMin, canvasMax, IM_COL32(90, 90, 100, 255));

    const float tpq = static_cast<float>((std::max)(1, state.ticksPerQuarter));
    const float pxPerTick = (std::max)(0.01f, state.pixelsPerQuarter / tpq);
    const int snapStep = SnapStepTicks(state.snapIndex, state.ticksPerQuarter);
    DrawPianoGrid(state, drawList, canvasMin, canvasMax, pianoWidth, rulerHeight, rowHeight, pxPerTick);

    if (state.hasLoadError || state.notes.empty())
    {
        ResetInteractionState(state);
        return;
    }

    std::vector<DrawNoteInfo> visibleNotes;
    BuildVisibleDrawNotes(state, canvasMin, canvasMax, pianoWidth, rulerHeight, rowHeight, pxPerTick, visibleNotes);

    const bool hovered = ImGui::IsItemHovered();
    const ImVec2 mousePos = ImGui::GetIO().MousePos;
    const bool mouseInGrid = hovered && mousePos.x >= gridMinX;
    const float noteAreaMinY = canvasMin.y + rulerHeight;
    const bool mouseInRuler = mouseInGrid && mousePos.y >= canvasMin.y && mousePos.y < noteAreaMinY;
    const bool mouseInNoteArea = mouseInGrid && mousePos.y >= noteAreaMinY;
    const bool panelFocused = hovered || ImGui::IsItemActive();
    const int noteHigh = (std::min)(127, state.noteOffset + state.visibleNoteCount - 1);
    const int mouseTick = MouseToTick(mousePos.x, gridMinX, (std::max)(0, state.tickOffset), pxPerTick);
    const int mouseNote = MouseToNote(mousePos.y, noteAreaMinY, rowHeight, noteHigh);

    if (state.previewRangeEnabled)
    {
        NormalizePreviewRange(state);
        const int drawStartTick = (std::max)(0, state.tickOffset);
        const float rangeX0 = gridMinX + static_cast<float>(state.previewRangeStartTick - drawStartTick) * pxPerTick;
        const float rangeX1 = gridMinX + static_cast<float>(state.previewRangeEndTick - drawStartTick) * pxPerTick;
        const float x0 = (std::max)(gridMinX, (std::min)(rangeX0, rangeX1));
        const float x1 = (std::min)(canvasMax.x, (std::max)(rangeX0, rangeX1));
        if (x1 > x0)
        {
            drawList->AddRectFilled(
                ImVec2(x0, canvasMin.y),
                ImVec2(x1, canvasMax.y),
                IM_COL32(90, 140, 210, 36));
            drawList->AddRect(
                ImVec2(x0, canvasMin.y),
                ImVec2(x1, canvasMax.y),
                IM_COL32(90, 140, 210, 160));
        }
    }

    if (hovered)
    {
        ImGuiIO& io = ImGui::GetIO();
        if (std::fabs(io.MouseWheel) > 0.0001f)
        {
            const float wheel = io.MouseWheel;
            const int wheelSteps = (wheel > 0.0f) ? 1 : -1;
            if (io.KeyCtrl)
            {
                // Ctrl+ホイール: マウス位置を中心に時間ズーム。
                const int anchorTick = MouseToTick(mousePos.x, gridMinX, (std::max)(0, state.tickOffset), pxPerTick);
                const float factor = std::pow(1.10f, wheel);
                state.pixelsPerQuarter = std::clamp(state.pixelsPerQuarter * factor, 16.0f, 240.0f);
                const float newPxPerTick = (std::max)(0.01f, state.pixelsPerQuarter / tpq);
                const int anchorScreenTick = static_cast<int>((mousePos.x - gridMinX) / newPxPerTick);
                state.tickOffset = (std::max)(0, anchorTick - anchorScreenTick);
            }
            else if (io.KeyShift)
            {
                // Shift+ホイール: 時間軸の横スクロール。
                const int tickStep = (std::max)(1, state.ticksPerQuarter * 2);
                state.tickOffset = (std::max)(0, state.tickOffset - wheelSteps * tickStep);
            }
            else
            {
                // 通常ホイール: 音高軸の縦スクロール。
                const int noteStep = (std::max)(1, state.visibleNoteCount / 4);
                state.noteOffset = std::clamp(
                    state.noteOffset + wheelSteps * noteStep,
                    0,
                    MaxNoteOffset(state.visibleNoteCount));
            }
            InvalidateVisibleCache(state);
        }

        if (std::fabs(io.MouseWheelH) > 0.0001f && !io.KeyCtrl)
        {
            const int wheelHSteps = (io.MouseWheelH > 0.0f) ? 1 : -1;
            const int tickStep = (std::max)(1, state.ticksPerQuarter * 2);
            state.tickOffset = (std::max)(0, state.tickOffset - wheelHSteps * tickStep);
            InvalidateVisibleCache(state);
        }
    }

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && mouseInRuler)
    {
        state.isPreviewRangeDragging = true;
        state.previewRangeDragStartTick = SnapTick(mouseTick, snapStep);
        state.previewRangeStartTick = state.previewRangeDragStartTick;
        state.previewRangeEndTick = state.previewRangeDragStartTick;
        state.previewRangeEnabled = true;
    }
    if (state.isPreviewRangeDragging)
    {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            state.previewRangeEndTick = SnapTick(mouseTick, snapStep);
            NormalizePreviewRange(state);
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            state.isPreviewRangeDragging = false;
            if (state.previewRangeStartTick == state.previewRangeEndTick)
            {
                state.previewRangeEnabled = false;
                state.previewStartTick = state.previewRangeStartTick;
                if (appendLog)
                {
                    appendLog("[PianoRoll] preview range cleared: full range");
                }
            }
            else
            {
                NormalizePreviewRange(state);
                if (appendLog)
                {
                    appendLog("[PianoRoll] preview range set: " +
                        std::to_string(state.previewRangeStartTick) + "-" +
                        std::to_string(state.previewRangeEndTick));
                }
            }
        }
    }

    if (panelFocused && !ImGui::GetIO().WantTextInput && ImGui::IsKeyPressed(ImGuiKey_Space, false))
    {
        const bool playing = (playback != nullptr) && playback->playing.load(std::memory_order_relaxed);
        if (playing)
        {
            if (requestPreviewStop)
            {
                requestPreviewStop();
            }
        }
        else
        {
            if (requestPreviewPlay)
            {
                requestPreviewPlay();
            }
        }
    }
    if (panelFocused && !ImGui::GetIO().WantTextInput)
    {
        if (ImGui::IsKeyPressed(ImGuiKey_Delete, false) || ImGui::IsKeyPressed(ImGuiKey_Backspace, false))
        {
            if (DeleteSelectedNotes(state))
            {
                BuildVisibleDrawNotes(state, canvasMin, canvasMax, pianoWidth, rulerHeight, rowHeight, pxPerTick, visibleNotes);
                if (appendLog)
                {
                    appendLog("[PianoRoll] selected notes deleted");
                }
            }
        }
        if (ImGui::IsKeyPressed(ImGuiKey_Q, false))
        {
            const int restore = (state.lastSnapIndex >= 1 && state.lastSnapIndex <= 4) ? state.lastSnapIndex : 3;
            const int next = (state.snapIndex == 0) ? restore : 0;
            SetSnapIndex(state, next, appendLog);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_1, false))
        {
            SetSnapIndex(state, 1, appendLog);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_2, false))
        {
            SetSnapIndex(state, 2, appendLog);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_3, false))
        {
            SetSnapIndex(state, 3, appendLog);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_4, false))
        {
            SetSnapIndex(state, 4, appendLog);
        }
    }

    if (panelFocused && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false))
    {
        ExecuteUndo(state);
        BuildVisibleDrawNotes(state, canvasMin, canvasMax, pianoWidth, rulerHeight, rowHeight, pxPerTick, visibleNotes);
    }
    if (panelFocused && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false))
    {
        ExecuteRedo(state);
        BuildVisibleDrawNotes(state, canvasMin, canvasMax, pianoWidth, rulerHeight, rowHeight, pxPerTick, visibleNotes);
    }

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && mouseInNoteArea)
    {
        int hitIndex = -1;
        bool onResize = false;
        const bool hit = HitTestNotes(visibleNotes, mousePos.x, mousePos.y, hitIndex, onResize);
        if (hit)
        {
            StartDragIfPossible(state, hitIndex, onResize, mouseTick, mouseNote);
        }
        else
        {
            if (ImGui::GetIO().KeyShift)
            {
                ClearSelection(state);
                state.isRangeSelecting = true;
                state.rangeStartX = mousePos.x;
                state.rangeStartY = mousePos.y;
                state.rangeEndX = mousePos.x;
                state.rangeEndY = mousePos.y;
            }
            else
            {
                // 空白ドラッグでノート作成。Shift時のみ範囲選択へ切替える。
                ClearSelection(state);
                state.isCreatingNote = true;
                state.createStartTick = SnapTick(mouseTick, snapStep);
                state.createCurrentTick = state.createStartTick + (std::max)(1, snapStep);
                state.createNote = ClampNote(mouseNote);
            }
        }
    }

    if (state.isRangeSelecting)
    {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            state.rangeEndX = mousePos.x;
            state.rangeEndY = mousePos.y;
            ApplyRangeSelection(
                state,
                visibleNotes,
                state.rangeStartX,
                state.rangeStartY,
                state.rangeEndX,
                state.rangeEndY);
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            state.isRangeSelecting = false;
        }
    }

    if (state.isDraggingMove)
    {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            UpdateMoveDrag(state, mouseTick, mouseNote, snapStep);
            BuildVisibleDrawNotes(state, canvasMin, canvasMax, pianoWidth, rulerHeight, rowHeight, pxPerTick, visibleNotes);
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            PushUndoCommand(state, state.dragSnapshot, state.notes);
            TouchNotesVersion(state);
            SyncProjectDataFromCurrentNotes(state);
            state.isDraggingMove = false;
            state.dragSnapshot.clear();
            state.dragTargetIndex = -1;
        }
    }
    else if (state.isDraggingResize)
    {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            UpdateResizeDrag(state, mouseTick, snapStep);
            BuildVisibleDrawNotes(state, canvasMin, canvasMax, pianoWidth, rulerHeight, rowHeight, pxPerTick, visibleNotes);
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            PushUndoCommand(state, state.dragSnapshot, state.notes);
            TouchNotesVersion(state);
            SyncProjectDataFromCurrentNotes(state);
            state.isDraggingResize = false;
            state.dragSnapshot.clear();
            state.dragTargetIndex = -1;
        }
    }
    else if (state.isCreatingNote)
    {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
        {
            state.createCurrentTick = SnapTick(mouseTick, snapStep);
            state.createNote = ClampNote(mouseNote);
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            const std::vector<PianoRollNote> before = state.notes;
            int startTick = (std::min)(state.createStartTick, state.createCurrentTick);
            int endTick = (std::max)(state.createStartTick, state.createCurrentTick);
            if (endTick <= startTick)
            {
                endTick = startTick + (std::max)(1, snapStep);
            }

            PianoRollNote created{};
            created.startTick = (std::max)(0, startTick);
            created.endTick = (std::max)(created.startTick + 1, endTick);
            created.note = ClampNote(state.createNote);
            created.channel = ClampChannel(state.displayChannel);
            created.velocity = 100;
            state.notes.push_back(created);

            EnsureSelectionSize(state);
            SelectSingle(state, static_cast<int>(state.notes.size()) - 1);
            PushUndoCommand(state, before, state.notes);
            TouchNotesVersion(state);
            RecomputeMaxTick(state);
            SyncProjectDataFromCurrentNotes(state);

            state.isCreatingNote = false;
        }
    }

    drawList->PushClipRect(ImVec2(canvasMin.x + pianoWidth, canvasMin.y), canvasMax, true);
    DrawNotes(state, drawList, visibleNotes);
    DrawCreatingNotePreview(state, drawList, ImVec2(canvasMin.x, noteAreaMinY), canvasMax, pianoWidth, rowHeight, pxPerTick);

    if (playback != nullptr && playback->playing.load(std::memory_order_relaxed))
    {
        const ma_uint32 sr = (playback->sampleRate > 0) ? playback->sampleRate : 44100;
        const double playbackSec = static_cast<double>(playback->frameCursor.load(std::memory_order_relaxed)) /
            static_cast<double>(sr);
        const int playStartTick = playback->playStartTick.load(std::memory_order_relaxed);
        const double absoluteSec = SecondsAtTick(state.tempoEvents, state.ticksPerQuarter, playStartTick) + playbackSec;
        const int headTick = TickAtSeconds(state.tempoEvents, state.ticksPerQuarter, absoluteSec);
        if (state.followPreviewPlayback)
        {
            const int visibleTickSpan = static_cast<int>((canvasMax.x - canvasMin.x - pianoWidth) / (std::max)(0.0001f, pxPerTick));
            const int leftBound = state.tickOffset;
            const int rightBound = state.tickOffset + visibleTickSpan;
            const int followMargin = (std::max)(1, visibleTickSpan / 4);
            if (headTick > rightBound - followMargin)
            {
                state.tickOffset = (std::max)(0, headTick - (visibleTickSpan * 3) / 4);
            }
            else if (headTick < leftBound + followMargin)
            {
                state.tickOffset = (std::max)(0, headTick - visibleTickSpan / 4);
            }
        }

        const int drawStartTick = (std::max)(0, state.tickOffset);
        const float headX = canvasMin.x + pianoWidth + static_cast<float>(headTick - drawStartTick) * pxPerTick;
        if (headX >= canvasMin.x + pianoWidth && headX <= canvasMax.x)
        {
            drawList->AddLine(ImVec2(headX, canvasMin.y), ImVec2(headX, canvasMax.y), IM_COL32(255, 80, 80, 240), 2.0f);
        }
    }
    drawList->PopClipRect();

    if (state.isRangeSelecting)
    {
        const float rx0 = (std::min)(state.rangeStartX, state.rangeEndX);
        const float rx1 = (std::max)(state.rangeStartX, state.rangeEndX);
        const float ry0 = (std::min)(state.rangeStartY, state.rangeEndY);
        const float ry1 = (std::max)(state.rangeStartY, state.rangeEndY);
        drawList->AddRectFilled(ImVec2(rx0, ry0), ImVec2(rx1, ry1), IM_COL32(120, 180, 255, 36));
        drawList->AddRect(ImVec2(rx0, ry0), ImVec2(rx1, ry1), IM_COL32(120, 180, 255, 200));
    }
}

void ApplyStepSeqNotes(PianoRollState& state, const std::vector<PianoRollNote>& ch9Notes)
{
    state.notes.erase(
        std::remove_if(state.notes.begin(), state.notes.end(),
            [](const PianoRollNote& n) { return n.channel == 9; }),
        state.notes.end());

    state.notes.insert(state.notes.end(), ch9Notes.begin(), ch9Notes.end());
    std::sort(state.notes.begin(), state.notes.end(),
        [](const PianoRollNote& a, const PianoRollNote& b) {
            if (a.startTick != b.startTick)
            {
                return a.startTick < b.startTick;
            }
            if (a.channel != b.channel)
            {
                return a.channel < b.channel;
            }
            return a.note < b.note;
        });

    RecomputeMaxTick(state);
    SyncProjectDataFromCurrentNotes(state);
    TouchNotesVersion(state);
}
} // namespace gui
