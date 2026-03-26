#pragma once

#include <filesystem>
#include <vector>

enum class MIDIEventType
{
    Note,
    ControlChange,
    PitchBend,
    ChannelPressure,
    PolyPressure
};

struct MIDIEventTick
{
    // tick軸の生イベント表現（sample変換前）。
    MIDIEventType type;
    int tick;
    int noteNumber;
    int velocity;
    int channel;
    int controller;
    int value;
    // NoteOn/NoteOff 対応付け用ID。Note以外は -1。
    int noteInstanceID = -1;
    int order;
    bool isNoteOn;
};

struct TempoEvent
{
    // tick時点で有効になるテンポ。
    int tick;
    double bpm;
};

struct MIDIParseStatus
{
    // パース時のメタ情報（診断/ログ向け）。
    int format;
    int numTracks;
    int unsupportedEvents;
};

bool LoadMIDIBasic(const std::filesystem::path& path, int targetChannel,
    std::vector<MIDIEventTick>& outEvents, std::vector<TempoEvent>& tempoEvents,
    int& ticksPerQuarter, MIDIParseStatus& outStats);
