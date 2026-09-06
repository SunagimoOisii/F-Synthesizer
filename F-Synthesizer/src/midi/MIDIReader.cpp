#include "midi/MIDIReader.h"

#include <fstream>
#include <map>
#include "MidiFile.h"

MIDIRawOutput ParseSMFFile(const std::filesystem::path& path, int targetChannel)
{
    MIDIRawOutput result{};
    try
    {
        std::ifstream input(path, std::ios::binary);
        unsigned char header[14]{};
        input.read(reinterpret_cast<char*>(header), sizeof(header));
        if (!input || std::string(reinterpret_cast<char*>(header), 4) != "MThd") return result;
        result.stats.format = (header[8] << 8) | header[9];
        if (result.stats.format > 1 || (header[12] & 0x80)) return result;
        input.seekg(0);
        smf::MidiFile file;
        if (!file.read(input)) return result;
        result.stats.numTracks = file.getTrackCount();
        result.ticksPerQuarter = file.getTicksPerQuarterNote();
        if (result.ticksPerQuarter <= 0) return result;
        file.absoluteTicks();
        file.joinTracks();
        file.linkNotePairsFIFO();
        std::map<const smf::MidiEvent*, int> noteIds;
        int nextId = 1;
        for (int i = 0; i < file[0].size(); ++i)
        {
            const auto& event = file[0][i];
            if (event.isTempo())
            {
                const double bpm = event.getTempoBPM();
                if (bpm > 0.0) result.tempoEvents.push_back({event.tick, bpm});
                continue;
            }
            if (event.empty() || event[0] >= 0xf0) continue;
            const int channel = event.getChannel();
            if (targetChannel >= 0 && channel != targetChannel) continue;
            MIDIEventTick out{};
            out.tick = event.tick;
            out.channel = channel;
            out.order = i;
            out.noteInstanceID = -1;
            if (event.isNote())
            {
                out.type = MIDIEventType::Note;
                out.noteNumber = event.getKeyNumber();
                out.velocity = event.getVelocity();
                out.isNoteOn = event.isNoteOn();
                if (out.isNoteOn)
                {
                    out.noteInstanceID = nextId++;
                    noteIds[&event] = out.noteInstanceID;
                }
                else if (const auto* linked = event.getLinkedEvent())
                {
                    const auto found = noteIds.find(linked);
                    if (found != noteIds.end()) out.noteInstanceID = found->second;
                }
            }
            else if (event.isController())
            {
                out.type = MIDIEventType::ControlChange;
                out.controller = event[1]; out.value = event[2];
            }
            else if (event.isPitchbend())
            {
                out.type = MIDIEventType::PitchBend;
                out.value = event[1] | (event[2] << 7);
            }
            else if (event.isPressure())
            {
                out.type = MIDIEventType::ChannelPressure; out.value = event[1];
            }
            else if (event.isAftertouch())
            {
                out.type = MIDIEventType::PolyPressure;
                out.noteNumber = event[1]; out.value = event[2];
            }
            else if (event.isTimbre())
            {
                out.type = MIDIEventType::ProgramChange; out.value = event[1];
            }
            else { ++result.stats.unsupportedEvents; continue; }
            result.rawEvents.push_back(out);
        }
        result.ok = true;
    }
    catch (const std::exception&) { result.ok = false; }
    return result;
}
