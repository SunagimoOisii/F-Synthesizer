#include "gui/GUIActions.h"
#include <algorithm>
#include <cmath>
#include "midi/TempoMap.h"

namespace gui
{
std::vector<int> SongBarTicks(const GUIState& state)
{
    std::vector<int> bars;
    int tick = 0, numerator = 4, denominator = 4;
    size_t meter = 0;
    const auto& signatures = state.pianoRoll.timeSignatures;
    do
    {
        bars.push_back(tick);
        while (meter < signatures.size() && signatures[meter].tick <= tick)
        { numerator = signatures[meter].numerator; denominator = signatures[meter].denominator; ++meter; }
        const int length = std::max(1, state.pianoRoll.ticksPerQuarter * 4 * numerator / std::max(1, denominator));
        int next = tick + length;
        if (meter < signatures.size() && signatures[meter].tick < next) next = signatures[meter].tick;
        tick = next;
    } while (tick < state.pianoRoll.maxTick && bars.size() < 100000);
    bars.push_back(tick);
    return bars;
}
double SongSecondsAtTick(const GUIState& state, int tick)
{
    return midi::TempoMap(state.pianoRoll.tempoEvents, state.pianoRoll.ticksPerQuarter).SecondsAtTick(tick);
}
int SongTickAtSeconds(const GUIState& state, double seconds)
{
    return static_cast<int>(std::lround(midi::TempoMap(state.pianoRoll.tempoEvents,
        state.pianoRoll.ticksPerQuarter).TickAtSeconds(seconds)));
}
bool SongIsPlaying(const GUIState& state)
{
    return !state.toneAuditionActive && !state.stopRequested.load(std::memory_order_relaxed) &&
        ((state.running && state.runIsPreview) || (state.playback.streamMode.load() && state.playback.playing.load()));
}
void PauseSongPlayback(GUIState& state)
{
    if (SongIsPlaying(state) && state.playback.streamMode.load() && state.playback.playing.load())
        state.songCursorTick = SongTickAtSeconds(state, SongSecondsAtTick(state, state.playback.playStartTick.load()) +
            double(state.playback.frameCursor.load()) / std::max(1u, state.playback.sampleRate));
    state.transportAction = gui::TransportAction::None; state.resumeAfterAudition = false;
    StopGUIRunAndPreview(state);
}
void RequestSongPlayback(GUIState& state)
{
    if (state.running && !state.runIsPreview) return;
    state.resumeAfterAudition = false;
    if (state.songCursorTick >= state.pianoRoll.maxTick) state.songCursorTick = 0;
    if (state.pianoRoll.previewRangeEnabled && (state.songCursorTick < state.pianoRoll.previewRangeStartTick ||
        state.songCursorTick >= state.pianoRoll.previewRangeEndTick)) state.songCursorTick = state.pianoRoll.previewRangeStartTick;
    state.transportAction = gui::TransportAction::Song;
    StopGUIRunAndPreview(state);
}
void SeekSong(GUIState& state, int tick)
{
    const bool playing = SongIsPlaying(state);
    state.songCursorTick = std::clamp(tick, 0, state.pianoRoll.maxTick);
    if (playing) RequestSongPlayback(state);
}
void RequestToneAudition(GUIState& state)
{
    if (state.toneAuditionActive || (state.running && !state.runIsPreview)) return;
    state.resumeAfterAudition = SongIsPlaying(state);
    if (state.resumeAfterAudition && state.playback.streamMode.load() && state.playback.playing.load())
        state.songCursorTick = SongTickAtSeconds(state, SongSecondsAtTick(state, state.playback.playStartTick.load()) +
            double(state.playback.frameCursor.load()) / std::max(1u, state.playback.sampleRate));
    state.resumeSongTick = state.songCursorTick;
    state.transportAction = gui::TransportAction::Audition;
    StopGUIRunAndPreview(state);
}
void UpdateGUITransport(GUIState& state)
{
    if (SongIsPlaying(state) && state.playback.streamMode.load() && state.playback.playing.load() && state.transportAction == gui::TransportAction::None)
    {
        const double absolute = SongSecondsAtTick(state, state.playback.playStartTick.load()) +
            double(state.playback.frameCursor.load()) / std::max(1u, state.playback.sampleRate);
        state.songCursorTick = std::clamp(SongTickAtSeconds(state, absolute), 0, state.pianoRoll.maxTick);
    }
    if (state.running || state.playback.playing.load()) return;
    if (state.toneAuditionActive)
    {
        state.toneAuditionActive = false;
        if (state.resumeAfterAudition && state.lastRunExitCode == 0)
        {
            state.songCursorTick = state.resumeSongTick;
            state.transportAction = gui::TransportAction::Song;
        }
        state.resumeAfterAudition = false;
    }
    const auto action = state.transportAction;
    state.transportAction = gui::TransportAction::None;
    if (action == gui::TransportAction::Song) { StartGUIRun(state, true); }
    if (action == gui::TransportAction::Audition)
    {
        state.toneAuditionActive = true;
        StartGUISoundTonePreview(state);
        if (!state.running) state.toneAuditionActive = false;
    }
}
}
