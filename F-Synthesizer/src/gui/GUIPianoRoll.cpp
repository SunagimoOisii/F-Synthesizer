#include "gui/GUIPianoRoll.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include <imgui.h>

#include "MIDIParser.h"
#include "io/PlatformPaths.h"

namespace gui
{
namespace
{
struct DrawNoteInfo
{
    int index = -1;
    float x0 = 0.0f;
    float y0 = 0.0f;
    float x1 = 0.0f;
    float y1 = 0.0f;
};

struct NoteStart
{
    int tick = 0;
    int velocity = 100;
};

void SyncProjectDataFromCurrentNotes(PianoRollState& state);

int ClampChannel(int channel)
{
    return (channel >= 0 && channel < 16) ? channel : 0;
}

int ClampNote(int note)
{
    return std::clamp(note, 0, 127);
}

bool IsBlackKey(int note)
{
    switch (note % 12)
    {
    case 1:
    case 3:
    case 6:
    case 8:
    case 10:
        return true;
    default:
        return false;
    }
}

int SnapStepTicks(int snapIndex, int tpq)
{
    if (tpq <= 0)
    {
        tpq = 480;
    }
    switch (snapIndex)
    {
    case 0: return 1; // OFF は tick 単位で自由編集
    case 1: return (std::max)(1, tpq);
    case 2: return (std::max)(1, tpq / 2);
    case 3: return (std::max)(1, tpq / 4);
    case 4: return (std::max)(1, tpq / 8);
    default: return (std::max)(1, tpq / 4);
    }
}

int SnapTick(int tick, int step)
{
    if (step <= 1)
    {
        return (std::max)(0, tick);
    }
    const int q = (tick >= 0) ? ((tick + step / 2) / step) : 0;
    return (std::max)(0, q * step);
}

void ResetInteractionState(PianoRollState& state)
{
    state.isRangeSelecting = false;
    state.isDraggingMove = false;
    state.isDraggingResize = false;
    state.dragTargetIndex = -1;
    state.dragSnapshot.clear();
}

void ClearSelection(PianoRollState& state)
{
    state.selected.assign(state.notes.size(), 0);
    state.primarySelectedIndex = -1;
}

void EnsureSelectionSize(PianoRollState& state)
{
    if (state.selected.size() != state.notes.size())
    {
        state.selected.assign(state.notes.size(), 0);
        state.primarySelectedIndex = -1;
    }
}

void SelectSingle(PianoRollState& state, int index)
{
    EnsureSelectionSize(state);
    std::fill(state.selected.begin(), state.selected.end(), static_cast<uint8_t>(0));
    if (index >= 0 && index < static_cast<int>(state.selected.size()))
    {
        state.selected[static_cast<size_t>(index)] = 1;
        state.primarySelectedIndex = index;
    }
    else
    {
        state.primarySelectedIndex = -1;
    }
}

bool AnySelected(const PianoRollState& state)
{
    for (uint8_t flag : state.selected)
    {
        if (flag != 0)
        {
            return true;
        }
    }
    return false;
}

void RecomputeMaxTick(PianoRollState& state)
{
    int maxTick = 0;
    for (const auto& n : state.notes)
    {
        maxTick = (std::max)(maxTick, n.endTick);
    }
    state.maxTick = maxTick;
    if (state.tickOffset > state.maxTick)
    {
        state.tickOffset = state.maxTick;
    }
}

bool NotesEqual(const std::vector<PianoRollNote>& a, const std::vector<PianoRollNote>& b)
{
    if (a.size() != b.size())
    {
        return false;
    }
    for (size_t i = 0; i < a.size(); i++)
    {
        const auto& x = a[i];
        const auto& y = b[i];
        if (x.startTick != y.startTick || x.endTick != y.endTick ||
            x.note != y.note || x.channel != y.channel || x.velocity != y.velocity)
        {
            return false;
        }
    }
    return true;
}

void PushUndoCommand(PianoRollState& state, const std::vector<PianoRollNote>& before, const std::vector<PianoRollNote>& after)
{
    if (NotesEqual(before, after))
    {
        return;
    }

    PianoRollEditCommand cmd{};
    cmd.before = before;
    cmd.after = after;
    state.undoStack.push_back(std::move(cmd));
    if (static_cast<int>(state.undoStack.size()) > state.maxUndoCommands)
    {
        state.undoStack.erase(state.undoStack.begin());
    }
    state.redoStack.clear();
}

bool ExecuteUndo(PianoRollState& state)
{
    if (state.undoStack.empty())
    {
        return false;
    }
    PianoRollEditCommand cmd = std::move(state.undoStack.back());
    state.undoStack.pop_back();
    state.notes = cmd.before;
    RecomputeMaxTick(state);
    SyncProjectDataFromCurrentNotes(state);
    state.redoStack.push_back(std::move(cmd));
    if (state.selected.size() != state.notes.size())
    {
        state.selected.assign(state.notes.size(), 0);
        state.primarySelectedIndex = -1;
    }
    return true;
}

bool ExecuteRedo(PianoRollState& state)
{
    if (state.redoStack.empty())
    {
        return false;
    }
    PianoRollEditCommand cmd = std::move(state.redoStack.back());
    state.redoStack.pop_back();
    state.notes = cmd.after;
    RecomputeMaxTick(state);
    SyncProjectDataFromCurrentNotes(state);
    state.undoStack.push_back(std::move(cmd));
    if (state.selected.size() != state.notes.size())
    {
        state.selected.assign(state.notes.size(), 0);
        state.primarySelectedIndex = -1;
    }
    return true;
}

void ApplyPendingSelection(PianoRollState& state)
{
    if (state.pendingSelectedIndices.empty())
    {
        return;
    }
    EnsureSelectionSize(state);
    std::fill(state.selected.begin(), state.selected.end(), static_cast<uint8_t>(0));
    int first = -1;
    for (int idx : state.pendingSelectedIndices)
    {
        if (idx < 0 || idx >= static_cast<int>(state.selected.size()))
        {
            continue;
        }
        state.selected[static_cast<size_t>(idx)] = 1;
        if (first < 0)
        {
            first = idx;
        }
    }
    state.primarySelectedIndex = first;
    state.pendingSelectedIndices.clear();
}

void SyncProjectDataFromCurrentNotes(PianoRollState& state)
{
    if (state.loadedMidiPath.empty())
    {
        return;
    }
    state.projectMidiPath = state.loadedMidiPath;
    state.projectTicksPerQuarter = state.ticksPerQuarter;
    state.projectNotes = state.notes;
    state.hasProjectData = !state.projectNotes.empty();
}

int MouseToTick(float mouseX, float gridMinX, int startTick, float pxPerTick)
{
    const float local = (mouseX - gridMinX) / (std::max)(0.0001f, pxPerTick);
    return (std::max)(0, startTick + static_cast<int>(std::floor(local)));
}

int MouseToNote(float mouseY, float canvasMinY, float rowHeight, int noteHigh)
{
    const int row = static_cast<int>(std::floor((mouseY - canvasMinY) / rowHeight));
    return ClampNote(noteHigh - row);
}

bool ShouldReload(const PianoRollState& state, const std::filesystem::path& midiPath)
{
    if (midiPath != state.loadedMidiPath)
    {
        return true;
    }

    std::error_code ec;
    const auto wt = std::filesystem::last_write_time(midiPath, ec);
    if (ec)
    {
        return false;
    }
    return wt != state.loadedWriteTime;
}

void ClearModel(PianoRollState& state)
{
    state.notes.clear();
    state.maxTick = 0;
    state.ticksPerQuarter = 480;
    state.selected.clear();
    state.primarySelectedIndex = -1;
    state.undoStack.clear();
    state.redoStack.clear();
    ResetInteractionState(state);
}

void BuildNotesFromTicks(const std::vector<MIDIEventTick>& ticks, int ticksPerQuarter, PianoRollState& state)
{
    ClearModel(state);
    state.ticksPerQuarter = (ticksPerQuarter > 0) ? ticksPerQuarter : 480;

    std::vector<MIDIEventTick> sorted = ticks;
    std::sort(sorted.begin(), sorted.end(), [](const MIDIEventTick& a, const MIDIEventTick& b) {
        if (a.tick != b.tick) return a.tick < b.tick;
        return a.order < b.order;
    });

    constexpr size_t kSlots = 16 * 128;
    std::vector<std::vector<NoteStart>> noteOnQueues(kSlots);
    std::array<size_t, kSlots> queueHeads{};

    state.notes.reserve(sorted.size() / 2);

    for (const auto& e : sorted)
    {
        state.maxTick = (std::max)(state.maxTick, e.tick);
        if (e.type != MIDIEventType::Note)
        {
            continue;
        }

        const int ch = ClampChannel(e.channel);
        const int note = ClampNote(e.noteNumber);
        const size_t slot = static_cast<size_t>(ch * 128 + note);

        if (e.isNoteOn)
        {
            NoteStart st{};
            st.tick = e.tick;
            st.velocity = std::clamp(e.velocity, 1, 127);
            noteOnQueues[slot].push_back(st);
            continue;
        }

        auto& q = noteOnQueues[slot];
        size_t& head = queueHeads[slot];
        if (head < q.size())
        {
            PianoRollNote n{};
            const NoteStart st = q[head++];
            n.startTick = st.tick;
            n.endTick = (std::max)(n.startTick + 1, e.tick);
            n.note = note;
            n.channel = ch;
            n.velocity = st.velocity;
            state.notes.push_back(n);
        }
    }

    const int fallbackEndTick = state.maxTick + (std::max)(state.ticksPerQuarter, 1);
    for (int ch = 0; ch < 16; ch++)
    {
        for (int note = 0; note < 128; note++)
        {
            const size_t slot = static_cast<size_t>(ch * 128 + note);
            auto& q = noteOnQueues[slot];
            size_t& head = queueHeads[slot];
            while (head < q.size())
            {
                PianoRollNote n{};
                const NoteStart st = q[head++];
                n.startTick = st.tick;
                n.endTick = (std::max)(n.startTick + 1, fallbackEndTick);
                n.note = note;
                n.channel = ch;
                n.velocity = st.velocity;
                state.notes.push_back(n);
            }
        }
    }

    state.selected.assign(state.notes.size(), 0);
    state.primarySelectedIndex = -1;
}

void EnsureModelLoaded(
    PianoRollState& state,
    const std::filesystem::path& midiPath,
    const std::function<void(const std::string&)>& appendLog)
{
    if (midiPath.empty())
    {
        state.hasLoadError = true;
        state.lastError = "MIDI path is empty.";
        ClearModel(state);
        return;
    }

    std::error_code ec;
    if (!std::filesystem::exists(midiPath, ec) || ec)
    {
        state.hasLoadError = true;
        state.lastError = "MIDI file not found: " + PathToUtf8(midiPath);
        ClearModel(state);
        return;
    }

    if (!ShouldReload(state, midiPath))
    {
        return;
    }

    std::vector<MIDIEventTick> ticks;
    std::vector<TempoEvent> tempoEvents;
    int ticksPerQuarter = 0;
    MIDIParseStatus stats{};
    if (!LoadMIDIBasic(midiPath, -1, ticks, tempoEvents, ticksPerQuarter, stats))
    {
        state.hasLoadError = true;
        state.lastError = "failed to parse MIDI: " + PathToUtf8(midiPath);
        ClearModel(state);
        return;
    }

    BuildNotesFromTicks(ticks, ticksPerQuarter, state);
    bool appliedProjectData = false;
    if (state.hasProjectData && state.projectMidiPath == midiPath && !state.projectNotes.empty())
    {
        // 専用project JSONがある場合は、MIDI由来ノートより編集済みノートを優先する。
        state.notes = state.projectNotes;
        state.ticksPerQuarter = (state.projectTicksPerQuarter > 0) ? state.projectTicksPerQuarter : state.ticksPerQuarter;
        state.selected.assign(state.notes.size(), 0);
        state.primarySelectedIndex = -1;
        RecomputeMaxTick(state);
        appliedProjectData = true;
    }
    if (!appliedProjectData)
    {
        state.hasProjectData = false;
        state.projectMidiPath.clear();
        state.projectNotes.clear();
        state.projectTicksPerQuarter = 0;
    }
    ApplyPendingSelection(state);
    state.loadedMidiPath = midiPath;
    state.loadedWriteTime = std::filesystem::last_write_time(midiPath, ec);
    state.hasLoadError = false;
    state.lastError.clear();

    if (appendLog)
    {
        appendLog("[PianoRoll] loaded: notes=" + std::to_string(state.notes.size()) + ", tpq=" +
            std::to_string(state.ticksPerQuarter));
    }
}

void DrawPianoGrid(
    const PianoRollState& state,
    ImDrawList* drawList,
    const ImVec2& canvasMin,
    const ImVec2& canvasMax,
    float pianoWidth,
    float rowHeight,
    float pxPerTick)
{
    const int visibleCount = state.visibleNoteCount;
    const int noteLow = state.noteOffset;
    const int noteHigh = (std::min)(127, noteLow + visibleCount - 1);
    const int startTick = (std::max)(0, state.tickOffset);
    const int endTick = startTick + static_cast<int>((canvasMax.x - canvasMin.x - pianoWidth) / pxPerTick) + 1;
    const int tpq = (std::max)(1, state.ticksPerQuarter);
    const int snapStep = SnapStepTicks(state.snapIndex, tpq);

    const float gridMinX = canvasMin.x + pianoWidth;
    const ImU32 laneDark = IM_COL32(30, 30, 34, 255);
    const ImU32 laneLight = IM_COL32(36, 36, 40, 255);
    const ImU32 laneC = IM_COL32(42, 46, 56, 255);
    const ImU32 keyDark = IM_COL32(24, 24, 28, 255);
    const ImU32 keyLight = IM_COL32(44, 44, 48, 255);

    for (int row = 0; row < visibleCount; row++)
    {
        const int note = noteHigh - row;
        const float y0 = canvasMin.y + row * rowHeight;
        const float y1 = y0 + rowHeight;
        ImU32 laneColor = (row % 2 == 0) ? laneDark : laneLight;
        if ((note % 12) == 0)
        {
            laneColor = laneC;
        }
        drawList->AddRectFilled(ImVec2(gridMinX, y0), ImVec2(canvasMax.x, y1), laneColor);
        drawList->AddRectFilled(
            ImVec2(canvasMin.x, y0),
            ImVec2(gridMinX, y1),
            IsBlackKey(note) ? keyDark : keyLight);
        drawList->AddText(ImVec2(canvasMin.x + 4.0f, y0 + 1.0f), IM_COL32(210, 210, 215, 255), std::to_string(note).c_str());
    }

    const int firstSnapTick = (startTick / snapStep) * snapStep;
    for (int tick = firstSnapTick; tick <= endTick; tick += snapStep)
    {
        const float x = gridMinX + (tick - startTick) * pxPerTick;
        const bool beatLine = (tick % tpq) == 0;
        const ImU32 col = beatLine ? IM_COL32(120, 130, 150, 180) : IM_COL32(90, 95, 110, 90);
        drawList->AddLine(ImVec2(x, canvasMin.y), ImVec2(x, canvasMax.y), col);
    }

    drawList->AddLine(ImVec2(gridMinX, canvasMin.y), ImVec2(gridMinX, canvasMax.y), IM_COL32(180, 180, 190, 180), 1.0f);
}

void BuildVisibleDrawNotes(
    const PianoRollState& state,
    const ImVec2& canvasMin,
    const ImVec2& canvasMax,
    float pianoWidth,
    float rowHeight,
    float pxPerTick,
    std::vector<DrawNoteInfo>& out)
{
    out.clear();
    out.reserve(256);

    const int noteLow = state.noteOffset;
    const int noteHigh = (std::min)(127, noteLow + state.visibleNoteCount - 1);
    const int startTick = (std::max)(0, state.tickOffset);
    const int endTick = startTick + static_cast<int>((canvasMax.x - canvasMin.x - pianoWidth) / pxPerTick) + 1;
    const float gridMinX = canvasMin.x + pianoWidth;

    for (int i = 0; i < static_cast<int>(state.notes.size()); i++)
    {
        const auto& n = state.notes[static_cast<size_t>(i)];
        if (n.channel != state.displayChannel)
        {
            continue;
        }
        if (n.note < noteLow || n.note > noteHigh)
        {
            continue;
        }
        if (n.endTick < startTick || n.startTick > endTick)
        {
            continue;
        }

        const int row = noteHigh - n.note;
        const float y0 = canvasMin.y + row * rowHeight + 1.0f;
        const float y1 = y0 + rowHeight - 2.0f;
        const float x0 = gridMinX + (n.startTick - startTick) * pxPerTick;
        const float x1 = gridMinX + (n.endTick - startTick) * pxPerTick;
        const float w = (std::max)(x1 - x0, 2.0f);

        DrawNoteInfo info{};
        info.index = i;
        info.x0 = x0;
        info.y0 = y0;
        info.x1 = x0 + w;
        info.y1 = y1;
        out.push_back(info);
    }
}

bool HitTestNotes(
    const std::vector<DrawNoteInfo>& visibleNotes,
    float mouseX,
    float mouseY,
    int& outIndex,
    bool& outResizeHandle)
{
    constexpr float kResizeHandlePx = 6.0f;
    outIndex = -1;
    outResizeHandle = false;

    for (int i = static_cast<int>(visibleNotes.size()) - 1; i >= 0; i--)
    {
        const auto& dn = visibleNotes[static_cast<size_t>(i)];
        if (mouseX < dn.x0 || mouseX > dn.x1 || mouseY < dn.y0 || mouseY > dn.y1)
        {
            continue;
        }
        outIndex = dn.index;
        outResizeHandle = (dn.x1 - mouseX) <= kResizeHandlePx;
        return true;
    }
    return false;
}

void ApplyRangeSelection(
    PianoRollState& state,
    const std::vector<DrawNoteInfo>& visibleNotes,
    float x0,
    float y0,
    float x1,
    float y1)
{
    EnsureSelectionSize(state);
    std::fill(state.selected.begin(), state.selected.end(), static_cast<uint8_t>(0));

    const float rx0 = (std::min)(x0, x1);
    const float rx1 = (std::max)(x0, x1);
    const float ry0 = (std::min)(y0, y1);
    const float ry1 = (std::max)(y0, y1);

    int firstSelected = -1;
    for (const auto& dn : visibleNotes)
    {
        const bool intersects = !(dn.x1 < rx0 || dn.x0 > rx1 || dn.y1 < ry0 || dn.y0 > ry1);
        if (!intersects)
        {
            continue;
        }
        state.selected[static_cast<size_t>(dn.index)] = 1;
        if (firstSelected < 0)
        {
            firstSelected = dn.index;
        }
    }
    state.primarySelectedIndex = firstSelected;
}

void StartDragIfPossible(PianoRollState& state, int hitIndex, bool resizeMode, int mouseTick, int mouseNote)
{
    EnsureSelectionSize(state);
    if (hitIndex < 0 || hitIndex >= static_cast<int>(state.notes.size()))
    {
        return;
    }

    if (hitIndex >= static_cast<int>(state.selected.size()) || state.selected[static_cast<size_t>(hitIndex)] == 0)
    {
        SelectSingle(state, hitIndex);
    }

    state.dragSnapshot = state.notes;
    state.dragTargetIndex = hitIndex;
    state.dragStartMouseTick = mouseTick;
    state.dragStartMouseNote = mouseNote;
    state.isDraggingResize = resizeMode;
    state.isDraggingMove = !resizeMode;
}

void UpdateMoveDrag(PianoRollState& state, int currentMouseTick, int currentMouseNote, int snapStep)
{
    if (!state.isDraggingMove || state.dragSnapshot.size() != state.notes.size())
    {
        return;
    }

    const int deltaTickRaw = currentMouseTick - state.dragStartMouseTick;
    const int deltaNote = currentMouseNote - state.dragStartMouseNote;

    for (size_t i = 0; i < state.notes.size(); i++)
    {
        if (i >= state.selected.size() || state.selected[i] == 0)
        {
            continue;
        }

        const auto& base = state.dragSnapshot[i];
        auto& dst = state.notes[i];
        const int baseLen = (std::max)(1, base.endTick - base.startTick);
        int newStart = base.startTick + deltaTickRaw;
        newStart = SnapTick(newStart, snapStep);
        dst.startTick = (std::max)(0, newStart);
        dst.endTick = dst.startTick + baseLen;
        dst.note = ClampNote(base.note + deltaNote);
        dst.channel = base.channel;
        dst.velocity = base.velocity;
    }
    RecomputeMaxTick(state);
}

void UpdateResizeDrag(PianoRollState& state, int currentMouseTick, int snapStep)
{
    if (!state.isDraggingResize || state.dragSnapshot.size() != state.notes.size())
    {
        return;
    }
    const int idx = state.dragTargetIndex;
    if (idx < 0 || idx >= static_cast<int>(state.notes.size()))
    {
        return;
    }

    const auto& base = state.dragSnapshot[static_cast<size_t>(idx)];
    auto& dst = state.notes[static_cast<size_t>(idx)];
    int newEnd = SnapTick(currentMouseTick, snapStep);
    newEnd = (std::max)(newEnd, base.startTick + 1);
    dst.endTick = newEnd;
    RecomputeMaxTick(state);
}

void DrawNotes(
    const PianoRollState& state,
    ImDrawList* drawList,
    const std::vector<DrawNoteInfo>& visibleNotes)
{
    const ImU32 noteColor = IM_COL32(120, 200, 255, 210);
    const ImU32 noteBorder = IM_COL32(50, 120, 170, 255);
    const ImU32 selectedColor = IM_COL32(255, 206, 120, 235);
    const ImU32 selectedBorder = IM_COL32(235, 150, 32, 255);

    for (const auto& dn : visibleNotes)
    {
        const bool isSelected = dn.index >= 0 &&
            dn.index < static_cast<int>(state.selected.size()) &&
            state.selected[static_cast<size_t>(dn.index)] != 0;
        drawList->AddRectFilled(
            ImVec2(dn.x0, dn.y0),
            ImVec2(dn.x1, dn.y1),
            isSelected ? selectedColor : noteColor,
            2.0f);
        drawList->AddRect(
            ImVec2(dn.x0, dn.y0),
            ImVec2(dn.x1, dn.y1),
            isSelected ? selectedBorder : noteBorder,
            2.0f);
    }
}
} // namespace

void DrawPianoRollPanel(
    PianoRollState& state,
    const char* midiPathUtf8,
    const std::function<void(const std::string&)>& appendLog)
{
    const std::filesystem::path midiPath = (midiPathUtf8 != nullptr) ? Utf8ToPath(midiPathUtf8) : std::filesystem::path{};
    EnsureModelLoaded(state, midiPath, appendLog);
    EnsureSelectionSize(state);

    ImGui::TextUnformatted("Piano Roll (Phase 2: Selection/Edit)");
    ImGui::BeginDisabled();
    ImGui::Checkbox("Drum Name Mode (next phase)", &state.drumNameMode);
    ImGui::EndDisabled();

    ImGui::SetNextItemWidth(140.0f);
    ImGui::SliderInt("PR Channel", &state.displayChannel, 0, 15);
    ImGui::SameLine();
    ImGui::SetNextItemWidth(180.0f);
    const char* snapItems[] = { "OFF", "1/4", "1/8", "1/16", "1/32" };
    ImGui::Combo("PR Snap", &state.snapIndex, snapItems, IM_ARRAYSIZE(snapItems));
    ImGui::SetNextItemWidth(260.0f);
    ImGui::SliderFloat("PR Zoom", &state.pixelsPerQuarter, 16.0f, 240.0f, "%.0f px/qn");
    ImGui::SetNextItemWidth(220.0f);
    ImGui::DragInt("PR Tick Offset", &state.tickOffset, 8.0f, 0, (std::max)(state.maxTick, 0));
    ImGui::SetNextItemWidth(220.0f);
    ImGui::SliderInt("PR Note Offset", &state.noteOffset, 0, 116);
    ImGui::SetNextItemWidth(220.0f);
    ImGui::SliderInt("PR Visible Notes", &state.visibleNoteCount, 12, 72);

    if (state.hasLoadError)
    {
        ImGui::TextColored(ImVec4(0.95f, 0.35f, 0.35f, 1.0f), "PianoRoll load failed: %s", state.lastError.c_str());
    }
    else
    {
        const bool hasSelection = AnySelected(state);
        ImGui::Text("PianoRoll notes=%zu, selected=%s, tpq=%d, maxTick=%d",
            state.notes.size(),
            hasSelection ? "yes" : "no",
            state.ticksPerQuarter,
            state.maxTick);
        ImGui::SameLine();
        ImGui::BeginDisabled(state.undoStack.empty());
        if (ImGui::SmallButton("Undo"))
        {
            ExecuteUndo(state);
        }
        ImGui::EndDisabled();
        ImGui::SameLine();
        ImGui::BeginDisabled(state.redoStack.empty());
        if (ImGui::SmallButton("Redo"))
        {
            ExecuteRedo(state);
        }
        ImGui::EndDisabled();
    }

    const float rowHeight = 12.0f;
    const float pianoWidth = 56.0f;
    const float panelHeight = std::clamp(state.visibleNoteCount * rowHeight + 4.0f, 180.0f, 460.0f);
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
    DrawPianoGrid(state, drawList, canvasMin, canvasMax, pianoWidth, rowHeight, pxPerTick);

    if (state.hasLoadError || state.notes.empty())
    {
        ResetInteractionState(state);
        return;
    }

    std::vector<DrawNoteInfo> visibleNotes;
    BuildVisibleDrawNotes(state, canvasMin, canvasMax, pianoWidth, rowHeight, pxPerTick, visibleNotes);

    const bool hovered = ImGui::IsItemHovered();
    const bool mouseInGrid = hovered && ImGui::GetIO().MousePos.x >= gridMinX;
    const bool panelFocused = hovered || ImGui::IsItemActive();
    const ImVec2 mousePos = ImGui::GetIO().MousePos;
    const int noteHigh = (std::min)(127, state.noteOffset + state.visibleNoteCount - 1);
    const int mouseTick = MouseToTick(mousePos.x, gridMinX, (std::max)(0, state.tickOffset), pxPerTick);
    const int mouseNote = MouseToNote(mousePos.y, canvasMin.y, rowHeight, noteHigh);

    if (panelFocused && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false))
    {
        ExecuteUndo(state);
        BuildVisibleDrawNotes(state, canvasMin, canvasMax, pianoWidth, rowHeight, pxPerTick, visibleNotes);
    }
    if (panelFocused && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false))
    {
        ExecuteRedo(state);
        BuildVisibleDrawNotes(state, canvasMin, canvasMax, pianoWidth, rowHeight, pxPerTick, visibleNotes);
    }

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && mouseInGrid)
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
            ClearSelection(state);
            state.isRangeSelecting = true;
            state.rangeStartX = mousePos.x;
            state.rangeStartY = mousePos.y;
            state.rangeEndX = mousePos.x;
            state.rangeEndY = mousePos.y;
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
            BuildVisibleDrawNotes(state, canvasMin, canvasMax, pianoWidth, rowHeight, pxPerTick, visibleNotes);
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            PushUndoCommand(state, state.dragSnapshot, state.notes);
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
            BuildVisibleDrawNotes(state, canvasMin, canvasMax, pianoWidth, rowHeight, pxPerTick, visibleNotes);
        }
        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
        {
            PushUndoCommand(state, state.dragSnapshot, state.notes);
            SyncProjectDataFromCurrentNotes(state);
            state.isDraggingResize = false;
            state.dragSnapshot.clear();
            state.dragTargetIndex = -1;
        }
    }

    drawList->PushClipRect(ImVec2(canvasMin.x + pianoWidth, canvasMin.y), canvasMax, true);
    DrawNotes(state, drawList, visibleNotes);
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
} // namespace gui
