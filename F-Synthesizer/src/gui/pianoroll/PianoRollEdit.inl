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
    state.hasProjectData = true;
}


void ClearModel(PianoRollState& state)
{
    state.notes.clear();
    state.tempoEvents.clear();
    state.noteCountByChannel.fill(0);
    state.programByChannel.fill(0);
    state.hasProgramByChannel.fill(false);
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
        if (e.type == MIDIEventType::ProgramChange)
        {
            const int ch = ClampChannel(e.channel);
            state.hasProgramByChannel[ch] = true;
            state.programByChannel[ch] = std::clamp(e.value, 0, 127);
            continue;
        }
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
            state.noteCountByChannel[ch]++;
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
                state.noteCountByChannel[ch]++;
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
    if (state.hasProjectData && state.projectMidiPath == midiPath)
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

