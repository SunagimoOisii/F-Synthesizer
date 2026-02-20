#include <iostream>
#include <string>
#include <array>
#include <vector>
#include <filesystem>
#include <cmath>
#include <fstream>
#include <optional>
#include <regex>
#include <sstream>
#include <Windows.h>

#include "AudioBuffer.h"
#include "MIDIParser.h"
#include "Sequencer.h"
#include "SynthEngine/SynthEngine.h"
#include "Writer.h"

struct AppConfig
{
    std::filesystem::path midiPath;
    std::filesystem::path wavPath;
    int targetChannel;
    WaveType defaultWave;
    int initialSeconds;
    int bits;
    int sampleRate;
    double extraReleaseSec;
    std::array<ChannelConfig, 16> channelConfigs;
};

namespace
{
std::filesystem::path FindProjectRoot()
{
    std::filesystem::path cur = std::filesystem::current_path();
    for (int depth = 0; depth < 8; depth++)
    {
        if (std::filesystem::exists(cur / "F-Synthesizer.vcxproj"))
        {
            return cur;
        }
        if (!cur.has_parent_path())
        {
            break;
        }
        cur = cur.parent_path();
    }
    return std::filesystem::current_path();
}

std::array<ChannelConfig, 16> BuildFrogThemeChannelConfigs()
{
    auto makeFm = [](WaveType carrierWave, WaveType modWave,
        double amp, double atk, double dec, double sus, double rel,
        double carrierRatio, double modRatio, double index, double outLevel)
    {
        return ChannelConfig{ FmConfig{ carrierWave, modWave, carrierRatio, modRatio, index, outLevel },
            amp, atk, dec, sus, rel };
    };
    auto makeDrumKitDetail = [](const DrumKitConfig& kit,
        double amp, double atk, double dec, double sus, double rel)
    {
        return ChannelConfig{ kit, amp, atk, dec, sus, rel };
    };
    auto makeGmDrumKit = []()
    {
        DrumKitConfig kit{};
        for (auto& d : kit.map)
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

        kit.map[36] = kick;  // Bass Drum 1
        kit.map[38] = snare; // Acoustic Snare
        kit.map[40] = snare; // Electric Snare
        kit.map[42] = hat;   // Closed Hi-Hat
        kit.map[44] = hat;   // Pedal Hi-Hat
        kit.map[46] = hat;   // Open Hi-Hat
        kit.map[49] = hat;   // Crash Cymbal 1

        return kit;
    };

    return {
        // Flute
        makeFm(WaveType::Sine, WaveType::Triangle, 0.40, 0.04, 0.22, 0.90, 0.35, 1.0, 2.01, 1.45, 0.9), // ch0
        makeFm(WaveType::Sine, WaveType::Triangle, 0.38, 0.04, 0.22, 0.90, 0.35, 1.0, 2.015, 1.45, 0.9), // ch1
        // Trumpet
        makeFm(WaveType::Saw, WaveType::Triangle, 0.45, 0.0025, 0.18, 0.68, 0.18, 1.0, 0.9965, 1.3, 1.15), // ch2
        makeFm(WaveType::Saw, WaveType::Square, 0.48, 0.0025, 0.18, 0.68, 0.18, 1.0, 0.9965, 1.3, 1.15), // ch3
        // Strings
        makeFm(WaveType::Sine, WaveType::Triangle, 0.30, 0.08, 0.40, 0.80, 0.55, 1.0, 1.01, 1.4, 1.0), // ch4
        makeFm(WaveType::Triangle, WaveType::Sine, 0.26, 0.06, 0.32, 0.72, 0.45, 1.0, 1.01, 1.4, 1.0), // ch5
        // Bass
        makeFm(WaveType::Triangle, WaveType::Sine, 0.5, 0.006, 0.14, 0.62, 0.2, 0.5, 0.9975, 1.0, 1.05), // ch6
        makeFm(WaveType::Triangle, WaveType::Sine, 0.47, 0.006, 0.14, 0.62, 0.2, 0.5, 0.9975, 1.0, 1.05), // ch7
        makeFm(WaveType::Square, WaveType::Square, 0.22, 0.01, 0.1, 0.75, 0.2, 1.0, 1.0, 0.9975, 1.0), // ch8
        // Drums
        makeDrumKitDetail(makeGmDrumKit(), 10.0, 0.001, 0.15, 0.1, 0.3), // ch9
        makeFm(WaveType::Triangle, WaveType::Triangle, 0.25, 0.02, 0.1, 0.7, 0.2, 1.0, 1.0, 0.0, 1.0), // ch10
        makeFm(WaveType::Triangle, WaveType::Triangle, 0.25, 0.02, 0.1, 0.7, 0.2, 1.0, 1.0, 0.0, 1.0), // ch11
        makeFm(WaveType::Triangle, WaveType::Triangle, 0.25, 0.02, 0.1, 0.7, 0.2, 1.0, 1.0, 0.0, 1.0), // ch12
        makeFm(WaveType::Triangle, WaveType::Triangle, 0.25, 0.02, 0.1, 0.7, 0.2, 1.0, 1.0, 0.0, 1.0), // ch13
        makeFm(WaveType::Triangle, WaveType::Triangle, 0.25, 0.02, 0.1, 0.7, 0.2, 1.0, 1.0, 0.0, 1.0), // ch14
        makeFm(WaveType::Triangle, WaveType::Triangle, 0.25, 0.02, 0.1, 0.7, 0.2, 1.0, 1.0, 0.0, 1.0)  // ch15
    };
}

AppConfig DefaultConfig()
{
    const std::filesystem::path projectRoot = FindProjectRoot();

    AppConfig config{};
    config.midiPath = projectRoot / "assets" / "midi" / "solstice_intro.mid";
    config.wavPath = projectRoot / "output" / "test.wav";
    config.targetChannel = -1;
    config.defaultWave = WaveType::Saw;
    config.initialSeconds = 6;
    config.bits = 16;
    config.sampleRate = 44100;
    config.extraReleaseSec = 0.3;
    config.channelConfigs = BuildFrogThemeChannelConfigs();
    return config;
}

std::string ReadTextFile(const std::filesystem::path& filePath)
{
    std::ifstream fin(filePath, std::ios::binary);
    if (!fin)
    {
        return "";
    }
    std::ostringstream oss;
    oss << fin.rdbuf();
    return oss.str();
}

std::optional<std::string> ReadJsonString(const std::string& text, const std::string& key)
{
    const std::regex pat("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch m;
    if (std::regex_search(text, m, pat) && m.size() >= 2)
    {
        return m[1].str();
    }
    return std::nullopt;
}

std::optional<int> ReadJsonInt(const std::string& text, const std::string& key)
{
    const std::regex pat("\"" + key + "\"\\s*:\\s*(-?\\d+)");
    std::smatch m;
    if (std::regex_search(text, m, pat) && m.size() >= 2)
    {
        return std::stoi(m[1].str());
    }
    return std::nullopt;
}

std::optional<double> ReadJsonDouble(const std::string& text, const std::string& key)
{
    const std::regex pat("\"" + key + "\"\\s*:\\s*(-?\\d+(?:\\.\\d+)?)");
    std::smatch m;
    if (std::regex_search(text, m, pat) && m.size() >= 2)
    {
        return std::stod(m[1].str());
    }
    return std::nullopt;
}

bool TryParseWaveType(const std::string& name, WaveType& outWave)
{
    if (name == "sine")
    {
        outWave = WaveType::Sine;
        return true;
    }
    if (name == "square")
    {
        outWave = WaveType::Square;
        return true;
    }
    if (name == "saw")
    {
        outWave = WaveType::Saw;
        return true;
    }
    if (name == "triangle")
    {
        outWave = WaveType::Triangle;
        return true;
    }
    return false;
}

std::filesystem::path ResolvePathFromBase(const std::filesystem::path& baseDir, const std::string& v)
{
    std::filesystem::path p(v);
    if (p.is_absolute())
    {
        return p;
    }
    return std::filesystem::weakly_canonical(baseDir / p);
}

bool LoadConfigFile(const std::filesystem::path& configPath, AppConfig& cfg, std::string& err)
{
    const std::string text = ReadTextFile(configPath);
    if (text.empty())
    {
        err = "failed to read config file";
        return false;
    }

    const std::filesystem::path baseDir = configPath.has_parent_path()
        ? configPath.parent_path()
        : std::filesystem::current_path();

    if (auto v = ReadJsonString(text, "midiPath"))
    {
        cfg.midiPath = ResolvePathFromBase(baseDir, *v);
    }
    if (auto v = ReadJsonString(text, "wavPath"))
    {
        cfg.wavPath = ResolvePathFromBase(baseDir, *v);
    }
    if (auto v = ReadJsonInt(text, "targetChannel"))
    {
        cfg.targetChannel = *v;
    }
    if (auto v = ReadJsonInt(text, "initialSeconds"))
    {
        cfg.initialSeconds = *v;
    }
    if (auto v = ReadJsonInt(text, "bits"))
    {
        cfg.bits = *v;
    }
    if (auto v = ReadJsonInt(text, "sampleRate"))
    {
        cfg.sampleRate = *v;
    }
    if (auto v = ReadJsonDouble(text, "extraReleaseSec"))
    {
        cfg.extraReleaseSec = *v;
    }
    if (auto v = ReadJsonString(text, "defaultWave"))
    {
        WaveType w{};
        if (!TryParseWaveType(*v, w))
        {
            err = "invalid defaultWave: " + *v;
            return false;
        }
        cfg.defaultWave = w;
    }

    return true;
}

bool ParseArguments(int argc, char** argv, std::filesystem::path& configPath, bool& showHelp)
{
    configPath.clear();
    showHelp = false;
    for (int i = 1; i < argc; i++)
    {
        const std::string arg = argv[i];
        if (arg == "--config")
        {
            if (i + 1 >= argc)
            {
                return false;
            }
            configPath = std::filesystem::path(argv[++i]);
        }
        else if (arg == "--help" || arg == "-h")
        {
            std::cout << "Usage: F-Synthesizer.exe [--config path/to/config.json]" << std::endl;
            showHelp = true;
            return true;
        }
    }
    return true;
}
} // namespace

int Run(const AppConfig& config)
{
    std::cout << "Build Marker: 2026-02-21-save-debug-v1" << std::endl;
    std::filesystem::create_directories(config.wavPath.parent_path());
    std::cout << "Working Directory: " << std::filesystem::current_path().string() << std::endl;
    std::cout << "MIDI Path: " << config.midiPath.string() << std::endl;
    std::cout << "Output Path: " << config.wavPath.string() << std::endl;

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
    if (!LoadMIDIBasic(config.midiPath.string(), config.targetChannel, ticks, tempoEvents, midiTPQ, stats))
    {
        std::cout << "Failed to load MIDI: " << config.midiPath.string() << std::endl;
        return 1;
    }

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
    std::cout << "MIDI Info: format=" << stats.format
        << ", tracks=" << stats.numTracks
        << ", TPQ=" << midiTPQ
        << ", tempoEvents=" << tempoEvents.size() << std::endl;
    std::cout << "Event Counts: note=" << noteCount
        << ", cc=" << ccCount
        << ", tempo=" << tempoEvents.size() << std::endl;
    std::cout << "Channel Note Counts:";
    for (ch = 0; ch < 16; ch++)
    {
        std::cout << " ch" << ch << "=" << noteByChannel[ch];
    }
    std::cout << std::endl;
    std::cout << "Channel CC Counts:";
    for (ch = 0; ch < 16; ch++)
    {
        std::cout << " ch" << ch << "=" << ccByChannel[ch];
    }
    std::cout << std::endl;
    std::cout << "CC Types:";
    for (c = 0; c < 128; c++)
    {
        if (ccByType[c] > 0)
        {
            std::cout << " cc" << c << "=" << ccByType[c];
        }
    }
    std::cout << std::endl;
    std::cout << "Unsupported Events: " << stats.unsupportedEvents << std::endl;

    // Convert tick events to sample events
    std::vector<MIDIEvent> events;
    BuildSampleEvents(ticks, tempoEvents, midiTPQ, sound.fs, config.defaultWave, events);
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
        std::array<std::vector<int>, 16 * 128> noteOnSamples;
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
        std::cout << "[EventStats] noteOn=" << noteOnCount
            << " cc7=" << cc7Count
            << " cc11=" << cc11Count;
        if (cc7Count > 0)
        {
            std::cout << " cc7Range=[" << cc7Min << "," << cc7Max << "]";
        }
        if (cc11Count > 0)
        {
            std::cout << " cc11Range=[" << cc11Min << "," << cc11Max << "]";
        }
        std::cout << " noteLen(paired=" << pairedNotes
            << ",positive=" << positiveLengthNotes
            << ",zeroOrNeg=" << zeroOrNegativeLengthNotes;
        if (positiveLengthNotes > 0)
        {
            std::cout << ",min=" << minNoteLength
                << ",max=" << maxNoteLength;
        }
        std::cout << ")";
        if (firstNoteOnSample >= 0)
        {
            std::cout << " firstNoteOn(sample=" << firstNoteOnSample
                << ",ch=" << firstNoteOnChannel
                << ",note=" << firstNoteOnNote
                << ",vel=" << firstNoteOnVelocity
                << ")";
        }
        std::cout << std::endl;

        for (size_t i = 0; i < events.size() && i < 8; i++)
        {
            const auto& e = events[i];
            std::cout << "[EventHead] i=" << i
                << " sample=" << e.sample
                << " type=" << (int)e.type
                << " noteOn=" << (e.isNoteOn ? 1 : 0)
                << " ch=" << e.channel
                << " note=" << e.noteNumber
                << " vel=" << e.velocity
                << " cc=" << e.controller
                << " val=" << e.value
                << std::endl;
        }
    }

