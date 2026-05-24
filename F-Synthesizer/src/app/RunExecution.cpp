#include "RunInternal.h"

#include <sstream>

#include "core/RenderGateway.h"
#include "io/PlatformPaths.h"

namespace app::run
{
namespace
{
int RunRenderCommon(
    const AppConfig& config,
    const RenderOptions& options,
    IRunObserver* observer,
    SoundData* renderedSound,
    bool saveOutput)
{
    const bool previewMode = (options.mode == RunMode::Preview);

    if (options.writeWAV)
    {
        std::string dirErr;
        if (!EnsureDirectoryForFile(config.wavPath, dirErr))
        {
            LogLine(observer, dirErr);
            return 1;
        }
    }

    std::error_code cwdEc;
    const std::filesystem::path cwd = std::filesystem::current_path(cwdEc);
    if (cwdEc)
    {
        LogLine(observer, "[IO] op=current working directory cause=\"" + cwdEc.message() + "\"");
    }
    else
    {
        LogLine(observer, "Working Directory: " + PathToUtf8(cwd));
    }
    LogLine(observer, "MIDI Path: " + PathToUtf8(config.midiPath));
    LogLine(observer, "Output Path: " + PathToUtf8(config.wavPath));
    LogLine(observer, std::string("Run Mode: ") + (previewMode ? "preview" : "export"));

    // app層の責務として、MIDI読込〜sampleイベント化まではここで完結させる。
    MIDIBuildOutput midiOut{};
    std::string midiErr;
    if (!BuildMIDIPipeline(
        config.midiPath,
        config.targetChannel,
        config.sampleRate,
        options.startSec,
        options.durationSec,
        config.overrideNoteTicks.get(),
        config.overrideTicksPerQuarter,
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

    LogMIDITickSummary(observer, midiOut.ticks, midiOut.tempoEvents, midiOut.ticksPerQuarter, midiOut.stats);

    std::vector<MIDIEvent> events = std::move(midiOut.events);
    LogSampleEventSummary(observer, events);

    if (events.empty())
    {
        if (midiErr.empty())
        {
            LogLine(observer, "No note events found.");
        }
        return 1;
    }

    const auto* channelConfigs = config.channelConfigs.get();
    const auto* channelMixStates = config.channelMixStates.get();
    if (channelConfigs == nullptr)
    {
        // 互換維持: 不完全な設定入力でも既定テーブルで実行可能にする。
        channelConfigs = BuildDefaultChannelConfigs().get();
    }
    if (channelMixStates == nullptr)
    {
        channelMixStates = BuildDefaultChannelMixStates().get();
    }

    int lastSample = events.back().sample;
    int extraRelease = static_cast<int>(config.extraReleaseSec * config.sampleRate);
    int neededSamples = lastSample + extraRelease + 1;
    if (previewMode && options.durationSec >= 0.0)
    {
        // Previewは指定window内に収め、Exportの全長確保方針と分離する。
        const double durSec = (options.durationSec > 0.0) ? options.durationSec : 0.0;
        const int previewMax = static_cast<int>(durSec * config.sampleRate) + extraRelease + 1;
        if (neededSamples > previewMax)
        {
            neededSamples = previewMax;
        }
    }
    int soundLength = config.initialSeconds * config.sampleRate;
    if (neededSamples > soundLength)
    {
        soundLength = neededSamples;
    }
    else if (previewMode && neededSamples > 0 && neededSamples < soundLength)
    {
        soundLength = neededSamples;
    }
    SoundData sound(soundLength, config.bits, config.sampleRate, 2);

    {
        std::ostringstream oss;
        oss << "Events: " << events.size()
            << ", FirstSample: " << events.front().sample
            << ", LastSample: " << events.back().sample
            << ", Length: " << sound.length;
        LogLine(observer, oss.str());
    }

    const RenderConfig renderConfig{
        events,
        midiOut.tempoEvents,
        midiOut.ticksPerQuarter,
        options.startSec,
        *channelConfigs,
        *channelMixStates,
        config.masterEffects
    };

    bool canceled = false;
    // レンダループ内の分岐を減らすため、キャンセル有無で経路を先に分ける。
    const bool canCancel = options.allowCancel && observer != nullptr;
    if (canCancel)
    {
        auto shouldCancelObserver = [&]() -> bool { return observer->ShouldCancel(); };
        RenderWithEngine(
            sound,
            renderConfig,
            shouldCancelObserver,
            &canceled);
    }
    else
    {
        auto neverCancel = []() -> bool { return false; };
        RenderWithEngine(
            sound,
            renderConfig,
            neverCancel,
            &canceled);
    }
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

    if (!saveOutput)
    {
        LogLine(observer, "Preview render completed (memory only, no WAV write).");
        return 0;
    }
    return SaveRunOutput(config, options, sound, observer);
}
} // namespace

int RunExportRender(
    const AppConfig& config,
    const RenderOptions& options,
    IRunObserver* observer,
    SoundData* renderedSound)
{
    return RunRenderCommon(config, options, observer, renderedSound, true);
}

int RunPreviewRender(
    const AppConfig& config,
    const RenderOptions& options,
    IRunObserver* observer,
    SoundData* renderedSound)
{
    RenderOptions previewOptions = options;
    previewOptions.mode = RunMode::Preview;
    previewOptions.writeWAV = false;
    return RunRenderCommon(config, previewOptions, observer, renderedSound, false);
}

int RunMain(
    const AppConfig& config,
    const RenderOptions& options,
    IRunObserver* observer,
    SoundData* renderedSound)
{
    if (options.mode == RunMode::Preview)
    {
        return RunPreviewRender(config, options, observer, renderedSound);
    }
    return RunExportRender(config, options, observer, renderedSound);
}
} // namespace app::run

