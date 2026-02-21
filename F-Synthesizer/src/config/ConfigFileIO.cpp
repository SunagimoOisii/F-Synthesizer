#include "AppCore.h"

#include <array>
#include <filesystem>
#include <fstream>
#include <functional>
#include <optional>
#include <regex>
#include <sstream>
#include <string>
#include <type_traits>

namespace
{
std::string PathToUtf8(const std::filesystem::path& p)
{
    const auto u8 = p.u8string();
    return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
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

bool ParseTopLevelObjectEntries(
    const std::string& objText,
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
    AppConfig base = DefaultConfig();
    const auto& src = cfg.channelConfigs ? *cfg.channelConfigs : *base.channelConfigs;
    *table = src;
    return table;
}

std::shared_ptr<std::array<ChannelMixState, 16>> MakeMutableChannelMixStates(const AppConfig& cfg)
{
    auto table = std::make_shared<std::array<ChannelMixState, 16>>();
    AppConfig base = DefaultConfig();
    const auto& src = cfg.channelMixStates ? *cfg.channelMixStates : *base.channelMixStates;
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
} // namespace

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

    AppConfig base = DefaultConfig();
    const auto& channels = config.channelConfigs ? *config.channelConfigs : *base.channelConfigs;
    for (int ch = 0; ch < 16; ch++)
    {
        WriteChannelConfig(out, ch, channels[ch], ch != 15);
    }

    out << "  },\n";
    out << "  \"channelMix\": {\n";
    const auto& channelMix = config.channelMixStates ? *config.channelMixStates : *base.channelMixStates;
    for (int ch = 0; ch < 16; ch++)
    {
        WriteChannelMixState(out, ch, channelMix[ch], ch != 15);
    }
    out << "  }\n";
    out << "}\n";

    return true;
}
