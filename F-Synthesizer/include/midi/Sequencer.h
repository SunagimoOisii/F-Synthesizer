#pragma once

#include <vector>

#include "midi/MIDIParser.h"
#include "synth/Oscillator.h"

struct MIDIEvent
{
    // sample軸の発火位置
    int sample;

    // イベント種別
    MIDIEventType type;
    bool isNoteOn;

    // ノート情報
    int noteNumber;
    int velocity;
    int channel;
    // NoteOn/NoteOff 対応付け用ID。Note以外は -1。
    int noteInstanceId;

    // Control Change / Pitch Bend の値
    int controller;
    int value;

    // 互換維持用の既定波形情報
    WaveType typeWave;
};

void BuildSampleEvents(const std::vector<MIDIEventTick>& ticks,
    const std::vector<TempoEvent>& tempoEvents,
    int ticksPerQuarter,
    int sampleRate,
    WaveType defaultWave,
    std::vector<MIDIEvent>& outEvents);
