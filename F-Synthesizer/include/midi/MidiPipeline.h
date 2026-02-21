#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "MIDIParser.h"
#include "Sequencer.h"

struct MidiBuildOutput
{
    // Parse -> tick列 -> sample列 までの中間成果物を一括で返す。
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
