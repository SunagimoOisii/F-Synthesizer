#include "RunInternal.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <limits>
#include <sstream>

namespace app::run
{
void LogLine(IRunObserver* observer, const std::string& line)
{
    if (observer != nullptr)
    {
        // GUI実行時はobserver経由でUIログへ流す。
        observer->OnLogLine(line);
        return;
    }
    // CLI実行時は標準出力をログ終端とする。
    std::cout << line << std::endl;
}

void LogMidiTickSummary(
    IRunObserver* observer,
    const std::vector<MIDIEventTick>& ticks,
    const std::vector<TempoEvent>& tempoEvents,
    int ticksPerQuarter,
    const MIDIParseStatus& stats)
{
    int noteCount = 0;
    int ccCount = 0;
    std::array<int, 16> noteByChannel{};
    std::array<int, 16> ccByChannel{};
    std::array<int, 128> ccByType{};

    for (const auto& t : ticks)
    {
        int ch = (t.channel >= 0 && t.channel < 16) ? t.channel : 0;
        if (t.type == MIDIEventType::Note)
        {
            noteCount++;
            noteByChannel[ch]++;
        }
        else if (t.type == MIDIEventType::ControlChange)
        {
            ccCount++;
            ccByChannel[ch]++;
            int c = std::clamp(t.controller, 0, 127);
            ccByType[c]++;
        }
    }

    {
        std::ostringstream oss;
        oss << "MIDI Info: format=" << stats.format
            << ", tracks=" << stats.numTracks
            << ", TPQ=" << ticksPerQuarter
            << ", tempoEvents=" << tempoEvents.size();
        LogLine(observer, oss.str());
    }
    {
        std::ostringstream oss;
        oss << "Event Counts: note=" << noteCount
            << ", cc=" << ccCount
            << ", tempo=" << tempoEvents.size();
        LogLine(observer, oss.str());
    }
    std::ostringstream noteCounts;
    noteCounts << "Channel Note Counts:";
    for (int ch = 0; ch < 16; ch++)
    {
        noteCounts << " ch" << ch << "=" << noteByChannel[ch];
    }
    LogLine(observer, noteCounts.str());

    std::ostringstream ccCounts;
    ccCounts << "Channel CC Counts:";
    for (int ch = 0; ch < 16; ch++)
    {
        ccCounts << " ch" << ch << "=" << ccByChannel[ch];
    }
    LogLine(observer, ccCounts.str());

    std::ostringstream ccTypes;
    ccTypes << "CC Types:";
    for (int c = 0; c < 128; c++)
    {
        if (ccByType[c] > 0)
        {
            ccTypes << " cc" << c << "=" << ccByType[c];
        }
    }
    LogLine(observer, ccTypes.str());

    {
        std::ostringstream oss;
        oss << "Unsupported Events: " << stats.unsupportedEvents;
        LogLine(observer, oss.str());
    }
}

void LogSampleEventSummary(IRunObserver* observer, const std::vector<MIDIEvent>& events)
{
    constexpr size_t kNoteSlots = 16 * 128;
    int noteOnCount = 0;
    int firstNoteOnSample = -1;
    int firstNoteOnVelocity = -1;
    int firstNoteOnChannel = -1;
    int firstNoteOnNote = -1;
    int cc7Count = 0;
    int cc11Count = 0;
    int cc7Min = 127;
    int cc7Max = 0;
    int cc11Min = 127;
    int cc11Max = 0;
    int pairedNotes = 0;
    int positiveLengthNotes = 0;
    int zeroOrNegativeLengthNotes = 0;
    int minNoteLength = 0;
    int maxNoteLength = 0;
    // (ch,note) ごとに追記専用キューを持ち、先頭インデックスだけ進める。
    // 密なMIDIでの erase(begin) の O(n) コストを避けるため。
    std::vector<std::vector<int>> noteOnSamples(kNoteSlots);
    std::array<size_t, kNoteSlots> noteOnHeads{};

    for (const auto& e : events)
    {
        if (e.type == MIDIEventType::Note && e.isNoteOn)
        {
            noteOnCount++;
            if (firstNoteOnSample < 0)
            {
                firstNoteOnSample = e.sample;
                firstNoteOnVelocity = e.velocity;
                firstNoteOnChannel = e.channel;
                firstNoteOnNote = e.noteNumber;
            }
            int chIdx = (e.channel >= 0 && e.channel < 16) ? e.channel : 0;
            int noteIdx = std::clamp(e.noteNumber, 0, 127);
            noteOnSamples[chIdx * 128 + noteIdx].push_back(e.sample);
        }
        else if (e.type == MIDIEventType::Note && !e.isNoteOn)
        {
            int chIdx = (e.channel >= 0 && e.channel < 16) ? e.channel : 0;
            int noteIdx = std::clamp(e.noteNumber, 0, 127);
            const size_t slot = static_cast<size_t>(chIdx * 128 + noteIdx);
            auto& starts = noteOnSamples[slot];
            size_t& head = noteOnHeads[slot];
            if (head < starts.size())
            {
                int startSample = starts[head++];
                int len = e.sample - startSample;
                pairedNotes++;
                if (len <= 0)
                {
                    zeroOrNegativeLengthNotes++;
                }
                else
                {
                    positiveLengthNotes++;
                    if (positiveLengthNotes == 1 || len < minNoteLength) minNoteLength = len;
                    if (positiveLengthNotes == 1 || len > maxNoteLength) maxNoteLength = len;
                }
            }
        }
        if (e.type == MIDIEventType::ControlChange && e.controller == 7)
        {
            cc7Count++;
            cc7Min = (std::min)(cc7Min, e.value);
            cc7Max = (std::max)(cc7Max, e.value);
        }
        if (e.type == MIDIEventType::ControlChange && e.controller == 11)
        {
            cc11Count++;
            cc11Min = (std::min)(cc11Min, e.value);
            cc11Max = (std::max)(cc11Max, e.value);
        }
    }

    std::ostringstream eventStats;
    eventStats << "[EventStats] noteOn=" << noteOnCount
        << " cc7=" << cc7Count
        << " cc11=" << cc11Count;
    if (cc7Count > 0)
    {
        eventStats << " cc7Range=[" << cc7Min << "," << cc7Max << "]";
    }
    if (cc11Count > 0)
    {
        eventStats << " cc11Range=[" << cc11Min << "," << cc11Max << "]";
    }
    eventStats << " noteLen(paired=" << pairedNotes
        << ",positive=" << positiveLengthNotes
        << ",zeroOrNeg=" << zeroOrNegativeLengthNotes;
    if (positiveLengthNotes > 0)
    {
        eventStats << ",min=" << minNoteLength
            << ",max=" << maxNoteLength;
    }
    eventStats << ")";
    if (firstNoteOnSample >= 0)
    {
        eventStats << " firstNoteOn(sample=" << firstNoteOnSample
            << ",ch=" << firstNoteOnChannel
            << ",note=" << firstNoteOnNote
            << ",vel=" << firstNoteOnVelocity
            << ")";
    }
    LogLine(observer, eventStats.str());

    for (size_t i = 0; i < events.size() && i < 8; i++)
    {
        const auto& e = events[i];
        std::ostringstream head;
        head << "[EventHead] i=" << i
            << " sample=" << e.sample
            << " type=" << (int)e.type
            << " noteOn=" << (e.isNoteOn ? 1 : 0)
            << " ch=" << e.channel
            << " note=" << e.noteNumber
            << " vel=" << e.velocity
            << " cc=" << e.controller
            << " val=" << e.value;
        LogLine(observer, head.str());
    }
}

void LogRenderStats(IRunObserver* observer, const SoundData& sound)
{
    double peak = 0.0;
    double sumSq = 0.0;
    int nonZero = 0;
    for (double v : sound.data)
    {
        double a = std::abs(v);
        peak = (std::max)(peak, a);
        sumSq += v * v;
        if (a > 1e-8)
        {
            nonZero++;
        }
    }
    double rms = sound.data.empty() ? 0.0 : std::sqrt(sumSq / (double)sound.data.size());
    std::ostringstream oss;
    oss << "[RenderStats] peak=" << peak
        << " rms=" << rms
        << " nonZero=" << nonZero
        << "/" << sound.data.size();
    LogLine(observer, oss.str());
}
} // namespace app::run
