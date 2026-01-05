#pragma once

#include <vector>

#include "MidiParser.h"
#include "Oscillator.h"

struct MidiEvent
{
    int sample;
    MidiEventType type;
    bool isNoteOn;
    int noteNumber;
    int velocity;
    int channel;
    int controller;
    int value;
    WaveType typeWave;
};

void BuildSampleEvents(const std::vector<MidiEventTick>& ticks,
    const std::vector<TempoEvent>& tempoEvents,
    int ticksPerQuarter, int sampleRate,
    WaveType defaultWave, std::vector<MidiEvent>& outEvents);