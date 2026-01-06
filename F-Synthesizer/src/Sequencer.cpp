#include <algorithm>

#include "Sequencer.h"

enum class TickPriority
{
    ControlChange = 0,
    NoteOff       = 1,
    NoteOn        = 2
};

int PriorityValue(const MIDIEventTick& e)
{
    if (e.type == MIDIEventType::ControlChange)
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

void BuildSampleEvents(const std::vector<MIDIEventTick>& ticks,
    const std::vector<TempoEvent>& tempoEvents,
    int ticksPerQuarter,
    int sampleRate,
    WaveType defaultWave,
    std::vector<MIDIEvent>& outEvents)
{
    //テンポイベントの整列と補完
    std::vector<TempoEvent> sortedTempo = tempoEvents;
    std::sort(sortedTempo.begin(), sortedTempo.end(), [](const TempoEvent& a, const TempoEvent& b)
    {
        return a.tick < b.tick;
    });
    if (sortedTempo.empty() ||
        sortedTempo.front().tick != 0)
    {
        TempoEvent te{};
        te.bpm  = 120.0;
        te.tick = 0;
        sortedTempo.insert(sortedTempo.begin(), te);
    }

    //tickイベントの整列(同tick内の優先順位も調整)
    std::vector<MIDIEventTick> sortedTicks = ticks;
    std::sort(sortedTicks.begin(), sortedTicks.end(), [](const MIDIEventTick& a, const MIDIEventTick& b)
    {
        if (a.tick != b.tick) return a.tick < b.tick;
        int aPri = PriorityValue(a);
        int bPri = PriorityValue(b);
        if (aPri != bPri) return aPri < bPri;
        return a.order < b.order;
    });

    //tick -> sample の変換準備
    outEvents.clear();
    outEvents.reserve(sortedTicks.size());
    int currentTick      = 0;
    double currentBPM    = sortedTempo.front().bpm;
    double currentSample = 0.0;
    size_t tempoIndex    = 1;

    //テンポマップを使って現在時刻を進める
    auto advanceToTick = [&](int targetTick)
    {
        while (tempoIndex < sortedTempo.size() &&
               sortedTempo[tempoIndex].tick <= targetTick)
        {
            int tempoTick  = sortedTempo[tempoIndex].tick;
            int deltaTicks = tempoTick - currentTick;
            double secPerQuarter = 60.0 / currentBPM;
            double samplesPerTick = (secPerQuarter * sampleRate) / ticksPerQuarter;

            currentSample += deltaTicks * samplesPerTick;
            currentTick    = tempoTick;
            currentBPM     = sortedTempo[tempoIndex].bpm;
            tempoIndex++;
        }
        int deltaTicks       = targetTick - currentTick;
        double secPerQuarter = 60.0 / currentBPM;
        double samplesPerTick = (secPerQuarter * sampleRate) / ticksPerQuarter;

        currentSample += deltaTicks * samplesPerTick;
        currentTick    = targetTick;
    };

    //tickイベントをsampleイベントへ変換
    for (const auto& t : sortedTicks)
    {
        advanceToTick(t.tick);
        int sample = (int)(currentSample);
        if (t.type == MIDIEventType::ControlChange)
        {
            outEvents.push_back(MakeControlChangeEvent(t, sample, defaultWave));
            continue;
        }

        outEvents.push_back(MakeNoteEvent(t, sample, defaultWave));
    }

    //sample順で整列
    std::stable_sort(outEvents.begin(), outEvents.end(), [](const MIDIEvent& a, const MIDIEvent& b)
    {
        return a.sample < b.sample;
    });
}
