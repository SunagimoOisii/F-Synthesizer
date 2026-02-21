#include <iostream>
#include <string>
#include <array>
#include <vector>
#include <filesystem>
#include <cmath>
#include <sstream>
#include <memory>
#include <algorithm>
#include <limits>
#include <Windows.h>

#include "AppCore.h"
#include "AudioBuffer.h"
#include "Writer.h"
#include "app/AppEntry.h"
#include "app/Cli.h"
#include "core/RenderGateway.h"
#include "io/PlatformPaths.h"
#include "midi/MidiPipeline.h"

namespace
{
std::filesystem::path GetExecutableDirectory()
{
    wchar_t modulePath[MAX_PATH] = {};
    DWORD len = GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
    if (len == 0 || len >= MAX_PATH)
    {
        return std::filesystem::path(".");
    }
    std::filesystem::path p(modulePath);
    if (p.has_parent_path())
    {
        return p.parent_path();
    }
    return std::filesystem::path(".");
}

std::filesystem::path FindProjectRootInternal()
{
    std::error_code ec;
    std::filesystem::path cur = std::filesystem::current_path(ec);
    if (ec || cur.empty())
    {
        cur = GetExecutableDirectory();
    }

    auto hasProjectMarker = [](const std::filesystem::path& dir)
    {
        std::error_code existsEc;
        return std::filesystem::exists(dir / "F-Synthesizer.vcxproj", existsEc) && !existsEc;
    };

    for (int depth = 0; depth < 8; depth++)
    {
        if (hasProjectMarker(cur))
        {
            return cur;
        }
        if (!cur.has_parent_path())
        {
            break;
        }
        cur = cur.parent_path();
    }

    // Fallback: search from executable directory as well.
    cur = GetExecutableDirectory();
    for (int depth = 0; depth < 8; depth++)
    {
        if (hasProjectMarker(cur))
        {
            return cur;
        }
        if (!cur.has_parent_path())
        {
            break;
        }
        cur = cur.parent_path();
    }

    return GetExecutableDirectory();
}

std::shared_ptr<const std::array<ChannelConfig, 16>> BuildDefaultChannelConfigs()
{
    auto makeWave = [](WaveType wave,
        double amp, double atk, double dec, double sus, double rel)
    {
        return ChannelConfig{ WaveformConfig{ wave },
            amp, atk, dec, sus, rel };
    };
    auto makeDrumKitDetail = [](const DrumKitConfig& kit,
        double amp, double atk, double dec, double sus, double rel)
    {
        return ChannelConfig{ kit, amp, atk, dec, sus, rel };
    };
    auto makeGmDrumKit = []()
    {
        auto kit = std::make_unique<DrumKitConfig>();
        for (auto& d : kit->map)
        {
            d.type = DrumType::None;
        }

        DrumConfig kick{ DrumType::Kick };
        kick.gain = 0.6;
        kick.baseFreq = 60.0;
        kick.pitchDrop = 3.0;
        kick.pitchDecaySec = 0.06;

        DrumConfig snare{ DrumType::Snare };
        snare.gain = 0.6;
        snare.toneFreq = 220.0;
        snare.toneLevel = 0.55;
        snare.noiseLevel = 0.35;
        snare.hpCut = 700.0;
        snare.lpCut = 6000.0;
        snare.toneWave = (int)WaveType::Triangle;
        snare.noiseType = (int)NoiseType::White;

        DrumConfig hat{ DrumType::Hat };
        hat.gain = 0.15;
        hat.toneFreq = 8000.0;
        hat.toneLevel = 0.2;
        hat.noiseLevel = 0.2;
        hat.hpCut = 4000.0;
        hat.lpCut = 6000.0;
        hat.toneWave = (int)WaveType::Sine;
        hat.noiseType = (int)NoiseType::White;

        kit->map[36] = kick;  // Bass Drum 1
        kit->map[38] = snare; // Acoustic Snare
        kit->map[40] = snare; // Electric Snare
        kit->map[42] = hat;   // Closed Hi-Hat
        kit->map[44] = hat;   // Pedal Hi-Hat
        kit->map[46] = hat;   // Open Hi-Hat
        kit->map[49] = hat;   // Crash Cymbal 1

        return *kit;
    };

    auto table = std::make_shared<std::array<ChannelConfig, 16>>();
    (*table)[0] = makeWave(WaveType::Saw, 0.25, 0.01, 0.15, 0.75, 0.20);
    (*table)[1] = makeWave(WaveType::Saw, 0.25, 0.01, 0.15, 0.75, 0.20);
    (*table)[2] = makeWave(WaveType::Saw, 0.25, 0.01, 0.15, 0.75, 0.20);
    (*table)[3] = makeWave(WaveType::Saw, 0.25, 0.01, 0.15, 0.75, 0.20);
    (*table)[4] = makeWave(WaveType::Saw, 0.25, 0.01, 0.15, 0.75, 0.20);
    (*table)[5] = makeWave(WaveType::Saw, 0.25, 0.01, 0.15, 0.75, 0.20);
    (*table)[6] = makeWave(WaveType::Saw, 0.25, 0.01, 0.15, 0.75, 0.20);
    (*table)[7] = makeWave(WaveType::Saw, 0.25, 0.01, 0.15, 0.75, 0.20);
    (*table)[8] = makeWave(WaveType::Saw, 0.25, 0.01, 0.15, 0.75, 0.20);
    (*table)[9] = makeDrumKitDetail(makeGmDrumKit(), 10.0, 0.001, 0.15, 0.1, 0.3);
    (*table)[10] = makeWave(WaveType::Saw, 0.25, 0.01, 0.15, 0.75, 0.20);
    (*table)[11] = makeWave(WaveType::Saw, 0.25, 0.01, 0.15, 0.75, 0.20);
    (*table)[12] = makeWave(WaveType::Saw, 0.25, 0.01, 0.15, 0.75, 0.20);
    (*table)[13] = makeWave(WaveType::Saw, 0.25, 0.01, 0.15, 0.75, 0.20);
    (*table)[14] = makeWave(WaveType::Saw, 0.25, 0.01, 0.15, 0.75, 0.20);
    (*table)[15] = makeWave(WaveType::Saw, 0.25, 0.01, 0.15, 0.75, 0.20);
    return table;
}

std::shared_ptr<const std::array<ChannelMixState, 16>> BuildDefaultChannelMixStates()
{
    auto table = std::make_shared<std::array<ChannelMixState, 16>>();
    for (int ch = 0; ch < 16; ch++)
    {
        (*table)[ch] = ChannelMixState{};
    }
    return table;
}

AppConfig BuildDefaultConfig()
{
    const std::filesystem::path projectRoot = FindProjectRootInternal();

    auto config = std::make_unique<AppConfig>();
    config->midiPath = projectRoot / "assets" / "midi" / "solstice_intro.mid";
    config->wavPath = projectRoot / "output" / "test.wav";
    config->targetChannel = -1;
    config->defaultWave = WaveType::Saw;
    config->initialSeconds = 6;
    config->bits = 16;
    config->sampleRate = 44100;
    config->extraReleaseSec = 0.3;
    config->channelConfigs = BuildDefaultChannelConfigs();
    config->channelMixStates = BuildDefaultChannelMixStates();
    return *config;
}

void LogLine(IRunObserver* observer, const std::string& line)
{
    if (observer != nullptr)
    {
        observer->OnLogLine(line);
        return;
    }
    std::cout << line << std::endl;
}
} // namespace

