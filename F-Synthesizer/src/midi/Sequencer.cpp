#include <algorithm>
#include <limits>
#include <stdexcept>
#include "MidiFile.h"

#include "midi/Sequencer.h"

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


}

enum class TickPriority
{
    ControlChange = 0,
    NoteOff       = 1,
    NoteOn        = 2
};

int PriorityValue(const MIDIEventTick& e)
{
    if (e.type == MIDIEventType::ControlChange ||
        e.type == MIDIEventType::PitchBend ||
        e.type == MIDIEventType::ChannelPressure ||
        e.type == MIDIEventType::PolyPressure ||
        e.type == MIDIEventType::ProgramChange)
    {
        return static_cast<int>(TickPriority::ControlChange);
    }
    return static_cast<int>(e.isNoteOn ? TickPriority::NoteOn : TickPriority::NoteOff);
}

MIDIEvent MakeControlChangeEvent(const MIDIEventTick& t, int sample)
{
    MIDIEvent e{};
    e.type       = MIDIEventType::ControlChange;
    e.sample     = sample;
    e.noteNumber = 0;
    e.velocity   = 0;
    e.channel    = t.channel;
    e.noteInstanceID = -1;
    e.controller = t.controller;
    e.value      = t.value;
    e.isNoteOn   = false;
    return e;
}

MIDIEvent MakeNoteEvent(const MIDIEventTick& t, int sample)
{
    MIDIEvent e{};
    e.type       = MIDIEventType::Note;
    e.sample     = sample;
    e.noteNumber = t.noteNumber;
    e.velocity   = t.velocity;
    e.channel    = t.channel;
    e.noteInstanceID = t.noteInstanceID;
    e.controller = 0;
    e.value      = 0;
    e.isNoteOn   = t.isNoteOn;
    return e;
}

MIDIEvent MakePitchBendEvent(const MIDIEventTick& t, int sample)
{
    MIDIEvent e{};
    e.type       = MIDIEventType::PitchBend;
    e.sample     = sample;
    e.noteNumber = 0;
    e.velocity   = 0;
    e.channel    = t.channel;
    e.noteInstanceID = -1;
    e.controller = 0;
    e.value      = t.value;
    e.isNoteOn   = false;
    return e;
}

MIDIEvent MakeChannelPressureEvent(const MIDIEventTick& t, int sample)
{
    MIDIEvent e{};
    e.type = MIDIEventType::ChannelPressure;
    e.sample = sample;
    e.noteNumber = 0;
    e.velocity = 0;
    e.channel = t.channel;
    e.noteInstanceID = -1;
    e.controller = 0;
    e.value = t.value;
    e.isNoteOn = false;
    return e;
}

MIDIEvent MakePolyPressureEvent(const MIDIEventTick& t, int sample)
{
    MIDIEvent e{};
    e.type = MIDIEventType::PolyPressure;
    e.sample = sample;
    e.noteNumber = t.noteNumber;
    e.velocity = 0;
    e.channel = t.channel;
    e.noteInstanceID = -1;
    e.controller = 0;
    e.value = t.value;
    e.isNoteOn = false;
    return e;
}

MIDIEvent MakeProgramChangeEvent(const MIDIEventTick& t, int sample)
{
    MIDIEvent e{};
    e.type = MIDIEventType::ProgramChange;
    e.sample = sample;
    e.noteNumber = 0;
    e.velocity = 0;
    e.channel = t.channel;
    e.noteInstanceID = -1;
    e.controller = 0;
    e.value = t.value;
    e.isNoteOn = false;
    return e;
}

void BuildSampleEvents(const std::vector<MIDIEventTick>& ticks,
    const std::vector<TempoEvent>& tempoEvents,
    int ticksPerQuarter,
    int sampleRate,
    std::vector<MIDIEvent>& outEvents)
{
    std::vector<TempoEvent> sortedTempo = NormalizeTempoEvents(tempoEvents);
    std::vector<MIDIEventTick> sortedTicks = SortTicksWithPriority(ticks);

    // tick -> sample の変換準備
    outEvents.clear();
    outEvents.reserve(sortedTicks.size());
    if (ticksPerQuarter <= 0 || sampleRate <= 0) return;
    smf::MidiFile timing;
    timing.setTicksPerQuarterNote(ticksPerQuarter);
    for (const auto& tempo : sortedTempo)
        timing.addTempo(0, tempo.tick, tempo.bpm > 0.0 ? tempo.bpm : 120.0);
    // Include every requested tick: upstream sparse-map interpolation misses
    // the latter half of a two-point map. Exact entries also avoid linear searches.
    std::vector<unsigned char> marker{0xff, 0x7f, 0x00};
    int previousTick = -1;
    for (const auto& event : sortedTicks)
    {
        if (event.tick != previousTick) timing.addEvent(0, event.tick, marker);
        previousTick = event.tick;
    }
    timing.sortTracks();
    timing.doTimeAnalysis();

    // tick イベントを sample イベントへ変換
    for (const auto& t : sortedTicks)
    {
        const double atSample = timing.getTimeInSeconds(t.tick) * sampleRate;
        if (atSample < 0.0 || atSample >= (std::numeric_limits<int>::max)())
            throw std::runtime_error("MIDI duration is outside the supported range");
        const int sample = static_cast<int>(atSample + 1e-7);
        if (t.type == MIDIEventType::ControlChange)
        {
            outEvents.push_back(MakeControlChangeEvent(t, sample));
            continue;
        }

        if (t.type == MIDIEventType::PitchBend)
        {
            outEvents.push_back(MakePitchBendEvent(t, sample));
            continue;
        }
        if (t.type == MIDIEventType::ChannelPressure)
        {
            outEvents.push_back(MakeChannelPressureEvent(t, sample));
            continue;
        }
        if (t.type == MIDIEventType::PolyPressure)
        {
            outEvents.push_back(MakePolyPressureEvent(t, sample));
            continue;
        }
        if (t.type == MIDIEventType::ProgramChange)
        {
            outEvents.push_back(MakeProgramChangeEvent(t, sample));
            continue;
        }

        outEvents.push_back(MakeNoteEvent(t, sample));
    }

    // sample 順で整列
    std::stable_sort(outEvents.begin(), outEvents.end(), [](const MIDIEvent& a, const MIDIEvent& b)
    {
        // 同sample内の優先順（Control/NoteOff/NoteOn）を保持するため stable_sort を使う。
        return a.sample < b.sample;
    });
}
