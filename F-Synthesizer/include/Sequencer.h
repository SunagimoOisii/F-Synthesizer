#pragma once

#include <vector>

#include "MIDIParser.h"
#include "Oscillator.h"

struct MidiEvent
{
    MIDIEventType type;
    WaveType typeWave;
    int sample;
    int noteNumber;
    int velocity;
    int channel;
    int controller;
    int value;
    bool isNoteOn;
};

void BuildSampleEvents(const std::vector<MIDIEventTick>& ticks,
    const std::vector<TempoEvent>& tempoEvents,
    int ticksPerQuarter, int sampleRate,
    WaveType defaultWave, std::vector<MidiEvent>& outEvents);