    if (events.empty())
    {
        std::cout << "No note events found." << std::endl;
        return 1;
    }

    // Channel config (default preset)
    const auto& channelConfigs = config.channelConfigs;

    // Resize output buffer
    int lastSample = events.back().sample;
    int extraRelease = (int)(config.extraReleaseSec * sound.fs);
    int neededSamples = lastSample + extraRelease + 1;
    if (neededSamples > sound.length)
    {
        sound = SoundData(neededSamples, sound.bits, sound.fs);
    }

    std::cout << "Events: " << events.size()
        << ", FirstSample: " << events.front().sample
        << ", LastSample: " << events.back().sample
        << ", Length: " << sound.length << std::endl;

    // Render
    RenderMIDIEvents(sound, events, channelConfigs);
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
        std::cout << "[RenderStats] peak=" << peak
            << " rms=" << rms
            << " nonZero=" << nonZero
            << "/" << sound.data.size()
            << std::endl;
    }

    // Save
    if (std::filesystem::exists(config.wavPath))
    {
        std::error_code rmEc;
        std::filesystem::remove(config.wavPath, rmEc);
        if (rmEc)
        {
            std::cout << "[SavePrep] failed to remove old file: " << rmEc.message() << std::endl;
        }
    }
    if (!SaveWavFilePath(sound, config.wavPath))
    {
        std::cout << "Failed to save WAV: " << config.wavPath.string()
            << " lastError=" << (unsigned long)GetLastError()
            << std::endl;
        return 1;
    }
    std::cout << "Saved SoundData: " << config.wavPath.string() << std::endl;

    return 0;
}

int main(int argc, char** argv)
{
    std::filesystem::path cliConfigPath;
    bool showHelp = false;
    if (!ParseArguments(argc, argv, cliConfigPath, showHelp))
    {
        return 1;
    }
    if (showHelp)
    {
        return 0;
    }

    AppConfig config = DefaultConfig();
    std::filesystem::path selectedConfigPath;
    if (!cliConfigPath.empty())
    {
        selectedConfigPath = cliConfigPath;
    }
    else
    {
        const std::filesystem::path autoConfigPath = FindProjectRoot() / "config" / "default.json";
        if (std::filesystem::exists(autoConfigPath))
        {
            selectedConfigPath = autoConfigPath;
        }
    }

    if (!selectedConfigPath.empty())
    {
        std::string err;
        if (!LoadConfigFile(selectedConfigPath, config, err))
        {
            std::cout << "Failed to load config: " << selectedConfigPath.string()
                << " (" << err << ")" << std::endl;
            return 1;
        }
    }

    if (!selectedConfigPath.empty())
    {
        std::cout << "Config Path: " << selectedConfigPath.string() << std::endl;
    }
    return Run(config);
}
