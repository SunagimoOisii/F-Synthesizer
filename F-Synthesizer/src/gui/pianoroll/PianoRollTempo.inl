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