std::filesystem::path FindProjectRootPath()
{
    return FindProjectRootInternal();
}

AppConfig DefaultConfig()
{
    return BuildDefaultConfig();
}

RenderOptions DefaultRenderOptions()
{
    return RenderOptions{};
}

RenderOptions DefaultPreviewRenderOptions()
{
    RenderOptions opt{};
    opt.mode = RunMode::Preview;
    opt.startSec = 0.0;
    opt.durationSec = 8.0;
    opt.writeWav = false;
    opt.allowCancel = true;
    return opt;
}

int Run(const AppConfig& config, const RenderOptions& options, IRunObserver* observer, SoundData* renderedSound)
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

    // Output buffer
    SoundData sound(config.initialSeconds * config.sampleRate, config.bits, config.sampleRate);

    // Load MIDI
    std::vector<MIDIEventTick> ticks;
    std::vector<TempoEvent> tempoEvents;
    int noteCount = 0;
    int ccCount = 0;
    std::array<int, 16> noteByChannel{};
    std::array<int, 16> ccByChannel{};
    std::array<int, 128> ccByType{};
    int i = 0;
    int ch = 0;
    int c = 0;
    int midiTPQ = 0;
    MIDIParseStatus stats{};
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
    ticks = midiOut.ticks;
    tempoEvents = midiOut.tempoEvents;
    midiTPQ = midiOut.ticksPerQuarter;
    stats = midiOut.stats;

    // MIDI event stats
    for (i = 0; i < 16; i++)
    {
        noteByChannel[i] = 0;
        ccByChannel[i] = 0;
    }
    for (i = 0; i < 128; i++)
    {
        ccByType[i] = 0;
    }
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
            int c = t.controller;
            if (c < 0) c = 0;
            if (c > 127) c = 127;
            ccByType[c]++;
        }
    }

    // Print parse summary
    {
        std::ostringstream oss;
        oss << "MIDI Info: format=" << stats.format
            << ", tracks=" << stats.numTracks
            << ", TPQ=" << midiTPQ
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
    for (ch = 0; ch < 16; ch++)
    {
        noteCounts << " ch" << ch << "=" << noteByChannel[ch];
    }
    LogLine(observer, noteCounts.str());
    std::ostringstream ccCounts;
    ccCounts << "Channel CC Counts:";
    for (ch = 0; ch < 16; ch++)
    {
        ccCounts << " ch" << ch << "=" << ccByChannel[ch];
    }
    LogLine(observer, ccCounts.str());
    std::ostringstream ccTypes;
    ccTypes << "CC Types:";
    for (c = 0; c < 128; c++)
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

    // Convert tick events to sample events
    std::vector<MIDIEvent> events = std::move(midiOut.events);
    {
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
        std::vector<std::vector<int>> noteOnSamples(16 * 128);
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
                int noteIdx = e.noteNumber;
                if (noteIdx < 0) noteIdx = 0;
                if (noteIdx > 127) noteIdx = 127;
                noteOnSamples[chIdx * 128 + noteIdx].push_back(e.sample);
            }
            else if (e.type == MIDIEventType::Note && !e.isNoteOn)
            {
                int chIdx = (e.channel >= 0 && e.channel < 16) ? e.channel : 0;
                int noteIdx = e.noteNumber;
                if (noteIdx < 0) noteIdx = 0;
                if (noteIdx > 127) noteIdx = 127;
                auto& starts = noteOnSamples[chIdx * 128 + noteIdx];
                if (!starts.empty())
                {
                    int startSample = starts.front();
                    starts.erase(starts.begin());
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
                if (e.value < cc7Min) cc7Min = e.value;
                if (e.value > cc7Max) cc7Max = e.value;
            }
            if (e.type == MIDIEventType::ControlChange && e.controller == 11)
            {
                cc11Count++;
                if (e.value < cc11Min) cc11Min = e.value;
                if (e.value > cc11Max) cc11Max = e.value;
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

    if (events.empty())
    {
        LogLine(observer, "No note events found.");
        return 1;
    }

    // Channel config (default preset)
    const auto fallbackChannelConfigs = BuildDefaultChannelConfigs();
    const auto& channelConfigs = config.channelConfigs ? *config.channelConfigs : *fallbackChannelConfigs;
    const auto fallbackChannelMixStates = BuildDefaultChannelMixStates();
    const auto& channelMixStates = config.channelMixStates ? *config.channelMixStates : *fallbackChannelMixStates;

    // Resize output buffer
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

    // Render
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
    {
        double peak = 0.0;
        double sumSq = 0.0;
        int nonZero = 0;
        for (double v : sound.data)
        {
            double a = std::abs(v);
            if (a > peak) peak = a;
            sumSq += v * v;
            if (a > 1e-8) nonZero++;
        }
        double rms = sound.data.empty() ? 0.0 : std::sqrt(sumSq / (double)sound.data.size());
        std::ostringstream oss;
        oss << "[RenderStats] peak=" << peak
            << " rms=" << rms
            << " nonZero=" << nonZero
            << "/" << sound.data.size();
        LogLine(observer, oss.str());
    }

    if (renderedSound != nullptr)
    {
        *renderedSound = sound;
    }

    // Save
    if (options.writeWav)
    {
        if (std::filesystem::exists(config.wavPath))
        {
            std::error_code rmEc;
            std::filesystem::remove(config.wavPath, rmEc);
            if (rmEc)
            {
                LogLine(observer, "[SavePrep] failed to remove old file: " + rmEc.message());
            }
        }
        WavWriteError err{};
        if (!SaveWavFilePath(sound, config.wavPath, &err))
        {
            std::ostringstream cause;
            cause << err.cause
                << " code=" << err.code
                << " errno=" << err.errnoValue
                << " winerr=" << err.systemError;
            LogLine(observer, FormatPathDiagnostic("save wav", config.wavPath, cause.str(), err.hint));
            return 1;
        }
        LogLine(observer, "Saved SoundData: " + PathToUtf8(config.wavPath));
    }
    else
    {
        LogLine(observer, "Preview render completed (memory only, no WAV write).");
    }

    return 0;
}

int Run(const AppConfig& config)
{
    return Run(config, DefaultRenderOptions(), nullptr, nullptr);
}

int Run(const AppConfig& config, const RenderOptions& options)
{
    return Run(config, options, nullptr, nullptr);
}

int Run(const AppConfig& config, IRunObserver* observer)
{
    return Run(config, DefaultRenderOptions(), observer, nullptr);
}

int Run(const AppConfig& config, const RenderOptions& options, IRunObserver* observer)
{
    return Run(config, options, observer, nullptr);
}

int RunGuiApp();

int main(int argc, char** argv)
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    CliOptions cli{};
    if (!ParseCliArguments(argc, argv, cli))
    {
        return 1;
    }
    if (cli.showHelp)
    {
        return 0;
    }
    if (!cli.startCli)
    {
        return RunGuiApp();
    }

    return RunCliApplication(cli);
}

