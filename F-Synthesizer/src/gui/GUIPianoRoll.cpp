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
void RecomputeMaxTick(PianoRollState& state);
void PushUndoCommand(PianoRollState& state, const std::vector<PianoRollNote>& before, const std::vector<PianoRollNote>& after);

int ClampChannel(int channel)
{
    return (channel >= 0 && channel < 16) ? channel : 0;
}

int ClampNote(int note)
{
    return std::clamp(note, 0, 127);
}

int MaxNoteOffset(int visibleCount)
{
    const int clampedVisible = std::clamp(visibleCount, 1, 128);
    return (std::max)(0, 127 - clampedVisible + 1);
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

const char* SnapLabel(int snapIndex)
{
    switch (snapIndex)
    {
    case 0: return "OFF";
    case 1: return "1/4";
    case 2: return "1/8";
    case 3: return "1/16";
    case 4: return "1/32";
    default: return "1/16";
    }
}

void SetSnapIndex(PianoRollState& state, int newIndex, const std::function<void(const std::string&)>& appendLog)
{
    const int clamped = std::clamp(newIndex, 0, 4);
    if (clamped == state.snapIndex)
    {
        return;
    }
    state.snapIndex = clamped;
    if (state.snapIndex != 0)
    {
        state.lastSnapIndex = state.snapIndex;
    }
    if (appendLog)
    {
        appendLog(std::string("[PianoRoll] snap changed: ") + SnapLabel(state.snapIndex));
    }
}

void NormalizePreviewRange(PianoRollState& state)
{
    if (!state.previewRangeEnabled)
    {
        return;
    }
    int a = (std::max)(0, state.previewRangeStartTick);
    int b = (std::max)(0, state.previewRangeEndTick);
    if (a > b)
    {
        std::swap(a, b);
    }
    state.previewRangeStartTick = a;
    state.previewRangeEndTick = b;
    state.previewStartTick = a;
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

double SecondsAtTick(const std::vector<TempoEvent>& tempoEvents, int ticksPerQuarter, int targetTick)
{
    if (targetTick <= 0 || ticksPerQuarter <= 0)
    {
        return 0.0;
    }

    std::vector<TempoEvent> sortedTempo = tempoEvents;
    std::sort(sortedTempo.begin(), sortedTempo.end(), [](const TempoEvent& a, const TempoEvent& b) {
        return a.tick < b.tick;
    });
    if (sortedTempo.empty() || sortedTempo.front().tick != 0)
    {
        TempoEvent te{};
        te.tick = 0;
        te.bpm = 120.0;
        sortedTempo.insert(sortedTempo.begin(), te);
    }

    double seconds = 0.0;
    int cursorTick = 0;
    double cursorBpm = sortedTempo.front().bpm;
    size_t tempoIndex = 1;
    while (tempoIndex < sortedTempo.size() && sortedTempo[tempoIndex].tick <= targetTick)
    {
        const int nextTick = sortedTempo[tempoIndex].tick;
        const int deltaTick = nextTick - cursorTick;
        const double secPerTick = (60.0 / cursorBpm) / static_cast<double>(ticksPerQuarter);
        seconds += secPerTick * static_cast<double>(deltaTick);
        cursorTick = nextTick;
        cursorBpm = sortedTempo[tempoIndex].bpm;
        tempoIndex++;
    }
    if (targetTick > cursorTick)
    {
        const int deltaTick = targetTick - cursorTick;
        const double secPerTick = (60.0 / cursorBpm) / static_cast<double>(ticksPerQuarter);
        seconds += secPerTick * static_cast<double>(deltaTick);
    }
    return seconds;
}

int TickAtSeconds(const std::vector<TempoEvent>& tempoEvents, int ticksPerQuarter, double targetSeconds)
{
    if (targetSeconds <= 0.0 || ticksPerQuarter <= 0)
    {
        return 0;
    }

    std::vector<TempoEvent> sortedTempo = tempoEvents;
    std::sort(sortedTempo.begin(), sortedTempo.end(), [](const TempoEvent& a, const TempoEvent& b) {
        return a.tick < b.tick;
    });
    if (sortedTempo.empty() || sortedTempo.front().tick != 0)
    {
        TempoEvent te{};
        te.tick = 0;
        te.bpm = 120.0;
        sortedTempo.insert(sortedTempo.begin(), te);
    }

    double seconds = 0.0;
    int cursorTick = 0;
    double cursorBpm = sortedTempo.front().bpm;
    size_t tempoIndex = 1;
    while (tempoIndex < sortedTempo.size())
    {
        const int nextTick = sortedTempo[tempoIndex].tick;
        const int deltaTick = nextTick - cursorTick;
        const double secPerTick = (60.0 / cursorBpm) / static_cast<double>(ticksPerQuarter);
        const double segmentSeconds = secPerTick * static_cast<double>(deltaTick);
        if (seconds + segmentSeconds >= targetSeconds)
        {
            const double remain = targetSeconds - seconds;
            return cursorTick + static_cast<int>(remain / secPerTick);
        }
        seconds += segmentSeconds;
        cursorTick = nextTick;
        cursorBpm = sortedTempo[tempoIndex].bpm;
        tempoIndex++;
    }

    const double secPerTick = (60.0 / cursorBpm) / static_cast<double>(ticksPerQuarter);
    const double remain = targetSeconds - seconds;
    return (std::max)(0, cursorTick + static_cast<int>(remain / secPerTick));
}

void ResetInteractionState(PianoRollState& state)
{
    state.isRangeSelecting = false;
    state.isDraggingMove = false;
    state.isDraggingResize = false;
    state.isCreatingNote = false;
    state.dragTargetIndex = -1;
    state.dragSnapshot.clear();
}

void InvalidateVisibleCache(PianoRollState& state)
{
    state.visibleNoteIndexCacheValid = false;
}

void TouchNotesVersion(PianoRollState& state)
{
    state.notesVersion++;
    InvalidateVisibleCache(state);
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

bool DeleteSelectedNotes(PianoRollState& state)
{
    EnsureSelectionSize(state);
    if (!AnySelected(state))
    {
        return false;
    }

    const std::vector<PianoRollNote> before = state.notes;
    std::vector<PianoRollNote> after;
    after.reserve(state.notes.size());
    for (size_t i = 0; i < state.notes.size(); i++)
    {
        if (i < state.selected.size() && state.selected[i] != 0)
        {
            continue;
        }
        after.push_back(state.notes[i]);
    }
    state.notes = std::move(after);
    state.selected.assign(state.notes.size(), 0);
    state.primarySelectedIndex = -1;
    PushUndoCommand(state, before, state.notes);
    TouchNotesVersion(state);
    RecomputeMaxTick(state);
    SyncProjectDataFromCurrentNotes(state);
    return true;
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
    TouchNotesVersion(state);
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
    TouchNotesVersion(state);
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
    state.tempoEvents.clear();
    state.maxTick = 0;
    state.ticksPerQuarter = 480;
    state.selected.clear();
    state.primarySelectedIndex = -1;
    state.undoStack.clear();
    state.redoStack.clear();
    InvalidateVisibleCache(state);
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
    TouchNotesVersion(state);
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
    state.tempoEvents = tempoEvents;
    bool appliedProjectData = false;
    if (state.hasProjectData && state.projectMidiPath == midiPath && !state.projectNotes.empty())
    {
        // 専用project JSONがある場合は、MIDI由来ノートより編集済みノートを優先する。
        state.notes = state.projectNotes;
        TouchNotesVersion(state);
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
    float rulerHeight,
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
    const float noteAreaMinY = canvasMin.y + rulerHeight;
    const ImU32 laneDark = IM_COL32(30, 30, 34, 255);
    const ImU32 laneLight = IM_COL32(36, 36, 40, 255);
    const ImU32 laneC = IM_COL32(42, 46, 56, 255);
    const ImU32 keyDark = IM_COL32(24, 24, 28, 255);
    const ImU32 keyLight = IM_COL32(44, 44, 48, 255);
    const ImU32 rulerBg = IM_COL32(22, 24, 30, 255);

    drawList->AddRectFilled(
        ImVec2(gridMinX, canvasMin.y),
        ImVec2(canvasMax.x, noteAreaMinY),
        rulerBg);
    drawList->AddRectFilled(
        ImVec2(canvasMin.x, canvasMin.y),
        ImVec2(gridMinX, noteAreaMinY),
        keyLight);

    for (int row = 0; row < visibleCount; row++)
    {
        const int note = noteHigh - row;
        const float y0 = noteAreaMinY + row * rowHeight;
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
        const std::string noteText = std::to_string(note);
        const ImVec2 textSize = ImGui::CalcTextSize(noteText.c_str());
        const float textY = y0 + (std::max)(0.0f, (rowHeight - textSize.y) * 0.5f);
        drawList->AddText(ImVec2(canvasMin.x + 4.0f, textY), IM_COL32(210, 210, 215, 255), noteText.c_str());
    }

    const int firstSnapTick = (startTick / snapStep) * snapStep;
    const float beatPixelStep = static_cast<float>(tpq) * pxPerTick;
    const float minLabelGapPx = (std::max)(20.0f, ImGui::CalcTextSize("0000").x + 8.0f);
    int beatLabelStep = 1;
    if (beatPixelStep > 0.0001f)
    {
        beatLabelStep = (std::max)(1, static_cast<int>(std::ceil(minLabelGapPx / beatPixelStep)));
    }
    for (int tick = firstSnapTick; tick <= endTick; tick += snapStep)
    {
        const float x = gridMinX + (tick - startTick) * pxPerTick;
        const bool beatLine = (tick % tpq) == 0;
        const ImU32 col = beatLine ? IM_COL32(120, 130, 150, 180) : IM_COL32(90, 95, 110, 90);
        drawList->AddLine(ImVec2(x, noteAreaMinY), ImVec2(x, canvasMax.y), col);
        if (beatLine)
        {
            const int beat = tick / tpq;
            if ((beat % beatLabelStep) == 0)
            {
                const std::string beatText = std::to_string(beat);
                drawList->AddText(ImVec2(x + 2.0f, canvasMin.y + 1.0f), IM_COL32(185, 190, 205, 255), beatText.c_str());
            }
            drawList->AddLine(ImVec2(x, canvasMin.y), ImVec2(x, noteAreaMinY), IM_COL32(120, 130, 150, 180), 1.0f);
        }
    }

    drawList->AddLine(ImVec2(canvasMin.x, noteAreaMinY), ImVec2(canvasMax.x, noteAreaMinY), IM_COL32(120, 125, 140, 180), 1.0f);
    drawList->AddLine(ImVec2(gridMinX, canvasMin.y), ImVec2(gridMinX, canvasMax.y), IM_COL32(180, 180, 190, 180), 1.0f);
}

void EnsureVisibleNoteIndexCache(
    PianoRollState& state,
    int noteLow,
    int noteHigh,
    int startTick,
    int endTick)
{
    const bool cacheHit =
        state.visibleNoteIndexCacheValid &&
        state.cacheNotesVersion == state.notesVersion &&
        state.visibleCacheKey.displayChannel == state.displayChannel &&
        state.visibleCacheKey.noteLow == noteLow &&
        state.visibleCacheKey.noteHigh == noteHigh &&
        state.visibleCacheKey.startTick == startTick &&
        state.visibleCacheKey.endTick == endTick;
    if (cacheHit)
    {
        return;
    }

    state.visibleNoteIndexCache.clear();
    state.visibleNoteIndexCache.reserve(512);
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
        state.visibleNoteIndexCache.push_back(i);
    }

    state.visibleCacheKey.displayChannel = state.displayChannel;
    state.visibleCacheKey.noteLow = noteLow;
    state.visibleCacheKey.noteHigh = noteHigh;
    state.visibleCacheKey.startTick = startTick;
    state.visibleCacheKey.endTick = endTick;
    state.cacheNotesVersion = state.notesVersion;
    state.visibleNoteIndexCacheValid = true;
}

void BuildVisibleDrawNotes(
    PianoRollState& state,
    const ImVec2& canvasMin,
    const ImVec2& canvasMax,
    float pianoWidth,
    float rulerHeight,
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
    const float noteAreaMinY = canvasMin.y + rulerHeight;

    EnsureVisibleNoteIndexCache(state, noteLow, noteHigh, startTick, endTick);
    out.reserve(state.visibleNoteIndexCache.size());
    for (int idx : state.visibleNoteIndexCache)
    {
        if (idx < 0 || idx >= static_cast<int>(state.notes.size()))
        {
            continue;
        }
        const auto& n = state.notes[static_cast<size_t>(idx)];
        const int row = noteHigh - n.note;
        const float y0 = noteAreaMinY + row * rowHeight + 1.0f;
        const float y1 = y0 + rowHeight - 2.0f;
        const float x0 = gridMinX + (n.startTick - startTick) * pxPerTick;
        const float x1 = gridMinX + (n.endTick - startTick) * pxPerTick;
        const float w = (std::max)(x1 - x0, 2.0f);

        DrawNoteInfo info{};
        info.index = idx;
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
    InvalidateVisibleCache(state);
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
    InvalidateVisibleCache(state);
    RecomputeMaxTick(state);
}

void DrawCreatingNotePreview(
    const PianoRollState& state,
    ImDrawList* drawList,
    const ImVec2& canvasMin,
    const ImVec2& canvasMax,
    float pianoWidth,
    float rowHeight,
    float pxPerTick)
{
    if (!state.isCreatingNote)
    {
        return;
    }

    const int drawStartTick = (std::max)(0, state.tickOffset);
    int tick0 = (std::min)(state.createStartTick, state.createCurrentTick);
    int tick1 = (std::max)(state.createStartTick, state.createCurrentTick);
    if (tick1 <= tick0)
    {
        tick1 = tick0 + 1;
    }

    const int noteHigh = (std::min)(127, state.noteOffset + state.visibleNoteCount - 1);
    const int row = noteHigh - ClampNote(state.createNote);
    if (row < 0 || row >= state.visibleNoteCount)
    {
        return;
    }

    const float x0 = canvasMin.x + pianoWidth + static_cast<float>(tick0 - drawStartTick) * pxPerTick;
    const float x1 = canvasMin.x + pianoWidth + static_cast<float>(tick1 - drawStartTick) * pxPerTick;
    const float y0 = canvasMin.y + row * rowHeight;
    const float y1 = y0 + rowHeight;

    drawList->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(130, 220, 255, 100), 2.0f);
    drawList->AddRect(ImVec2(x0, y0), ImVec2(x1, y1), IM_COL32(120, 220, 255, 220), 2.0f);
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
    const PreviewPlaybackState* playback,
    const std::function<void(const std::string&)>& appendLog,
    const std::function<void()>& requestPreviewPlay,
    const std::function<void()>& requestPreviewStop)
{
    const std::filesystem::path midiPath = (midiPathUtf8 != nullptr) ? Utf8ToPath(midiPathUtf8) : std::filesystem::path{};
    EnsureModelLoaded(state, midiPath, appendLog);
    EnsureSelectionSize(state);

    ImGui::TextUnformatted("Piano Roll");
    ImGui::SetNextItemWidth(140.0f);
    ImGui::SliderInt("PR Channel", &state.displayChannel, 0, 15);
    ImGui::SameLine();
    ImGui::Text("Snap: %s", SnapLabel(state.snapIndex));
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
    ImGui::Checkbox("PR Follow", &state.followPreviewPlayback);
    ImGui::SameLine();
    if (state.previewRangeEnabled)
    {
        ImGui::Text("PR Range=%d-%d", state.previewRangeStartTick, state.previewRangeEndTick);
    }
    else
    {
        ImGui::Text("PR StartTick=%d", state.previewStartTick);
    }
    ImGui::TextDisabled("操作: 左ドラッグ=移動 / 端ドラッグ=長さ / 空白ドラッグ=追加 / Shift+空白=範囲選択 / Delete=削除 / ルーラD&D=再生範囲 / ルーラ後Ctrl+A=全範囲 / Space=再生停止 / Q,1-4=Snap");

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
                const int tickStep = (std::max)(1, state.ticksPerQuarter / 2);
                state.tickOffset = (std::max)(0, state.tickOffset - wheelSteps * tickStep);
            }
            else
            {
                // 通常ホイール: 音高軸の縦スクロール。
                const int noteStep = (std::max)(1, state.visibleNoteCount / 8);
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
            const int tickStep = (std::max)(1, state.ticksPerQuarter / 2);
            state.tickOffset = (std::max)(0, state.tickOffset - wheelHSteps * tickStep);
            InvalidateVisibleCache(state);
        }
    }

    const bool rulerDoubleClicked = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && mouseInRuler;
    if (rulerDoubleClicked)
    {
        state.previewRangeShortcutArmed = true;
        state.previewRangeEnabled = true;
        state.previewRangeStartTick = 0;
        state.previewRangeEndTick = (std::max)(state.maxTick, 0);
        NormalizePreviewRange(state);
        if (appendLog)
        {
            appendLog("[PianoRoll] preview range set: full");
        }
    }

    if (!rulerDoubleClicked && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && mouseInRuler)
    {
        state.previewRangeShortcutArmed = true;
        state.isPreviewRangeDragging = true;
        state.previewRangeDragStartTick = SnapTick(mouseTick, snapStep);
        state.previewRangeStartTick = state.previewRangeDragStartTick;
        state.previewRangeEndTick = state.previewRangeDragStartTick;
        state.previewRangeEnabled = true;
    }
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !mouseInRuler)
    {
        state.previewRangeShortcutArmed = false;
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
                    appendLog("[PianoRoll] preview start tick set: " + std::to_string(state.previewStartTick));
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
        if (state.previewRangeShortcutArmed && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A, false))
        {
            state.previewRangeEnabled = true;
            state.previewRangeStartTick = 0;
            state.previewRangeEndTick = (std::max)(state.maxTick, 0);
            NormalizePreviewRange(state);
            if (appendLog)
            {
                appendLog("[PianoRoll] preview range set: full (Ctrl+A)");
            }
        }
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
} // namespace gui
