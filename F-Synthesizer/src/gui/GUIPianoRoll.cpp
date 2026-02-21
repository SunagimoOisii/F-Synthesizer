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
int ClampChannel(int channel)
{
    return (channel >= 0 && channel < 16) ? channel : 0;
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
    case 1: return (std::max)(1, tpq);
    case 2: return (std::max)(1, tpq / 2);
    case 3: return (std::max)(1, tpq / 4);
    case 4: return (std::max)(1, tpq / 8);
    default: return (std::max)(1, tpq / 4);
    }
}

void ClearModel(PianoRollState& state)
{
    state.notes.clear();
    state.maxTick = 0;
    state.ticksPerQuarter = 480;
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
    std::vector<std::vector<int>> noteOnQueues(kSlots);
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
        const int note = std::clamp(e.noteNumber, 0, 127);
        const size_t slot = static_cast<size_t>(ch * 128 + note);

        if (e.isNoteOn)
        {
            noteOnQueues[slot].push_back(e.tick);
            continue;
        }

        auto& q = noteOnQueues[slot];
        size_t& head = queueHeads[slot];
        if (head < q.size())
        {
            PianoRollNote n{};
            n.startTick = q[head++];
            n.endTick = (std::max)(n.startTick + 1, e.tick);
            n.note = note;
            n.channel = ch;
            n.velocity = e.velocity;
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
                n.startTick = q[head++];
                n.endTick = (std::max)(n.startTick + 1, fallbackEndTick);
                n.note = note;
                n.channel = ch;
                n.velocity = 0;
                state.notes.push_back(n);
            }
        }
    }
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
} // namespace

void DrawPianoRollPanel(
    PianoRollState& state,
    const char* midiPathUtf8,
    const std::function<void(const std::string&)>& appendLog)
{
    const std::filesystem::path midiPath = (midiPathUtf8 != nullptr) ? Utf8ToPath(midiPathUtf8) : std::filesystem::path{};
    EnsureModelLoaded(state, midiPath, appendLog);

    ImGui::TextUnformatted("Piano Roll (Phase 1: View Base)");
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
        ImGui::Text("PianoRoll notes=%zu, tpq=%d, maxTick=%d", state.notes.size(), state.ticksPerQuarter, state.maxTick);
    }

    const float rowHeight = 12.0f;
    const float pianoWidth = 56.0f;
    const float panelHeight = std::clamp(state.visibleNoteCount * rowHeight + 4.0f, 180.0f, 460.0f);
    const ImVec2 canvasSize((std::max)(120.0f, ImGui::GetContentRegionAvail().x - 4.0f), panelHeight);
    ImGui::InvisibleButton("piano_roll_canvas", canvasSize);

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImVec2 canvasMin = ImGui::GetItemRectMin();
    const ImVec2 canvasMax = ImGui::GetItemRectMax();
    drawList->AddRectFilled(canvasMin, canvasMax, IM_COL32(20, 20, 24, 255));
    drawList->AddRect(canvasMin, canvasMax, IM_COL32(90, 90, 100, 255));

    const float tpq = static_cast<float>((std::max)(1, state.ticksPerQuarter));
    const float pxPerTick = (std::max)(0.01f, state.pixelsPerQuarter / tpq);
    DrawPianoGrid(state, drawList, canvasMin, canvasMax, pianoWidth, rowHeight, pxPerTick);

    if (state.hasLoadError || state.notes.empty())
    {
        return;
    }

    drawList->PushClipRect(ImVec2(canvasMin.x + pianoWidth, canvasMin.y), canvasMax, true);
    const int noteLow = state.noteOffset;
    const int noteHigh = (std::min)(127, noteLow + state.visibleNoteCount - 1);
    const int startTick = (std::max)(0, state.tickOffset);
    const int endTick = startTick + static_cast<int>((canvasMax.x - canvasMin.x - pianoWidth) / pxPerTick) + 1;
    const ImU32 noteColor = IM_COL32(120, 200, 255, 210);
    const ImU32 noteBorder = IM_COL32(50, 120, 170, 255);

    for (const auto& n : state.notes)
    {
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
        const float x0 = canvasMin.x + pianoWidth + (n.startTick - startTick) * pxPerTick;
        const float x1 = canvasMin.x + pianoWidth + (n.endTick - startTick) * pxPerTick;
        const float w = (std::max)(x1 - x0, 2.0f);
        drawList->AddRectFilled(ImVec2(x0, y0), ImVec2(x0 + w, y1), noteColor, 2.0f);
        drawList->AddRect(ImVec2(x0, y0), ImVec2(x0 + w, y1), noteBorder, 2.0f);
    }
    drawList->PopClipRect();
}
} // namespace gui
