#pragma once

#include <filesystem>
#include <vector>

#include "midi/MIDIParser.h"

struct MIDIRawOutput
{
    std::vector<MIDIEventTick> rawEvents;
    std::vector<TempoEvent> tempoEvents;
    std::vector<TimeSignatureEvent> timeSignatures;
    MIDIParseStatus stats{};
    int ticksPerQuarter = 480;
    bool ok = false;
};

MIDIRawOutput ParseSMFFile(const std::filesystem::path& path, int targetChannel);
