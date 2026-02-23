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

