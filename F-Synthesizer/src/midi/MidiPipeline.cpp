#include "midi/MidiPipeline.h"

#include <array>
#include <limits>
#include <vector>

namespace
{
std::vector<MIDIEvent> BuildWindowedEvents(
    const std::vector<MIDIEvent>& events,
    int startSample,
    int endSample)
{
    // 目的: 部分レンダでも CC/Pitch の直前状態を失わないよう、window 開始時点の状態を補う。
    // 前提: イベント列は sample 昇順に整列済み。
    std::array<bool, 16> hasCc7{};
    std::array<bool, 16> hasCc11{};
    std::array<bool, 16> hasPitch{};
    std::array<int, 16> cc7{};
    std::array<int, 16> cc11{};
    std::array<int, 16> pitch{};
    std::vector<MIDIEvent> out;
    out.reserve(events.size());

    for (const auto& e : events)
    {
        if (e.sample < startSample)
        {
            const int ch = (e.channel >= 0 && e.channel < 16) ? e.channel : 0;
            if (e.type == MIDIEventType::ControlChange && e.controller == 7)
            {
                hasCc7[ch] = true;
                cc7[ch] = e.value;
            }
            else if (e.type == MIDIEventType::ControlChange && e.controller == 11)
            {
                hasCc11[ch] = true;
                cc11[ch] = e.value;
            }
            else if (e.type == MIDIEventType::PitchBend)
            {
                hasPitch[ch] = true;
                pitch[ch] = e.value;
            }
            continue;
        }
        if (e.sample > endSample)
        {
            break;
        }

        MIDIEvent shifted = e;
        shifted.sample -= startSample;
        out.push_back(shifted);
    }

    std::vector<MIDIEvent> prefix;
    for (int ch = 0; ch < 16; ch++)
    {
        if (hasCc7[ch])
        {
            MIDIEvent e{};
            e.sample = 0;
            e.type = MIDIEventType::ControlChange;
            e.channel = ch;
            e.controller = 7;
            e.value = cc7[ch];
            prefix.push_back(e);
        }
        if (hasCc11[ch])
        {
            MIDIEvent e{};
            e.sample = 0;
            e.type = MIDIEventType::ControlChange;
            e.channel = ch;
            e.controller = 11;
            e.value = cc11[ch];
            prefix.push_back(e);
        }
        if (hasPitch[ch])
        {
            MIDIEvent e{};
            e.sample = 0;
            e.type = MIDIEventType::PitchBend;
            e.channel = ch;
            e.value = pitch[ch];
            prefix.push_back(e);
        }
    }

    prefix.insert(prefix.end(), out.begin(), out.end());
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

bool BuildMidiPipeline(
    const std::filesystem::path& midiPath,
    int targetChannel,
    int sampleRate,
    WaveType defaultWave,
    double startSec,
    double durationSec,
    const std::vector<MIDIEventTick>* overrideNoteTicks,
    int overrideTicksPerQuarter,
    MidiBuildOutput& out,
    std::string& err)
{
    out = MidiBuildOutput{};
    if (!LoadMIDIBasic(midiPath, targetChannel, out.ticks, out.tempoEvents, out.ticksPerQuarter, out.stats))
    {
        err = "failed to load MIDI";
        return false;
    }

    if (overrideNoteTicks != nullptr && !overrideNoteTicks->empty())
    {
        out.ticks = ReplaceNoteTicks(out.ticks, *overrideNoteTicks, targetChannel);
        if (overrideTicksPerQuarter > 0)
        {
            out.ticksPerQuarter = overrideTicksPerQuarter;
        }
    }

    BuildSampleEvents(out.ticks, out.tempoEvents, out.ticksPerQuarter, sampleRate, defaultWave, out.events);
    if (startSec > 0.0 || durationSec >= 0.0)
    {
        // 負値入力は 0 扱いにそろえ、呼び出し側の入力ぶれをここで吸収する。
        const double normalizedStartSec = (startSec > 0.0) ? startSec : 0.0;
        const int startSample = static_cast<int>(normalizedStartSec * sampleRate);
        int endSample = (std::numeric_limits<int>::max)();
        if (durationSec >= 0.0)
        {
            const double normalizedDurationSec = (durationSec > 0.0) ? durationSec : 0.0;
            const int durationSamples = static_cast<int>(normalizedDurationSec * sampleRate);
            endSample = startSample + durationSamples;
        }
        out.events = BuildWindowedEvents(out.events, startSample, endSample);
    }

    if (out.events.empty())
    {
        err = "no note events found";
        return false;
    }

    return true;
}
