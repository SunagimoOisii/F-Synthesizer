#pragma once

#include <filesystem>
#include <vector>

enum class MIDIEventType
{
    Note,
    ControlChange,
    PitchBend
};

struct MIDIEventTick
{
    MIDIEventType type;
    int tick;
    int noteNumber;
    int velocity;
    int channel;
    int controller;
    int value;
    int order;
    bool isNoteOn;
};

struct TempoEvent
{
    int tick;
    double bpm;
};

struct MIDIParseStatus
{
    int format;
    int numTracks;
    int unsupportedEvents;
};

bool LoadMIDIBasic(const std::filesystem::path& path, int targetChannel,
    std::vector<MIDIEventTick>& outEvents, std::vector<TempoEvent>& tempoEvents,
    int& ticksPerQuarter, MIDIParseStatus& outStats);
