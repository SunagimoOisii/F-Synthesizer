#include "Sequencer.h"

#include <algorithm>

void BuildSampleEvents(const std::vector<MIDIEventTick>& ticks,
    const std::vector<TempoEvent>& tempoEvents,
    int ticksPerQuarter,
    int sampleRate,
    WaveType defaultWave,
    std::vector<MidiEvent>& outEvents)
{
    //テンポイベントの整列と補完
    std::vector<TempoEvent> sortedTempo = tempoEvents;
    std::sort(sortedTempo.begin(), sortedTempo.end(), [](const TempoEvent& a, const TempoEvent& b)
    {
        return a.tick < b.tick;
    });
    if (sortedTempo.empty() || sortedTempo.front().tick != 0)
    {
        TempoEvent te{};
        te.tick = 0;
        te.bpm = 120.0;
        sortedTempo.insert(sortedTempo.begin(), te);
    }

    //tickイベントの整列(同tick内の優先順位も調整)
    std::vector<MIDIEventTick> sortedTicks = ticks;
    std::sort(sortedTicks.begin(), sortedTicks.end(), [](const MIDIEventTick& a, const MIDIEventTick& b)
    {
        if (a.tick != b.tick) return a.tick < b.tick;
        int aPri = (a.type == MIDIEventType::ControlChange) ? 0 : (a.isNoteOn ? 2 : 1);
        int bPri = (b.type == MIDIEventType::ControlChange) ? 0 : (b.isNoteOn ? 2 : 1);
        if (aPri != bPri) return aPri < bPri;
        return a.order < b.order;
    });

    //tick -> sample の変換準備
    outEvents.clear();
    outEvents.reserve(sortedTicks.size());
    int currentTick = 0;
    double currentSample = 0.0;
    double currentBPM = sortedTempo.front().bpm;
    size_t tempoIndex = 1;

    //テンポマップを使って現在時刻を進める
    auto advanceToTick = [&](int targetTick)
    {
        while (tempoIndex < sortedTempo.size() && sortedTempo[tempoIndex].tick <= targetTick)
        {
            int tempoTick = sortedTempo[tempoIndex].tick;
            int deltaTicks = tempoTick - currentTick;
            double secPerQuarter = 60.0 / currentBPM;
            currentSample += (deltaTicks / (double)ticksPerQuarter) * secPerQuarter * sampleRate;
            currentTick = tempoTick;
            currentBPM = sortedTempo[tempoIndex].bpm;
            tempoIndex++;
        }
        int deltaTicks = targetTick - currentTick;
        double secPerQuarter = 60.0 / currentBPM;
        currentSample += (deltaTicks / (double)ticksPerQuarter) * secPerQuarter * sampleRate;
        currentTick = targetTick;
    };

    //tickイベントをsampleイベントへ変換
    for (const auto& t : sortedTicks)
    {
        advanceToTick(t.tick);
        if (t.type == MIDIEventType::ControlChange)
        {
            MidiEvent e{};
            e.sample = (int)(currentSample);
            e.type = MIDIEventType::ControlChange;
            e.isNoteOn = false;
            e.noteNumber = 0;
            e.velocity = 0;
            e.channel = t.channel;
            e.controller = t.controller;
            e.value = t.value;
            e.typeWave = defaultWave;
            outEvents.push_back(e);
            continue;
        }

        MidiEvent e{};
        e.sample = (int)(currentSample);
        e.type = MIDIEventType::Note;
        e.isNoteOn = t.isNoteOn;
        e.noteNumber = t.noteNumber;
        e.velocity = t.velocity;
        e.channel = t.channel;
        e.controller = 0;
        e.value = 0;
        e.typeWave = defaultWave;
        outEvents.push_back(e);
    }

    //sample順で整列
    std::stable_sort(outEvents.begin(), outEvents.end(), [](const MidiEvent& a, const MidiEvent& b)
    {
        return a.sample < b.sample;
    });
}

