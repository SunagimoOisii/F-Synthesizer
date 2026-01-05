#include "MidiParser.h"

#include <cstdint>
#include <fstream>

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

void ParseTrack(const std::vector<unsigned char>& data,
    int targetChannel,
    std::vector<MidiEventTick>& outEvents,
    std::vector<TempoEvent>& tempoEvents,
    MidiParseStats& stats)
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

        //ステータス読み取り
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
            if (idx >= data.size()) break;
            unsigned char type = data[idx++];
            uint32_t len = ReadVarLen(data, idx);
            if (type == 0x2F)
            {
                idx += len;
                break;
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
            continue;
        }
        if (status == 0xF0 || status == 0xF7) //SysEx
        {
            uint32_t len = ReadVarLen(data, idx);
            idx += len;
            stats.unsupportedEvents++;
            continue;
        }

        unsigned char type = status & 0xF0;
        unsigned char ch = status & 0x0F;
        if (type == 0x80 || type == 0x90) //NoteOn, Off
        {
            if (idx + 1 >= data.size()) break;
            int note = data[idx++];
            int vel = data[idx++];
            if (targetChannel < 0 || ch == targetChannel)
            {
                MidiEventTick e{};
                e.tick = (int)currentTick;
                e.type = MidiEventType::Note;
                if (type == 0x90 && vel > 0)
                {
                    e.isNoteOn = true;
                    e.velocity = vel;
                }
                else
                {
                    e.isNoteOn = false;
                    e.velocity = 0;
                }
                e.noteNumber = note;
                e.channel = ch;
                e.controller = 0;
                e.value = 0;
                e.order = eventOrder++;
                outEvents.push_back(e);
            }
        }
        else if (type == 0xB0) //ControlChange
        {
            if (idx + 1 >= data.size()) break;
            int controller = data[idx++];
            int value = data[idx++];
            if (targetChannel < 0 || ch == targetChannel)
            {
                MidiEventTick e{};
                e.tick = (int)currentTick;
                e.type = MidiEventType::ControlChange;
                e.isNoteOn = false;
                e.noteNumber = 0;
                e.velocity = 0;
                e.channel = ch;
                e.controller = controller;
                e.value = value;
                e.order = eventOrder++;
                outEvents.push_back(e);
            }
        }
        else //未対応イベント
        {
            int dataLen = (type == 0xC0 || type == 0xD0) ? 1 : 2;
            idx += dataLen;
            if (targetChannel < 0 || ch == targetChannel)
            {
                stats.unsupportedEvents++;
            }
        }
    }
}

bool LoadMidiBasic(const std::string& path,
    int targetChannel,
    std::vector<MidiEventTick>& outEvents,
    std::vector<TempoEvent>& tempoEvents,
    int& ticksPerQuarter,
    MidiParseStats& outStats)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) return false;

    //ヘッダー(MThd)
    char chunkId[4]{};
    in.read(chunkId, 4);
    if (std::string(chunkId, 4) != "MThd") return false;

    uint32_t headerSize = ReadBE32(in);
    uint16_t format = ReadBE16(in);
    uint16_t ntrks = ReadBE16(in);
    uint16_t division = ReadBE16(in);
    if (headerSize > 6)
    {
        in.seekg(headerSize - 6, std::ios::cur);
    }
    if (format != 0 && format != 1) return false;
    if (ntrks < 1) return false;
    if (division & 0x8000) return false; //SMPTEには未対応
    ticksPerQuarter = division;
    outStats.format = format;
    outStats.numTracks = ntrks;
    outStats.unsupportedEvents = 0;

    //トラック(MTrk)
    for (uint16_t t = 0; t < ntrks; t++)
    {
        in.read(chunkId, 4);
        if (std::string(chunkId, 4) != "MTrk") return false;
        uint32_t trackSize = ReadBE32(in);

        std::vector<unsigned char> data(trackSize);
        in.read(reinterpret_cast<char*>(data.data()), trackSize);
        if (!in) return false;

        ParseTrack(data, targetChannel, outEvents, tempoEvents, outStats);
    }

    return !outEvents.empty();
}

