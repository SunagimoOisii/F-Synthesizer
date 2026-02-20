#include "MIDIParser.h"

#include <cstdint>
#include <fstream>

namespace {
uint32_t ReadBE32(std::ifstream& in)
{
    unsigned char b[4]{};
    in.read(reinterpret_cast<char*>(b), 4);
    return ((uint32_t)b[0] << 24) | ((uint32_t)b[1] << 16) | ((uint32_t)b[2] << 8) | (uint32_t)b[3];
}

uint16_t ReadBE16(std::ifstream& in)
{
    unsigned char b[2]{};
    in.read(reinterpret_cast<char*>(b), 2);
    return (uint16_t)((b[0] << 8) | b[1]);
}

uint32_t ReadVarLen(const std::vector<unsigned char>& data, size_t& idx)
{
    uint32_t value = 0;
    for (int i = 0; i < 4 && idx < data.size(); i++)
    {
        unsigned char c = data[idx++];
        value = (value << 7) | (c & 0x7F);
        if ((c & 0x80) == 0) break;
    }
    return value;
}

struct MIDIHeader
{
    uint16_t format = 0;
    uint16_t numTracks = 0;
    uint16_t division = 0;
};

bool ReadChunkId(std::ifstream& in, const char* expected)
{
    char chunkId[4]{};
    in.read(chunkId, 4);
    if (!in)
    {
        return false;
    }
    return std::string(chunkId, 4) == expected;
}

bool ReadHeader(std::ifstream& in, MIDIHeader& header, MIDIParseStatus& stats)
{
    if (!ReadChunkId(in, "MThd")) return false;

    uint32_t headerSize = ReadBE32(in);
    header.format = ReadBE16(in);
    header.numTracks = ReadBE16(in);
    header.division = ReadBE16(in);
    if (headerSize > 6)
    {
        in.seekg(headerSize - 6, std::ios::cur);
    }
    if (header.format != 0 && header.format != 1) return false;
    if (header.numTracks < 1) return false;
    if (header.division & 0x8000) return false; //SMPTEには未対応

    stats.format = header.format;
    stats.numTracks = header.numTracks;
    stats.unsupportedEvents = 0;
    return true;
}

bool ReadTrackChunk(std::ifstream& in, std::vector<unsigned char>& data)
{
    if (!ReadChunkId(in, "MTrk")) return false;
    uint32_t trackSize = ReadBE32(in);
    data.resize(trackSize);
    in.read(reinterpret_cast<char*>(data.data()), trackSize);
    return static_cast<bool>(in);
}

void AppendNoteEvent(std::vector<MIDIEventTick>& outEvents,
    uint32_t currentTick,
    int note,
    int velocity,
    unsigned char channel,
    int& eventOrder,
    bool isNoteOn)
{
    MIDIEventTick e{};
    e.tick = (int)currentTick;
    e.type = MIDIEventType::Note;
    e.isNoteOn = isNoteOn;
    e.noteNumber = note;
    e.velocity = velocity;
    e.channel = channel;
    e.controller = 0;
    e.value = 0;
    e.order = eventOrder++;
    outEvents.push_back(e);
}

void AppendControlChangeEvent(std::vector<MIDIEventTick>& outEvents,
    uint32_t currentTick,
    int controller,
    int value,
    unsigned char channel,
    int& eventOrder)
{
    MIDIEventTick e{};
    e.tick = (int)currentTick;
    e.type = MIDIEventType::ControlChange;
    e.isNoteOn = false;
    e.noteNumber = 0;
    e.velocity = 0;
    e.channel = channel;
    e.controller = controller;
    e.value = value;
    e.order = eventOrder++;
    outEvents.push_back(e);
}

void AppendPitchBendEvent(std::vector<MIDIEventTick>& outEvents,
    uint32_t currentTick,
    int value,
    unsigned char channel,
    int& eventOrder)
{
    MIDIEventTick e{};
    e.tick = (int)currentTick;
    e.type = MIDIEventType::PitchBend;
    e.isNoteOn = false;
    e.noteNumber = 0;
    e.velocity = 0;
    e.channel = channel;
    e.controller = 0;
    e.value = value;
    e.order = eventOrder++;
    outEvents.push_back(e);
}

bool ParseMetaEvent(const std::vector<unsigned char>& data,
    size_t& idx,
    uint32_t currentTick,
    std::vector<TempoEvent>& tempoEvents,
    MIDIParseStatus& stats,
    bool& endOfTrack)
{
    if (idx >= data.size()) return false;
    unsigned char type = data[idx++];
    uint32_t len = ReadVarLen(data, idx);

    if (type == 0x2F)
    {
        idx += len;
        endOfTrack = true;
        return true;
    }

    if (type == 0x51 && len == 3 && idx + 2 < data.size())
    {
        uint32_t tempo = (data[idx] << 16) | (data[idx + 1] << 8) | data[idx + 2];
        if (tempo > 0)
        {
            TempoEvent te{};
            te.tick = (int)currentTick;
            te.bpm = 60000000.0 / tempo;
            tempoEvents.push_back(te);
        }
    }
    else
    {
        stats.unsupportedEvents++;
    }
    idx += len;
    return true;
}

bool ParseSysExEvent(const std::vector<unsigned char>& data, size_t& idx, MIDIParseStatus& stats)
{
    uint32_t len = ReadVarLen(data, idx);
    idx += len;
    stats.unsupportedEvents++;
    return true;
}

bool ParseChannelEvent(unsigned char type,
    unsigned char ch,
    const std::vector<unsigned char>& data,
    size_t& idx,
    int targetChannel,
    uint32_t currentTick,
    std::vector<MIDIEventTick>& outEvents,
    int& eventOrder,
    MIDIParseStatus& stats)
{
    if (type == 0x80 || type == 0x90)
    {
        if (idx + 1 >= data.size()) return false;
        int note = data[idx++];
        int vel = data[idx++];
        if (targetChannel < 0 || ch == targetChannel)
        {
            if (type == 0x90 && vel > 0)
            {
                AppendNoteEvent(outEvents, currentTick, note, vel, ch, eventOrder, true);
            }
            else
            {
                AppendNoteEvent(outEvents, currentTick, note, 0, ch, eventOrder, false);
            }
        }
        return true;
    }

    if (type == 0xB0)
    {
        if (idx + 1 >= data.size()) return false;
        int controller = data[idx++];
        int value = data[idx++];
        if (targetChannel < 0 || ch == targetChannel)
        {
            AppendControlChangeEvent(outEvents, currentTick, controller, value, ch, eventOrder);
        }
        return true;
    }

    if (type == 0xE0)
    {
        if (idx + 1 >= data.size()) return false;
        int lsb = data[idx++];
        int msb = data[idx++];
        int value = (msb << 7) | lsb;
        if (targetChannel < 0 || ch == targetChannel)
        {
            AppendPitchBendEvent(outEvents, currentTick, value, ch, eventOrder);
        }
        return true;
    }

    int dataLen = (type == 0xC0 || type == 0xD0) ? 1 : 2;
    idx += dataLen;
    if (targetChannel < 0 || ch == targetChannel)
    {
        stats.unsupportedEvents++;
    }
    return true;
}
}

