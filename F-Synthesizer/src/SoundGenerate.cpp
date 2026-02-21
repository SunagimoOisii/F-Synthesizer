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
#include <memory>
#include <functional>
#include <type_traits>
#include <algorithm>
#include <limits>
#include <Windows.h>

#include "AppCore.h"
#include "AudioBuffer.h"
#include "MIDIParser.h"
#include "Sequencer.h"
#include "SynthEngine/SynthEngine.h"
#include "Writer.h"

namespace
{
std::string PathToUtf8(const std::filesystem::path& p)
{
    const auto u8 = p.u8string();
    return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
}

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

std::optional<bool> ReadJsonBool(const std::string& text, const std::string& key)
{
    const std::regex pat("\"" + key + "\"\\s*:\\s*(true|false)");
    std::smatch m;
    if (std::regex_search(text, m, pat) && m.size() >= 2)
    {
        return m[1].str() == "true";
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

bool TryParseNoiseType(const std::string& name, NoiseType& outNoise)
{
    if (name == "white")
    {
        outNoise = NoiseType::White;
        return true;
    }
    if (name == "pink")
    {
        outNoise = NoiseType::Pink;
        return true;
    }
    if (name == "brown")
    {
        outNoise = NoiseType::Brown;
        return true;
    }
    if (name == "blue")
    {
        outNoise = NoiseType::Blue;
        return true;
    }
    return false;
}

bool TryParseDrumType(const std::string& name, DrumType& outType)
{
    if (name == "none")
    {
        outType = DrumType::None;
        return true;
    }
    if (name == "kick")
    {
        outType = DrumType::Kick;
        return true;
    }
    if (name == "snare")
    {
        outType = DrumType::Snare;
        return true;
    }
    if (name == "hat")
    {
        outType = DrumType::Hat;
        return true;
    }
    return false;
}

std::string WaveTypeToString(WaveType w)
{
    switch (w)
    {
    case WaveType::Sine: return "sine";
    case WaveType::Square: return "square";
    case WaveType::Saw: return "saw";
    case WaveType::Triangle: return "triangle";
    }
    return "saw";
}

std::string NoiseTypeToString(NoiseType n)
{
    switch (n)
    {
    case NoiseType::White: return "white";
    case NoiseType::Pink: return "pink";
    case NoiseType::Brown: return "brown";
    case NoiseType::Blue: return "blue";
    }
    return "white";
}

std::string DrumTypeToString(DrumType d)
{
    switch (d)
    {
    case DrumType::None: return "none";
    case DrumType::Kick: return "kick";
    case DrumType::Snare: return "snare";
    case DrumType::Hat: return "hat";
    }
    return "none";
}

std::string EscapeJson(const std::string& src)
{
    std::string out;
    out.reserve(src.size() + 16);
    for (char c : src)
    {
        if (c == '\\') out += "\\\\";
        else if (c == '"') out += "\\\"";
        else if (c == '\n') out += "\\n";
        else out += c;
    }
    return out;
}

bool ExtractObjectAt(const std::string& text, size_t openBracePos, std::string& outObject, std::string& err)
{
    if (openBracePos >= text.size() || text[openBracePos] != '{')
    {
        err = "invalid object start";
        return false;
    }
    int depth = 0;
    bool inString = false;
    bool escaped = false;
    for (size_t i = openBracePos; i < text.size(); i++)
    {
        const char c = text[i];
        if (inString)
        {
            if (escaped)
            {
                escaped = false;
            }
            else if (c == '\\')
            {
                escaped = true;
            }
            else if (c == '"')
            {
                inString = false;
            }
            continue;
        }

        if (c == '"')
        {
            inString = true;
            continue;
        }
        if (c == '{')
        {
            depth++;
        }
        else if (c == '}')
        {
            depth--;
            if (depth == 0)
            {
                outObject = text.substr(openBracePos, i - openBracePos + 1);
                return true;
            }
        }
    }
    err = "unterminated object";
    return false;
}

bool ExtractObjectForKey(const std::string& text, const std::string& key, std::string& outObject, bool& found, std::string& err)
{
    found = false;
    const std::regex keyPat("\"" + key + "\"\\s*:");
    std::smatch m;
    if (!std::regex_search(text, m, keyPat))
    {
        return true;
    }

    found = true;
    size_t pos = static_cast<size_t>(m.position() + m.length());
    while (pos < text.size() && (text[pos] == ' ' || text[pos] == '\t' || text[pos] == '\r' || text[pos] == '\n'))
    {
        pos++;
    }
    if (pos >= text.size() || text[pos] != '{')
    {
        err = "key '" + key + "' must be an object";
        return false;
    }
    return ExtractObjectAt(text, pos, outObject, err);
}

bool ParseTopLevelObjectEntries(const std::string& objText,
    const std::function<bool(const std::string&, const std::string&)>& onEntry,
    std::string& err)
{
    if (objText.size() < 2 || objText.front() != '{' || objText.back() != '}')
    {
        err = "invalid object";
        return false;
    }

    size_t i = 1;
    while (i + 1 < objText.size())
    {
        while (i < objText.size() && (objText[i] == ' ' || objText[i] == '\t' || objText[i] == '\r' || objText[i] == '\n' || objText[i] == ','))
        {
            i++;
        }
        if (i >= objText.size() || objText[i] == '}')
        {
            break;
        }
        if (objText[i] != '"')
        {
            err = "expected key string";
            return false;
        }
        const size_t keyStart = ++i;
        while (i < objText.size() && objText[i] != '"')
        {
            i++;
        }
        if (i >= objText.size())
        {
            err = "unterminated key";
            return false;
        }
        const std::string key = objText.substr(keyStart, i - keyStart);
        i++;
        while (i < objText.size() && (objText[i] == ' ' || objText[i] == '\t' || objText[i] == '\r' || objText[i] == '\n'))
        {
            i++;
        }
        if (i >= objText.size() || objText[i] != ':')
        {
            err = "expected ':' after key";
            return false;
        }
        i++;
        while (i < objText.size() && (objText[i] == ' ' || objText[i] == '\t' || objText[i] == '\r' || objText[i] == '\n'))
        {
            i++;
        }
        if (i >= objText.size() || objText[i] != '{')
        {
            err = "expected object value for key '" + key + "'";
            return false;
        }
        std::string valueObj;
        if (!ExtractObjectAt(objText, i, valueObj, err))
        {
            return false;
        }
        if (!onEntry(key, valueObj))
        {
            return false;
        }
        i += valueObj.size();
    }

    return true;
}

std::shared_ptr<std::array<ChannelConfig, 16>> MakeMutableChannelConfigs(const AppConfig& cfg)
{
    auto table = std::make_shared<std::array<ChannelConfig, 16>>();
    const auto fallback = BuildDefaultChannelConfigs();
    const auto& src = cfg.channelConfigs ? *cfg.channelConfigs : *fallback;
    *table = src;
    return table;
}

std::shared_ptr<std::array<ChannelMixState, 16>> MakeMutableChannelMixStates(const AppConfig& cfg)
{
    auto table = std::make_shared<std::array<ChannelMixState, 16>>();
    const auto fallback = BuildDefaultChannelMixStates();
    const auto& src = cfg.channelMixStates ? *cfg.channelMixStates : *fallback;
    *table = src;
    return table;
}

bool ParseDrumConfigObject(const std::string& text, DrumConfig& drum, std::string& err)
{
    if (auto t = ReadJsonString(text, "drumType"))
    {
        DrumType dt{};
        if (!TryParseDrumType(*t, dt))
        {
            err = "invalid drumType: " + *t;
            return false;
        }
        drum.type = dt;
    }
    if (auto v = ReadJsonDouble(text, "gain")) drum.gain = *v;
    if (auto v = ReadJsonDouble(text, "baseFreq")) drum.baseFreq = *v;
    if (auto v = ReadJsonDouble(text, "pitchDrop")) drum.pitchDrop = *v;
    if (auto v = ReadJsonDouble(text, "pitchDecaySec")) drum.pitchDecaySec = *v;
    if (auto v = ReadJsonDouble(text, "toneFreq")) drum.toneFreq = *v;
    if (auto v = ReadJsonDouble(text, "toneLevel")) drum.toneLevel = *v;
    if (auto v = ReadJsonDouble(text, "noiseLevel")) drum.noiseLevel = *v;
    if (auto v = ReadJsonDouble(text, "hpCut")) drum.hpCut = *v;
    if (auto v = ReadJsonDouble(text, "lpCut")) drum.lpCut = *v;
    if (auto v = ReadJsonString(text, "toneWave"))
    {
        WaveType w{};
        if (!TryParseWaveType(*v, w))
        {
            err = "invalid toneWave: " + *v;
            return false;
        }
        drum.toneWave = (int)w;
    }
    if (auto v = ReadJsonString(text, "noiseType"))
    {
        NoiseType n{};
        if (!TryParseNoiseType(*v, n))
        {
            err = "invalid noiseType: " + *v;
            return false;
        }
        drum.noiseType = (int)n;
    }
    return true;
}

bool ParseSourceObject(const std::string& sourceObjText, SourceConfig& outSource, std::string& err)
{
    const auto type = ReadJsonString(sourceObjText, "type");
    if (!type)
    {
        err = "source.type is required";
        return false;
    }

    if (*type == "waveform")
    {
        auto wave = ReadJsonString(sourceObjText, "wave");
        if (!wave)
        {
            err = "waveform source requires 'wave'";
            return false;
        }
        WaveType w{};
        if (!TryParseWaveType(*wave, w))
        {
            err = "invalid wave: " + *wave;
            return false;
        }
        outSource = WaveformConfig{ w };
        return true;
    }
    if (*type == "noise")
    {
        auto noise = ReadJsonString(sourceObjText, "noise");
        if (!noise)
        {
            err = "noise source requires 'noise'";
            return false;
        }
        NoiseType n{};
        if (!TryParseNoiseType(*noise, n))
        {
            err = "invalid noise: " + *noise;
            return false;
        }
        outSource = NoiseConfig{ n };
        return true;
    }
    if (*type == "fm")
    {
        auto carrier = ReadJsonString(sourceObjText, "carrierWave");
        auto mod = ReadJsonString(sourceObjText, "modWave");
        auto carrierRatio = ReadJsonDouble(sourceObjText, "carrierRatio");
        auto modRatio = ReadJsonDouble(sourceObjText, "modRatio");
        auto index = ReadJsonDouble(sourceObjText, "index");
        auto outLevel = ReadJsonDouble(sourceObjText, "outLevel");
        if (!carrier || !mod || !carrierRatio || !modRatio || !index || !outLevel)
        {
            err = "fm source requires carrierWave/modWave/carrierRatio/modRatio/index/outLevel";
            return false;
        }
        WaveType cw{}, mw{};
        if (!TryParseWaveType(*carrier, cw) || !TryParseWaveType(*mod, mw))
        {
            err = "invalid fm wave type";
            return false;
        }
        outSource = FmConfig{ cw, mw, *carrierRatio, *modRatio, *index, *outLevel };
        return true;
    }
    if (*type == "drum")
    {
        DrumConfig drum{};
        if (!ParseDrumConfigObject(sourceObjText, drum, err))
        {
            return false;
        }
        outSource = drum;
        return true;
    }
    if (*type == "drumkit")
    {
        DrumKitConfig kit{};
        for (auto& d : kit.map)
        {
            d.type = DrumType::None;
        }
        std::string mapObj;
        bool mapFound = false;
        if (!ExtractObjectForKey(sourceObjText, "map", mapObj, mapFound, err))
        {
            return false;
        }
        if (mapFound)
        {
            if (!ParseTopLevelObjectEntries(mapObj, [&](const std::string& k, const std::string& valueObj) {
                int note = -1;
                try
                {
                    note = std::stoi(k);
                }
                catch (...)
                {
                    err = "invalid drumkit note key: " + k;
                    return false;
                }
                if (note < 0 || note > 127)
                {
                    err = "drumkit note out of range: " + k;
                    return false;
                }
                DrumConfig d{};
                if (!ParseDrumConfigObject(valueObj, d, err))
                {
                    return false;
                }
                kit.map[note] = d;
                return true;
                }, err))
            {
                return false;
            }
        }
        outSource = kit;
        return true;
    }

    err = "unknown source.type: " + *type;
    return false;
}

bool ParseChannelObject(const std::string& channelObjText, ChannelConfig& cfg, std::string& err)
{
    if (auto v = ReadJsonDouble(channelObjText, "amp")) cfg.amp = *v;
    if (auto v = ReadJsonDouble(channelObjText, "attackSec")) cfg.attackSec = *v;
    if (auto v = ReadJsonDouble(channelObjText, "decaySec")) cfg.decaySec = *v;
    if (auto v = ReadJsonDouble(channelObjText, "sustainLevel")) cfg.sustainLevel = *v;
    if (auto v = ReadJsonDouble(channelObjText, "releaseSec")) cfg.releaseSec = *v;

    std::string sourceObj;
    bool found = false;
    if (!ExtractObjectForKey(channelObjText, "source", sourceObj, found, err))
    {
        return false;
    }
    if (found)
    {
        SourceConfig s = cfg.source;
        if (!ParseSourceObject(sourceObj, s, err))
        {
            return false;
        }
        cfg.source = s;
    }
    return true;
}

bool LoadChannelsDiff(const std::string& text, AppConfig& cfg, std::string& err)
{
    std::string channelsObj;
    bool found = false;
    if (!ExtractObjectForKey(text, "channels", channelsObj, found, err))
    {
        return false;
    }
    if (!found)
    {
        return true;
    }

    auto table = MakeMutableChannelConfigs(cfg);
    if (!ParseTopLevelObjectEntries(channelsObj, [&](const std::string& k, const std::string& valueObj) {
        int ch = -1;
        try
        {
            ch = std::stoi(k);
        }
        catch (...)
        {
            err = "invalid channel key: " + k;
            return false;
        }
        if (ch < 0 || ch > 15)
        {
            err = "channel key out of range: " + k;
            return false;
        }
        ChannelConfig chCfg = (*table)[ch];
        if (!ParseChannelObject(valueObj, chCfg, err))
        {
            err = "channel " + k + ": " + err;
            return false;
        }
        (*table)[ch] = chCfg;
        return true;
        }, err))
    {
        return false;
    }

    cfg.channelConfigs = table;
    return true;
}

bool ParseChannelMixObject(const std::string& mixObjText, ChannelMixState& mix, std::string& err)
{
    if (auto v = ReadJsonBool(mixObjText, "mute")) mix.mute = *v;
    if (auto v = ReadJsonBool(mixObjText, "solo")) mix.solo = *v;
    if (auto v = ReadJsonDouble(mixObjText, "level")) mix.level = *v;
    if (auto v = ReadJsonDouble(mixObjText, "pan")) mix.pan = *v;
    if (auto v = ReadJsonDouble(mixObjText, "gain")) mix.gain = *v;

    if (mix.level < 0.0 || mix.level > 2.0)
    {
        err = "level must be in range 0.0..2.0";
        return false;
    }
    if (mix.pan < -1.0 || mix.pan > 1.0)
    {
        err = "pan must be in range -1.0..1.0";
        return false;
    }
    if (mix.gain < 0.0 || mix.gain > 4.0)
    {
        err = "gain must be in range 0.0..4.0";
        return false;
    }
    return true;
}

bool LoadChannelMixDiff(const std::string& text, AppConfig& cfg, std::string& err)
{
    std::string mixObj;
    bool found = false;
    if (!ExtractObjectForKey(text, "channelMix", mixObj, found, err))
    {
        return false;
    }
    if (!found)
    {
        return true;
    }

    auto table = MakeMutableChannelMixStates(cfg);
    if (!ParseTopLevelObjectEntries(mixObj, [&](const std::string& k, const std::string& valueObj) {
        int ch = -1;
        try
        {
            ch = std::stoi(k);
        }
        catch (...)
        {
            err = "invalid channelMix key: " + k;
            return false;
        }
        if (ch < 0 || ch > 15)
        {
            err = "channelMix key out of range: " + k;
            return false;
        }
        ChannelMixState mix = (*table)[ch];
        if (!ParseChannelMixObject(valueObj, mix, err))
        {
            err = "channelMix " + k + ": " + err;
            return false;
        }
        (*table)[ch] = mix;
        return true;
        }, err))
    {
        return false;
    }

    cfg.channelMixStates = table;
    return true;
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

bool LoadConfigFileInternal(const std::filesystem::path& configPath, AppConfig& cfg, std::string& err)
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

    if (cfg.targetChannel < -1 || cfg.targetChannel > 15)
    {
        err = "targetChannel must be -1 or 0..15";
        return false;
    }
    if (cfg.sampleRate <= 0)
    {
        err = "sampleRate must be positive";
        return false;
    }
    if (cfg.initialSeconds <= 0)
    {
        err = "initialSeconds must be positive";
        return false;
    }
    if (cfg.bits != 16)
    {
        err = "bits must be 16";
        return false;
    }
    if (!LoadChannelsDiff(text, cfg, err))
    {
        return false;
    }
    if (!LoadChannelMixDiff(text, cfg, err))
    {
        return false;
    }

    return true;
}

void WriteIndent(std::ostream& out, int indent)
{
    for (int i = 0; i < indent; i++)
    {
        out << ' ';
    }
}

void WriteDrumConfig(std::ostream& out, const DrumConfig& d, int indent)
{
    WriteIndent(out, indent); out << "{\n";
    WriteIndent(out, indent + 2); out << "\"drumType\": \"" << DrumTypeToString(d.type) << "\",\n";
    WriteIndent(out, indent + 2); out << "\"gain\": " << d.gain << ",\n";
    WriteIndent(out, indent + 2); out << "\"baseFreq\": " << d.baseFreq << ",\n";
    WriteIndent(out, indent + 2); out << "\"pitchDrop\": " << d.pitchDrop << ",\n";
    WriteIndent(out, indent + 2); out << "\"pitchDecaySec\": " << d.pitchDecaySec << ",\n";
    WriteIndent(out, indent + 2); out << "\"toneFreq\": " << d.toneFreq << ",\n";
    WriteIndent(out, indent + 2); out << "\"toneLevel\": " << d.toneLevel << ",\n";
    WriteIndent(out, indent + 2); out << "\"noiseLevel\": " << d.noiseLevel << ",\n";
    WriteIndent(out, indent + 2); out << "\"hpCut\": " << d.hpCut << ",\n";
    WriteIndent(out, indent + 2); out << "\"lpCut\": " << d.lpCut << ",\n";
    WriteIndent(out, indent + 2); out << "\"toneWave\": \"" << WaveTypeToString((WaveType)d.toneWave) << "\",\n";
    WriteIndent(out, indent + 2); out << "\"noiseType\": \"" << NoiseTypeToString((NoiseType)d.noiseType) << "\"\n";
    WriteIndent(out, indent); out << "}";
}

void WriteSourceConfig(std::ostream& out, const SourceConfig& src, int indent)
{
    WriteIndent(out, indent); out << "\"source\": {\n";
    std::visit([&](const auto& v) {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, WaveformConfig>)
        {
            WriteIndent(out, indent + 2); out << "\"type\": \"waveform\",\n";
            WriteIndent(out, indent + 2); out << "\"wave\": \"" << WaveTypeToString(v.wave) << "\"\n";
        }
        else if constexpr (std::is_same_v<T, NoiseConfig>)
        {
            WriteIndent(out, indent + 2); out << "\"type\": \"noise\",\n";
            WriteIndent(out, indent + 2); out << "\"noise\": \"" << NoiseTypeToString(v.noise) << "\"\n";
        }
        else if constexpr (std::is_same_v<T, FmConfig>)
        {
            WriteIndent(out, indent + 2); out << "\"type\": \"fm\",\n";
            WriteIndent(out, indent + 2); out << "\"carrierWave\": \"" << WaveTypeToString(v.carrierWave) << "\",\n";
            WriteIndent(out, indent + 2); out << "\"modWave\": \"" << WaveTypeToString(v.modWave) << "\",\n";
            WriteIndent(out, indent + 2); out << "\"carrierRatio\": " << v.carrierRatio << ",\n";
            WriteIndent(out, indent + 2); out << "\"modRatio\": " << v.modRatio << ",\n";
            WriteIndent(out, indent + 2); out << "\"index\": " << v.index << ",\n";
            WriteIndent(out, indent + 2); out << "\"outLevel\": " << v.outLevel << "\n";
        }
        else if constexpr (std::is_same_v<T, DrumConfig>)
        {
            WriteIndent(out, indent + 2); out << "\"type\": \"drum\",\n";
            WriteIndent(out, indent + 2); out << "\"drumType\": \"" << DrumTypeToString(v.type) << "\",\n";
            WriteIndent(out, indent + 2); out << "\"gain\": " << v.gain << ",\n";
            WriteIndent(out, indent + 2); out << "\"baseFreq\": " << v.baseFreq << ",\n";
            WriteIndent(out, indent + 2); out << "\"pitchDrop\": " << v.pitchDrop << ",\n";
            WriteIndent(out, indent + 2); out << "\"pitchDecaySec\": " << v.pitchDecaySec << ",\n";
            WriteIndent(out, indent + 2); out << "\"toneFreq\": " << v.toneFreq << ",\n";
            WriteIndent(out, indent + 2); out << "\"toneLevel\": " << v.toneLevel << ",\n";
            WriteIndent(out, indent + 2); out << "\"noiseLevel\": " << v.noiseLevel << ",\n";
            WriteIndent(out, indent + 2); out << "\"hpCut\": " << v.hpCut << ",\n";
            WriteIndent(out, indent + 2); out << "\"lpCut\": " << v.lpCut << ",\n";
            WriteIndent(out, indent + 2); out << "\"toneWave\": \"" << WaveTypeToString((WaveType)v.toneWave) << "\",\n";
            WriteIndent(out, indent + 2); out << "\"noiseType\": \"" << NoiseTypeToString((NoiseType)v.noiseType) << "\"\n";
        }
        else if constexpr (std::is_same_v<T, DrumKitConfig>)
        {
            WriteIndent(out, indent + 2); out << "\"type\": \"drumkit\",\n";
            WriteIndent(out, indent + 2); out << "\"map\": {\n";
            bool first = true;
            for (int note = 0; note < 128; note++)
            {
                const auto& d = v.map[note];
                if (d.type == DrumType::None) continue;
                if (!first) out << ",\n";
                first = false;
                WriteIndent(out, indent + 4); out << "\"" << note << "\": ";
                WriteDrumConfig(out, d, 0);
            }
            out << "\n";
            WriteIndent(out, indent + 2); out << "}\n";
        }
        }, src);
    WriteIndent(out, indent); out << "}";
}

void WriteChannelConfig(std::ostream& out, int ch, const ChannelConfig& cfg, bool withComma)
{
    WriteIndent(out, 4); out << "\"" << ch << "\": {\n";
    WriteIndent(out, 6); out << "\"amp\": " << cfg.amp << ",\n";
    WriteIndent(out, 6); out << "\"attackSec\": " << cfg.attackSec << ",\n";
    WriteIndent(out, 6); out << "\"decaySec\": " << cfg.decaySec << ",\n";
    WriteIndent(out, 6); out << "\"sustainLevel\": " << cfg.sustainLevel << ",\n";
    WriteIndent(out, 6); out << "\"releaseSec\": " << cfg.releaseSec << ",\n";
    WriteSourceConfig(out, cfg.source, 6);
    out << "\n";
    WriteIndent(out, 4); out << "}";
    if (withComma) out << ",";
    out << "\n";
}

void WriteChannelMixState(std::ostream& out, int ch, const ChannelMixState& mix, bool withComma)
{
    WriteIndent(out, 4); out << "\"" << ch << "\": {\n";
    WriteIndent(out, 6); out << "\"mute\": " << (mix.mute ? "true" : "false") << ",\n";
    WriteIndent(out, 6); out << "\"solo\": " << (mix.solo ? "true" : "false") << ",\n";
    WriteIndent(out, 6); out << "\"level\": " << mix.level << ",\n";
    WriteIndent(out, 6); out << "\"pan\": " << mix.pan << ",\n";
    WriteIndent(out, 6); out << "\"gain\": " << mix.gain << "\n";
    WriteIndent(out, 4); out << "}";
    if (withComma) out << ",";
    out << "\n";
}

bool ParseArguments(
    int argc,
    char** argv,
    std::filesystem::path& configPath,
    std::string& presetName,
    bool& startCli,
    bool& showHelp)
{
    configPath.clear();
    presetName.clear();
    startCli = false;
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
            startCli = true;
        }
        else if (arg == "--preset")
        {
            if (i + 1 >= argc)
            {
                return false;
            }
            presetName = argv[++i];
            startCli = true;
        }
        else if (arg == "--cli")
        {
            startCli = true;
        }
        else if (arg == "--gui")
        {
            startCli = false;
        }
        else if (arg == "--help" || arg == "-h")
        {
            std::cout << "Usage: F-Synthesizer.exe [--gui] [--cli] [--config path/to/config.json] [--preset name]" << std::endl;
            std::cout << "Default: start GUI when no CLI options are given." << std::endl;
            showHelp = true;
            return true;
        }
    }
    return true;
}

