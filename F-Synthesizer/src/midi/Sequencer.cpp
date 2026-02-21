#include <algorithm>

#include "Sequencer.h"

int PriorityValue(const MIDIEventTick& e);

namespace
{
    std::vector<TempoEvent> NormalizeTempoEvents(const std::vector<TempoEvent>& tempoEvents)
    {
        std::vector<TempoEvent> sortedTempo = tempoEvents;
        std::sort(sortedTempo.begin(), sortedTempo.end(), [](const TempoEvent& a, const TempoEvent& b)
        {
            return a.tick < b.tick;
        });
        if (sortedTempo.empty() || sortedTempo.front().tick != 0)
        {
            TempoEvent te{};
            te.bpm = 120.0;
            te.tick = 0;
            sortedTempo.insert(sortedTempo.begin(), te);
        }
        return sortedTempo;
    }

    std::vector<MIDIEventTick> SortTicksWithPriority(const std::vector<MIDIEventTick>& ticks)
    {
        std::vector<MIDIEventTick> sortedTicks = ticks;
        std::sort(sortedTicks.begin(), sortedTicks.end(), [](const MIDIEventTick& a, const MIDIEventTick& b)
        {
            // 同tickは Control -> NoteOff -> NoteOn の順で処理する。
            // 目的: 同時刻イベントの音切れ/重なりを安定させる。
            if (a.tick != b.tick) return a.tick < b.tick;
            int aPri = PriorityValue(a);
            int bPri = PriorityValue(b);
            if (aPri != bPri) return aPri < bPri;
            return a.order < b.order;
        });
        return sortedTicks;
    }

    struct TempoCursor
    {
        int currentTick = 0;
        double currentBPM = 120.0;
        double currentSample = 0.0;
        size_t tempoIndex = 0;
    };

    void AdvanceToTick(TempoCursor& cursor,
        int targetTick,
        const std::vector<TempoEvent>& tempoEvents,
        int ticksPerQuarter,
        int sampleRate)
    {
        while (cursor.tempoIndex < tempoEvents.size() &&
            tempoEvents[cursor.tempoIndex].tick <= targetTick)
        {
            int tempoTick = tempoEvents[cursor.tempoIndex].tick;
            int deltaTicks = tempoTick - cursor.currentTick;
            double secPerQuarter = 60.0 / cursor.currentBPM;
            double samplesPerTick = (secPerQuarter * sampleRate) / ticksPerQuarter;

            cursor.currentSample += deltaTicks * samplesPerTick;
            cursor.currentTick = tempoTick;
            cursor.currentBPM = tempoEvents[cursor.tempoIndex].bpm;
            cursor.tempoIndex++;
        }
        int deltaTicks = targetTick - cursor.currentTick;
        double secPerQuarter = 60.0 / cursor.currentBPM;
        double samplesPerTick = (secPerQuarter * sampleRate) / ticksPerQuarter;

        cursor.currentSample += deltaTicks * samplesPerTick;
        cursor.currentTick = targetTick;
    }
}

enum class TickPriority
{
    ControlChange = 0,
    NoteOff       = 1,
    NoteOn        = 2
};

int PriorityValue(const MIDIEventTick& e)
{
    if (e.type == MIDIEventType::ControlChange || e.type == MIDIEventType::PitchBend)
    {
        return static_cast<int>(TickPriority::ControlChange);
    }
    return static_cast<int>(e.isNoteOn ? TickPriority::NoteOn : TickPriority::NoteOff);
}

MIDIEvent MakeControlChangeEvent(const MIDIEventTick& t, int sample, WaveType defaultWave)
{
    MIDIEvent e{};
    e.type       = MIDIEventType::ControlChange;
    e.typeWave   = defaultWave;
    e.sample     = sample;
    e.noteNumber = 0;
    e.velocity   = 0;
    e.channel    = t.channel;
    e.controller = t.controller;
    e.value      = t.value;
    e.isNoteOn   = false;
    return e;
}

MIDIEvent MakeNoteEvent(const MIDIEventTick& t, int sample, WaveType defaultWave)
{
    MIDIEvent e{};
    e.type       = MIDIEventType::Note;
    e.typeWave   = defaultWave;
    e.sample     = sample;
    e.noteNumber = t.noteNumber;
    e.velocity   = t.velocity;
    e.channel    = t.channel;
    e.controller = 0;
    e.value      = 0;
    e.isNoteOn   = t.isNoteOn;
    return e;
}

MIDIEvent MakePitchBendEvent(const MIDIEventTick& t, int sample, WaveType defaultWave)
{
    MIDIEvent e{};
    e.type       = MIDIEventType::PitchBend;
    e.typeWave   = defaultWave;
    e.sample     = sample;
    e.noteNumber = 0;
    e.velocity   = 0;
    e.channel    = t.channel;
    e.controller = 0;
    e.value      = t.value;
    e.isNoteOn   = false;
    return e;
}

void BuildSampleEvents(const std::vector<MIDIEventTick>& ticks,
    const std::vector<TempoEvent>& tempoEvents,
    int ticksPerQuarter,
    int sampleRate,
    WaveType defaultWave,
    std::vector<MIDIEvent>& outEvents)
{
    std::vector<TempoEvent> sortedTempo = NormalizeTempoEvents(tempoEvents);
    std::vector<MIDIEventTick> sortedTicks = SortTicksWithPriority(ticks);

    // tick -> sample の変換準備
    outEvents.clear();
    outEvents.reserve(sortedTicks.size());
    TempoCursor cursor{};
    cursor.currentBPM = sortedTempo.front().bpm;
    cursor.tempoIndex = 1;

    // tick イベントを sample イベントへ変換
    for (const auto& t : sortedTicks)
    {
        AdvanceToTick(cursor, t.tick, sortedTempo, ticksPerQuarter, sampleRate);
        int sample = (int)(cursor.currentSample);
        if (t.type == MIDIEventType::ControlChange)
        {
            outEvents.push_back(MakeControlChangeEvent(t, sample, defaultWave));
            continue;
        }

        if (t.type == MIDIEventType::PitchBend)
        {
            outEvents.push_back(MakePitchBendEvent(t, sample, defaultWave));
            continue;
        }

        outEvents.push_back(MakeNoteEvent(t, sample, defaultWave));
    }

    // sample 順で整列
    std::stable_sort(outEvents.begin(), outEvents.end(), [](const MIDIEvent& a, const MIDIEvent& b)
    {
        return a.sample < b.sample;
    });
}
