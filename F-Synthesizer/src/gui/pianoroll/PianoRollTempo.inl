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

double SecondsAtTick(const std::vector<TempoEvent>& events, int tpq, int tick)
{
    return tpq > 0 ? midi::TempoMap(events, tpq).SecondsAtTick(tick) : 0.0;
}
int TickAtSeconds(const std::vector<TempoEvent>& events, int tpq, double seconds)
{
    // The piano roll follows the last complete tick; the transport rounds to nearest.
    return tpq > 0 ? static_cast<int>(midi::TempoMap(events, tpq).TickAtSeconds(seconds)) : 0;
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