void ParseTrack(const std::vector<unsigned char>& data, int targetChannel,
    std::vector<MIDIEventTick>& outEvents, std::vector<TempoEvent>& tempoEvents,
    MIDIParseStatus& stats)
{
    size_t idx = 0;
    uint32_t currentTick = 0;
    unsigned char runningStatus = 0;
    int eventOrder = 0;

    while (idx < data.size())
    {
        uint32_t delta = ReadVarLen(data, idx);
        currentTick += delta;
        if (idx >= data.size()) break;

        // Status byte
        unsigned char status = data[idx];
        if (status & 0x80)
        {
            idx++;
            runningStatus = status;
        }
        else
        {
            status = runningStatus;
        }

        if (status == 0xFF)
        {
            bool endOfTrack = false;
            if (!ParseMetaEvent(data, idx, currentTick, tempoEvents, stats, endOfTrack)) break;
            if (endOfTrack) break;
            continue;
        }

        if (status == 0xF0 || status == 0xF7)
        {
            if (!ParseSysExEvent(data, idx, stats)) break;
            continue;
        }

        unsigned char type = status & 0xF0;
        unsigned char ch = status & 0x0F;
        if (!ParseChannelEvent(type, ch, data, idx, targetChannel, currentTick, outEvents, eventOrder, stats))
        {
            break;
        }
    }
}

bool LoadMIDIBasic(const std::filesystem::path& path, int targetChannel,
    std::vector<MIDIEventTick>& outEvents, std::vector<TempoEvent>& tempoEvents,
    int& ticksPerQuarter, MIDIParseStatus& outStats)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;

    //ヘッダー(MThd)
    MIDIHeader header{};
    if (!ReadHeader(in, header, outStats)) return false;
    ticksPerQuarter = header.division;

    //トラック(MTrk)
    for (uint16_t t = 0; t < header.numTracks; t++)
    {
        std::vector<unsigned char> data;
        if (!ReadTrackChunk(in, data)) return false;

        ParseTrack(data, targetChannel, outEvents, tempoEvents, outStats);
    }

    return !outEvents.empty();
}
