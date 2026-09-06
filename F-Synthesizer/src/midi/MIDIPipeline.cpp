#include "midi/MIDIPipeline.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <vector>

namespace
{
std::vector<MIDIEvent> BuildWindowedEvents(
    const std::vector<MIDIEvent>& events,
    int startSample,
    int endSample)
{
    std::vector<MIDIEvent> prefix;
    std::vector<MIDIEvent> active;
    std::array<bool, 16> sustain{};
    std::vector<int> releasedIds;
    std::vector<MIDIEvent> result;
    for (const auto& event : events)
    {
        if (event.sample > endSample) break;
        if (event.sample >= startSample)
        {
            auto shifted = event; shifted.sample -= startSample;
            result.push_back(shifted);
            continue;
        }
        const int ch = std::clamp(event.channel, 0, 15);
        if (event.type != MIDIEventType::Note)
        {
            // Preserve controller order, including RPN selection/data and sustain.
            auto controller = event; controller.sample = 0; prefix.push_back(controller);
            if (event.type == MIDIEventType::ControlChange && event.controller == 64)
            {
                sustain[ch] = event.value >= 64;
                if (!sustain[ch])
                    std::erase_if(active, [&](const auto& note) {
                        return note.channel == ch && std::find(releasedIds.begin(), releasedIds.end(), note.noteInstanceID) != releasedIds.end();
                    });
            }
            continue;
        }
        if (event.isNoteOn) active.push_back(event);
        else
        {
            releasedIds.push_back(event.noteInstanceID);
            if (!sustain[ch]) std::erase_if(active, [&](const auto& note) { return note.noteInstanceID == event.noteInstanceID; });
        }
    }
    for (auto note : active)
    {
        note.sample = 0; prefix.push_back(note);
        if (std::find(releasedIds.begin(), releasedIds.end(), note.noteInstanceID) != releasedIds.end())
        { note.isNoteOn = false; prefix.push_back(note); }
    }
    prefix.insert(prefix.end(), result.begin(), result.end());
    return prefix;
}

std::vector<MIDIEventTick> ReplaceNoteTicks(
    const std::vector<MIDIEventTick>& baseTicks,
    const std::vector<MIDIEventTick>& overrideNoteTicks,
    int targetChannel)
{
    std::vector<MIDIEventTick> merged;
    merged.reserve(baseTicks.size() + overrideNoteTicks.size());

    // Note以外は元MIDIを維持し、Noteのみ編集バッファで差し替える。
    for (const auto& e : baseTicks)
    {
        if (e.type != MIDIEventType::Note)
        {
            merged.push_back(e);
        }
    }
    for (const auto& e : overrideNoteTicks)
    {
        if (e.type != MIDIEventType::Note)
        {
            continue;
        }
        if (targetChannel >= 0 && e.channel != targetChannel)
        {
            continue;
        }
        merged.push_back(e);
    }
    return merged;
}
} // namespace

bool BuildMIDIPipeline(
    const std::filesystem::path& midiPath,
    int targetChannel,
    int sampleRate,
    double startSec,
    double durationSec,
    const std::vector<MIDIEventTick>* overrideNoteTicks,
    int overrideTicksPerQuarter,
    MIDIBuildOutput& out,
    std::string& err)
{
    // 失敗時に前回の出力が残らないよう、先頭で out を初期化する。
    out = MIDIBuildOutput{};
    const bool hasOverrideNotes = (overrideNoteTicks != nullptr);
    bool loadedBaseMidi = false;
    if (!midiPath.empty())
    {
        loadedBaseMidi = LoadMIDIBasic(midiPath, targetChannel, out.ticks, out.tempoEvents, out.ticksPerQuarter, out.stats);
    }

    if (!loadedBaseMidi && !hasOverrideNotes)
    {
        err = "failed to load MIDI";
        return false;
    }
    if (!loadedBaseMidi)
    {
        // 編集バッファのみで再生できるよう、MIDI本体未読込でもパイプラインを継続する。
        out.ticks.clear();
        out.tempoEvents.clear();
        out.ticksPerQuarter = (overrideTicksPerQuarter > 0) ? overrideTicksPerQuarter : 480;
        out.stats = MIDIParseStatus{};
    }

    if (hasOverrideNotes)
    {
        if (loadedBaseMidi)
        {
            // GUI編集時は Note だけ差し替え、CC/Tempo は原MIDIを保持して再現性を維持する。
            out.ticks = ReplaceNoteTicks(out.ticks, *overrideNoteTicks, targetChannel);
        }
        else
        {
            out.ticks.reserve(overrideNoteTicks->size());
            for (const auto& e : *overrideNoteTicks)
            {
                if (e.type != MIDIEventType::Note)
                {
                    continue;
                }
                if (targetChannel >= 0 && e.channel != targetChannel)
                {
                    continue;
                }
                out.ticks.push_back(e);
            }
        }
        if (overrideTicksPerQuarter > 0)
        {
            out.ticksPerQuarter = overrideTicksPerQuarter;
        }
    }

    BuildSampleEvents(out.ticks, out.tempoEvents, out.ticksPerQuarter, sampleRate, out.events);
    if (startSec > 0.0 || durationSec >= 0.0)
    {
        // 負値入力は 0 扱いにそろえ、呼び出し側の入力ぶれをここで吸収する。
        const double normalizedStartSec = (startSec > 0.0) ? startSec : 0.0;
        const int startSample = static_cast<int>(normalizedStartSec * sampleRate);
        int endSample = (std::numeric_limits<int>::max)();
        if (durationSec >= 0.0)
        {
            const double normalizedDurationSec = (durationSec > 0.0) ? durationSec : 0.0;
            // 範囲終端のNoteOffを落とさないよう、ceil+1sampleで右端をわずかに広げる。
            const int durationSamples = static_cast<int>(std::ceil(normalizedDurationSec * sampleRate)) + 1;
            endSample = startSample + durationSamples;
        }
        out.events = BuildWindowedEvents(out.events, startSample, endSample);
    }

    const bool hasNoteEvent = std::any_of(out.events.begin(), out.events.end(), [](const MIDIEvent& e)
    {
        return e.type == MIDIEventType::Note;
    });
    if (!hasNoteEvent)
    {
        err = "no note events found";
        return false;
    }

    return true;
}
