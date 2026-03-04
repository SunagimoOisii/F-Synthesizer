#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "midi/MIDIParser.h"
#include "midi/Sequencer.h"

struct MIDIBuildOutput
{
    // Parse -> tick列 -> sample列 までの中間成果物を一括で返す。
    std::vector<MIDIEventTick> ticks;
    std::vector<TempoEvent> tempoEvents;
    std::vector<MIDIEvent> events;
    int ticksPerQuarter = 0;
    MIDIParseStatus stats{};
};

bool BuildMIDIPipeline(
    const std::filesystem::path& midiPath,
    int targetChannel,
    int sampleRate,
    WaveType defaultWave,
    double startSec,
    double durationSec,
    const std::vector<MIDIEventTick>* overrideNoteTicks,
    int overrideTicksPerQuarter,
    MIDIBuildOutput& out,
    std::string& err);

