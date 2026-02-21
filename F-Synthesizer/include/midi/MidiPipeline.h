#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "MIDIParser.h"
#include "Sequencer.h"

struct MidiBuildOutput
{
    std::vector<MIDIEventTick> ticks;
    std::vector<TempoEvent> tempoEvents;
    std::vector<MIDIEvent> events;
    int ticksPerQuarter = 0;
    MIDIParseStatus stats{};
};

bool BuildMidiPipeline(
    const std::filesystem::path& midiPath,
    int targetChannel,
    int sampleRate,
    WaveType defaultWave,
    double startSec,
    double durationSec,
    MidiBuildOutput& out,
    std::string& err);