void LogLine(IRunObserver* observer, const std::string& line)
{
    std::cout << line << std::endl;
    if (observer != nullptr)
    {
        observer->OnLogLine(line);
    }
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

bool LoadConfigFile(const std::filesystem::path& configPath, AppConfig& cfg, std::string& err)
{
    return LoadConfigFileInternal(configPath, cfg, err);
}

bool SaveConfigFile(const std::filesystem::path& configPath, const AppConfig& config, std::string& err)
{
    std::error_code ec;
    if (configPath.has_parent_path())
    {
        std::filesystem::create_directories(configPath.parent_path(), ec);
    }

    std::ofstream out(configPath, std::ios::binary | std::ios::trunc);
    if (!out)
    {
        err = "failed to open output file";
        return false;
    }

    out << "{\n";
    out << "  \"midiPath\": \"" << EscapeJson(PathToUtf8(config.midiPath)) << "\",\n";
    out << "  \"wavPath\": \"" << EscapeJson(PathToUtf8(config.wavPath)) << "\",\n";
    out << "  \"targetChannel\": " << config.targetChannel << ",\n";
    out << "  \"defaultWave\": \"" << WaveTypeToString(config.defaultWave) << "\",\n";
    out << "  \"initialSeconds\": " << config.initialSeconds << ",\n";
    out << "  \"bits\": " << config.bits << ",\n";
    out << "  \"sampleRate\": " << config.sampleRate << ",\n";
    out << "  \"extraReleaseSec\": " << config.extraReleaseSec << ",\n";
    out << "  \"channels\": {\n";

    const auto fallback = BuildDefaultChannelConfigs();
    const auto& channels = config.channelConfigs ? *config.channelConfigs : *fallback;
    for (int ch = 0; ch < 16; ch++)
    {
        WriteChannelConfig(out, ch, channels[ch], ch != 15);
    }

    out << "  },\n";
    out << "  \"channelMix\": {\n";
    const auto fallbackMix = BuildDefaultChannelMixStates();
    const auto& channelMix = config.channelMixStates ? *config.channelMixStates : *fallbackMix;
    for (int ch = 0; ch < 16; ch++)
    {
        WriteChannelMixState(out, ch, channelMix[ch], ch != 15);
    }
    out << "  }\n";
    out << "}\n";

    return true;
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

std::vector<MIDIEvent> BuildWindowedEvents(
    const std::vector<MIDIEvent>& events,
    int startSample,
    int endSample)
{
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

int Run(const AppConfig& config, const RenderOptions& options, IRunObserver* observer, SoundData* renderedSound)
{
    LogLine(observer, "Build Marker: 2026-02-21-save-debug-v1");
    if (options.writeWav)
    {
        std::filesystem::create_directories(config.wavPath.parent_path());
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
    if (!LoadMIDIBasic(config.midiPath, config.targetChannel, ticks, tempoEvents, midiTPQ, stats))
    {
        LogLine(observer, "Failed to load MIDI: " + PathToUtf8(config.midiPath));
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
    std::vector<MIDIEvent> events;
    BuildSampleEvents(ticks, tempoEvents, midiTPQ, sound.fs, config.defaultWave, events);
    if (options.startSec > 0.0 || options.durationSec >= 0.0)
    {
        const double startSec = (options.startSec > 0.0) ? options.startSec : 0.0;
        int startSample = static_cast<int>(startSec * sound.fs);
        int endSample = (std::numeric_limits<int>::max)();
        if (options.durationSec >= 0.0)
        {
            const double durSec = (options.durationSec > 0.0) ? options.durationSec : 0.0;
            const int durSamples = static_cast<int>(durSec * sound.fs);
            endSample = startSample + durSamples;
        }
        events = BuildWindowedEvents(events, startSample, endSample);
    }
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
    RenderMIDIEvents(sound, events, channelConfigs, channelMixStates);
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
        if (!SaveWavFilePath(sound, config.wavPath))
        {
            std::ostringstream oss;
            oss << "Failed to save WAV: " << PathToUtf8(config.wavPath)
                << " lastError=" << (unsigned long)GetLastError();
            LogLine(observer, oss.str());
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

    std::filesystem::path cliConfigPath;
    std::string cliPresetName;
    bool startCli = false;
    bool showHelp = false;
    if (!ParseArguments(argc, argv, cliConfigPath, cliPresetName, startCli, showHelp))
    {
        return 1;
    }
    if (showHelp)
    {
        return 0;
    }
    if (!startCli)
    {
        return RunGuiApp();
    }

    AppConfig config = DefaultConfig();
    const std::filesystem::path projectRoot = FindProjectRootPath();
    std::filesystem::path selectedConfigPath;
    if (!cliConfigPath.empty())
    {
        selectedConfigPath = cliConfigPath;
    }
    else if (cliPresetName.empty())
    {
        const std::filesystem::path autoConfigPath = projectRoot / "config" / "default.json";
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
    else if (!cliPresetName.empty())
    {
        std::string err;
        const std::filesystem::path basePath = projectRoot / "config" / "base.json";
        const std::filesystem::path presetPath = projectRoot / "config" / "presets" / (cliPresetName + ".json");
        if (!std::filesystem::exists(basePath))
        {
            std::cout << "Base config not found: " << basePath.string() << std::endl;
            return 1;
        }
        if (!std::filesystem::exists(presetPath))
        {
            std::cout << "Preset config not found: " << presetPath.string() << std::endl;
            return 1;
        }
        if (!LoadConfigFile(basePath, config, err))
        {
            std::cout << "Failed to load base config: " << basePath.string()
                << " (" << err << ")" << std::endl;
            return 1;
        }
        if (!LoadConfigFile(presetPath, config, err))
        {
            std::cout << "Failed to load preset config: " << presetPath.string()
                << " (" << err << ")" << std::endl;
            return 1;
        }
        std::cout << "Preset: " << cliPresetName << std::endl;
        std::cout << "Base Config Path: " << basePath.string() << std::endl;
        std::cout << "Preset Config Path: " << presetPath.string() << std::endl;
    }
    else
    {
        const std::filesystem::path basePath = projectRoot / "config" / "base.json";
        const std::filesystem::path fallbackPresetPath = projectRoot / "config" / "presets" / "basic_wave.json";
        std::string err;
        if (std::filesystem::exists(basePath) && std::filesystem::exists(fallbackPresetPath))
        {
            if (!LoadConfigFile(basePath, config, err))
            {
                std::cout << "Failed to load base config: " << basePath.string()
                    << " (" << err << ")" << std::endl;
                return 1;
            }
            if (!LoadConfigFile(fallbackPresetPath, config, err))
            {
                std::cout << "Failed to load preset config: " << fallbackPresetPath.string()
                    << " (" << err << ")" << std::endl;
                return 1;
            }
            std::cout << "Preset: basic_wave (auto)" << std::endl;
            std::cout << "Base Config Path: " << basePath.string() << std::endl;
            std::cout << "Preset Config Path: " << fallbackPresetPath.string() << std::endl;
        }
    }

    if (!selectedConfigPath.empty())
    {
        std::cout << "Config Path: " << selectedConfigPath.string() << std::endl;
    }
    return Run(config);
}
