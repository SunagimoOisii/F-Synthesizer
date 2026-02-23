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

