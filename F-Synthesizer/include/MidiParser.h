#pragma once

#include <string>
#include <vector>

enum class MidiEventType
{
    Note,
    ControlChange
};

struct MidiEventTick
{
    int tick;
    MidiEventType type;
    bool isNoteOn;
    int noteNumber;
    int velocity;
    int channel;
    int controller;
    int value;
    int order;
};

struct TempoEvent
{
    int tick;
    double bpm;
};

struct MidiParseStats
{
    int format;
    int numTracks;
    int unsupportedEvents;
};

bool LoadMidiBasic(const std::string& path, int targetChannel,
    std::vector<MidiEventTick>& outEvents, std::vector<TempoEvent>& tempoEvents,
    int& ticksPerQuarter, MidiParseStats& outStats);