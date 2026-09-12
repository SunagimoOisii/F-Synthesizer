#pragma once
#include "midi/MIDIParser.h"
#include <vector>

namespace midi
{
// UI timeline conversion. Fractional ticks let each caller choose its rounding.
// Events are sorted once; a missing initial tempo uses 120 BPM.
class TempoMap
{
  public:
    TempoMap(const std::vector<TempoEvent> &events, int ticksPerQuarter);
    double SecondsAtTick(int tick) const;
    double TickAtSeconds(double seconds) const;

  private:
    struct Segment
    {
        int tick;
        double seconds;
        double secondsPerTick;
    };
    std::vector<Segment> segments_;
};
} // namespace midi
