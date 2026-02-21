#include "RunInternal.h"

#include <sstream>

#include "core/RenderGateway.h"
#include "io/PlatformPaths.h"

namespace app::run
{
int RunMain(
    const AppConfig& config,
    const RenderOptions& options,
    IRunObserver* observer,
    SoundData* renderedSound)
{
    LogLine(observer, "Build Marker: 2026-02-21-save-debug-v1");
    if (options.writeWav)
    {
        std::string dirErr;
        if (!EnsureDirectoryForFile(config.wavPath, dirErr))
        {
            LogLine(observer, dirErr);
            return 1;
        }
    }

    LogLine(observer, "Working Directory: " + PathToUtf8(std::filesystem::current_path()));
    LogLine(observer, "MIDI Path: " + PathToUtf8(config.midiPath));
    LogLine(observer, "Output Path: " + PathToUtf8(config.wavPath));
    LogLine(observer, std::string("Run Mode: ") + (options.mode == RunMode::Preview ? "preview" : "export"));

    SoundData sound(config.initialSeconds * config.sampleRate, config.bits, config.sampleRate);

    MidiBuildOutput midiOut{};
    std::string midiErr;
    if (!BuildMidiPipeline(
        config.midiPath,
        config.targetChannel,
        sound.fs,
        config.defaultWave,
        options.startSec,
        options.durationSec,
        midiOut,
        midiErr))
    {
        if (midiErr == "no note events found")
        {
            LogLine(observer, "No note events found.");
        }
        else
        {
            LogLine(observer, "Failed to load MIDI: " + PathToUtf8(config.midiPath));
        }
        return 1;
    }

    LogMidiTickSummary(observer, midiOut.ticks, midiOut.tempoEvents, midiOut.ticksPerQuarter, midiOut.stats);

    std::vector<MIDIEvent> events = std::move(midiOut.events);
    LogSampleEventSummary(observer, events);

    if (events.empty())
    {
        LogLine(observer, "No note events found.");
        return 1;
    }

    const auto fallbackChannelConfigs = BuildDefaultChannelConfigs();
    const auto& channelConfigs = config.channelConfigs ? *config.channelConfigs : *fallbackChannelConfigs;
    const auto fallbackChannelMixStates = BuildDefaultChannelMixStates();
    const auto& channelMixStates = config.channelMixStates ? *config.channelMixStates : *fallbackChannelMixStates;

    int lastSample = events.back().sample;
    int extraRelease = (int)(config.extraReleaseSec * sound.fs);
    int neededSamples = lastSample + extraRelease + 1;
    if (options.mode == RunMode::Preview && options.durationSec >= 0.0)
    {
        const double durSec = (options.durationSec > 0.0) ? options.durationSec : 0.0;
        const int previewMax = static_cast<int>(durSec * sound.fs) + extraRelease + 1;
        if (neededSamples > previewMax)
        {
            neededSamples = previewMax;
        }
    }
    if (neededSamples > sound.length)
    {
        sound = SoundData(neededSamples, sound.bits, sound.fs);
    }
    else if (options.mode == RunMode::Preview && neededSamples > 0 && neededSamples < sound.length)
    {
        sound = SoundData(neededSamples, sound.bits, sound.fs);
    }

    {
        std::ostringstream oss;
        oss << "Events: " << events.size()
            << ", FirstSample: " << events.front().sample
            << ", LastSample: " << events.back().sample
            << ", Length: " << sound.length;
        LogLine(observer, oss.str());
    }

    bool canceled = false;
    auto shouldCancel = [&]() -> bool
    {
        if (!options.allowCancel || observer == nullptr)
        {
            return false;
        }
        return observer->ShouldCancel();
    };
    RenderWithEngine(sound, events, channelConfigs, channelMixStates, shouldCancel, &canceled);
    if (canceled)
    {
        LogLine(observer, "[Run] Canceled by request.");
        return 2;
    }

    LogRenderStats(observer, sound);

    if (renderedSound != nullptr)
    {
        *renderedSound = sound;
    }

    return SaveRunOutput(config, options, sound, observer);
}
} // namespace app::run
