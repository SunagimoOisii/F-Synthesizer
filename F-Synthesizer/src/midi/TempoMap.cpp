#include "midi/TempoMap.h"
#include <algorithm>
#include <cmath>

namespace midi
{
TempoMap::TempoMap(const std::vector<TempoEvent> &events, int ticksPerQuarter)
{
    const int tpq = std::max(1, ticksPerQuarter);
    segments_.push_back({0, 0.0, .5 / tpq});
    auto sorted = events;
    std::stable_sort(sorted.begin(), sorted.end(), [](const auto &a, const auto &b) { return a.tick < b.tick; });
    for (const auto &event : sorted)
    {
        const int tick = std::max(0, event.tick);
        const auto &previous = segments_.back();
        const double seconds = previous.seconds + (tick - previous.tick) * previous.secondsPerTick;
        const double bpm = std::isfinite(event.bpm) && event.bpm > 0 ? event.bpm : 120;
        const Segment next{tick, seconds, 60.0 / bpm / tpq};
        if (tick == previous.tick)
            segments_.back() = next;
        else
            segments_.push_back(next);
    }
}

double TempoMap::SecondsAtTick(int tick) const
{
    tick = std::max(0, tick);
    const auto end = std::upper_bound(segments_.begin(), segments_.end(), tick,
                                      [](int value, const Segment &segment) { return value < segment.tick; });
    const auto &segment = *std::prev(end);
    return segment.seconds + (tick - segment.tick) * segment.secondsPerTick;
}

double TempoMap::TickAtSeconds(double seconds) const
{
    seconds = std::max(0.0, seconds);
    const auto end = std::upper_bound(segments_.begin(), segments_.end(), seconds,
                                      [](double value, const Segment &segment) { return value < segment.seconds; });
    const auto &segment = *std::prev(end);
    return segment.tick + (seconds - segment.seconds) / segment.secondsPerTick;
}
} // namespace midi
