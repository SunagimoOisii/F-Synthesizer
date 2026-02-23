#include "ConfigFileInternal.h"

#include <fstream>
#include <regex>
#include <sstream>
#include <type_traits>

#include "config/SourceRegistry.h"

namespace config::internal
{
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

bool TryParseFilterMode(const std::string& name, FilterMode& outMode)
{
    if (name == "bypass")
    {
        outMode = FilterMode::Bypass;
        return true;
    }
    if (name == "lowpass")
    {
        outMode = FilterMode::LowPass;
        return true;
    }
    if (name == "highpass")
    {
        outMode = FilterMode::HighPass;
        return true;
    }
    if (name == "bandpass")
    {
        outMode = FilterMode::BandPass;
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

std::string FilterModeToString(FilterMode mode)
{
    switch (mode)
    {
    case FilterMode::Bypass: return "bypass";
    case FilterMode::LowPass: return "lowpass";
    case FilterMode::HighPass: return "highpass";
    case FilterMode::BandPass: return "bandpass";
    }
    return "bypass";
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
            WriteIndent(out, indent + 2); out << "\"type\": \"" << config::SourceKindToTypeName(config::SourceKind::Waveform) << "\",\n";
            WriteIndent(out, indent + 2); out << "\"wave\": \"" << WaveTypeToString(v.wave) << "\",\n";
            WriteIndent(out, indent + 2); out << "\"unisonVoices\": " << v.unisonVoices << ",\n";
            WriteIndent(out, indent + 2); out << "\"unisonDetuneCents\": " << v.unisonDetuneCents << ",\n";
            WriteIndent(out, indent + 2); out << "\"unisonSpread\": " << v.unisonSpread << ",\n";
            WriteIndent(out, indent + 2); out << "\"subOscLevel\": " << v.subOscLevel << ",\n";
            WriteIndent(out, indent + 2); out << "\"filterMode\": \"" << FilterModeToString(v.filterMode) << "\",\n";
            WriteIndent(out, indent + 2); out << "\"filterCutoffHz\": " << v.filterCutoffHz << ",\n";
            WriteIndent(out, indent + 2); out << "\"filterResonance\": " << v.filterResonance << "\n";
        }
        else if constexpr (std::is_same_v<T, NoiseConfig>)
        {
            WriteIndent(out, indent + 2); out << "\"type\": \"" << config::SourceKindToTypeName(config::SourceKind::Noise) << "\",\n";
            WriteIndent(out, indent + 2); out << "\"noise\": \"" << NoiseTypeToString(v.noise) << "\"\n";
        }
        else if constexpr (std::is_same_v<T, FmConfig>)
        {
            WriteIndent(out, indent + 2); out << "\"type\": \"" << config::SourceKindToTypeName(config::SourceKind::Fm) << "\",\n";
            WriteIndent(out, indent + 2); out << "\"carrierWave\": \"" << WaveTypeToString(v.carrierWave) << "\",\n";
            WriteIndent(out, indent + 2); out << "\"modWave\": \"" << WaveTypeToString(v.modWave) << "\",\n";
            WriteIndent(out, indent + 2); out << "\"carrierRatio\": " << v.carrierRatio << ",\n";
            WriteIndent(out, indent + 2); out << "\"modRatio\": " << v.modRatio << ",\n";
            WriteIndent(out, indent + 2); out << "\"index\": " << v.index << ",\n";
            WriteIndent(out, indent + 2); out << "\"outLevel\": " << v.outLevel << "\n";
        }
        else if constexpr (std::is_same_v<T, DrumConfig>)
        {
            WriteIndent(out, indent + 2); out << "\"type\": \"" << config::SourceKindToTypeName(config::SourceKind::Drum) << "\",\n";
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
            WriteIndent(out, indent + 2); out << "\"type\": \"" << config::SourceKindToTypeName(config::SourceKind::DrumKit) << "\",\n";
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
} // namespace config::internal
