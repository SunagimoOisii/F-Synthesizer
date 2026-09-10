#include "midi/MIDIParser.h"

#include "midi/MIDIReader.h"
#include <utility>

bool LoadMIDIBasic(const std::filesystem::path& path, int targetChannel,
    std::vector<MIDIEventTick>& outEvents, std::vector<TempoEvent>& tempoEvents,
    int& ticksPerQuarter, MIDIParseStatus& outStats, std::vector<TimeSignatureEvent>* timeSignatures)
{
    MIDIRawOutput raw = ParseSMFFile(path, targetChannel);
    if (!raw.ok) return false;
    outEvents = std::move(raw.rawEvents);
    tempoEvents = std::move(raw.tempoEvents);
    ticksPerQuarter = raw.ticksPerQuarter;
    outStats = raw.stats;
    if (timeSignatures) *timeSignatures = std::move(raw.timeSignatures);
    return true;
}
