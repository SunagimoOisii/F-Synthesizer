#pragma once

#include <vector>

#include "MIDIParser.h"
#include "Oscillator.h"

struct MIDIEvent
{
    //時間
    int sample;

    //イベント種別
    MIDIEventType type;
    bool isNoteOn;

    //ノート
    int noteNumber;
    int velocity;
    int channel;

    //コントロールチェンジ
    int controller;
    int value;

    //音色
    WaveType typeWave;
};

void BuildSampleEvents(const std::vector<MIDIEventTick>& ticks,
    const std::vector<TempoEvent>& tempoEvents,
    int ticksPerQuarter,
    int sampleRate,
    WaveType defaultWave,
    std::vector<MIDIEvent>& outEvents);