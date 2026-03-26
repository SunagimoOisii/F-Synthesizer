#include "ConfigFileInternal.h"

#include <fstream>
#include <regex>
#include <sstream>
#include <type_traits>

#include "config/SourceRegistry.h"

namespace config::internal
{
// 目的: 設定ファイルを文字列ベースで読み書きする共通ユーティリティ群。
// 前提: 本実装は軽量運用を優先し、正規JSONパーサではなく正規表現/手書き走査を使う。
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

std::optional<std::string> ReadJSONString(const std::string& text, const std::string& key)
{
    const std::regex pat("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch m;
    if (std::regex_search(text, m, pat) && m.size() >= 2)
    {
        return m[1].str();
    }
    return std::nullopt;
}

std::optional<int> ReadJSONInt(const std::string& text, const std::string& key)
{
    const std::regex pat("\"" + key + "\"\\s*:\\s*(-?\\d+)");
    std::smatch m;
    if (std::regex_search(text, m, pat) && m.size() >= 2)
    {
        return std::stoi(m[1].str());
    }
    return std::nullopt;
}

std::optional<double> ReadJSONDouble(const std::string& text, const std::string& key)
{
    const std::regex pat("\"" + key + "\"\\s*:\\s*(-?\\d+(?:\\.\\d+)?)");
    std::smatch m;
    if (std::regex_search(text, m, pat) && m.size() >= 2)
    {
        return std::stod(m[1].str());
    }
    return std::nullopt;
}

std::optional<bool> ReadJSONBool(const std::string& text, const std::string& key)
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

bool TryParseLfoWave(const std::string& name, LfoWave& outWave)
{
    if (name == "sine")
    {
        outWave = LfoWave::Sine;
        return true;
    }
    if (name == "triangle")
    {
        outWave = LfoWave::Triangle;
        return true;
    }
    if (name == "square")
    {
        outWave = LfoWave::Square;
        return true;
    }
    if (name == "saw")
    {
        outWave = LfoWave::Saw;
        return true;
    }
    if (name == "sampleAndHold")
    {
        outWave = LfoWave::SampleAndHold;
        return true;
    }
    return false;
}

bool TryParseModSource(const std::string& name, ModSource& outSource)
{
    if (name == "none")
    {
        outSource = ModSource::None;
        return true;
    }
    if (name == "lfo1")
    {
        outSource = ModSource::Lfo1;
        return true;
    }
    if (name == "env2")
    {
        outSource = ModSource::Env2;
        return true;
    }
    if (name == "velocity")
    {
        outSource = ModSource::Velocity;
        return true;
    }
    if (name == "channelPressure")
    {
        outSource = ModSource::ChannelPressure;
        return true;
    }
    if (name == "polyPressure")
    {
        outSource = ModSource::PolyPressure;
        return true;
    }
    if (name == "modWheel")
    {
        outSource = ModSource::ModWheel;
        return true;
    }
    return false;
}

bool TryParseModDestination(const std::string& name, ModDestination& outDestination)
{
    // 互換方針:
    // - 新命名: pitchMul / filterCutoffHz
    // - 旧命名: pitch / filterCutoff も読込だけ許可する。
    // - pan は現行モノラル経路では非採用のため非受理。
    // - source固有 destination は source.type ごとの検証で受理可否を制御する。
    if (name == "none")
    {
        outDestination = ModDestination::None;
        return true;
    }
    if (name == "pitch" || name == "pitchMul")
    {
        outDestination = ModDestination::Pitch;
        return true;
    }
    if (name == "amp")
    {
        outDestination = ModDestination::Amp;
        return true;
    }
    if (name == "filterCutoff" || name == "filterCutoffHz")
    {
        outDestination = ModDestination::FilterCutoff;
        return true;
    }
    if (name == "filterResonance")
    {
        outDestination = ModDestination::FilterResonance;
        return true;
    }
    if (name == "fm.index")
    {
        outDestination = ModDestination::FmIndex;
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

std::string PsgWaveTypeToString(PsgWaveType w)
{
    switch (w)
    {
    case PsgWaveType::Square: return "square";
    case PsgWaveType::Pulse: return "pulse";
    case PsgWaveType::Triangle: return "triangle";
    case PsgWaveType::Noise: return "noise";
    }
    return "square";
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

std::string LfoWaveToString(LfoWave wave)
{
    switch (wave)
    {
    case LfoWave::Sine: return "sine";
    case LfoWave::Triangle: return "triangle";
    case LfoWave::Square: return "square";
    case LfoWave::Saw: return "saw";
    case LfoWave::SampleAndHold: return "sampleAndHold";
    }
    return "sine";
}

std::string ModSourceToString(ModSource source)
{
    switch (source)
    {
    case ModSource::None: return "none";
    case ModSource::Lfo1: return "lfo1";
    case ModSource::Env2: return "env2";
    case ModSource::Velocity: return "velocity";
    case ModSource::ChannelPressure: return "channelPressure";
    case ModSource::PolyPressure: return "polyPressure";
    case ModSource::ModWheel: return "modWheel";
    }
    return "none";
}

std::string ModDestinationToString(ModDestination destination)
{
    switch (destination)
    {
    case ModDestination::None: return "none";
    case ModDestination::Pitch: return "pitchMul";
    case ModDestination::Amp: return "amp";
    case ModDestination::FilterCutoff: return "filterCutoffHz";
    case ModDestination::FilterResonance: return "filterResonance";
    case ModDestination::FmIndex: return "fm.index";
    }
    return "none";
}

std::string EscapeJSON(const std::string& src)
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
    // 目的: openBracePos から対応する '}' までを切り出す。
    // 前提: 文字列リテラル内の '{' '}' は深さ計算に含めない。
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
    // 目的: {"k": {...}, ...} 形式の top-level object を1段だけ走査する。
    // 制約: value は object を前提とし、配列やプリミティブ値はここでは扱わない。
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

void WriteModulationConfig(std::ostream& out, const ModulationConfig& m, int indent)
{
    WriteIndent(out, indent); out << "\"modulation\": {\n";
    WriteIndent(out, indent + 2); out << "\"lfo1\": {\n";
    WriteIndent(out, indent + 4); out << "\"wave\": \"" << LfoWaveToString(m.lfo1.wave) << "\",\n";
    WriteIndent(out, indent + 4); out << "\"rateHz\": " << m.lfo1.rateHz << ",\n";
    WriteIndent(out, indent + 4); out << "\"depth\": " << m.lfo1.depth << ",\n";
    WriteIndent(out, indent + 4); out << "\"bipolar\": " << (m.lfo1.bipolar ? "true" : "false") << ",\n";
    WriteIndent(out, indent + 4); out << "\"keySync\": " << (m.lfo1.keySync ? "true" : "false") << ",\n";
    WriteIndent(out, indent + 4); out << "\"delayMs\": " << m.lfo1.delayMs << ",\n";
    WriteIndent(out, indent + 4); out << "\"fadeMs\": " << m.lfo1.fadeMs << "\n";
    WriteIndent(out, indent + 2); out << "},\n";

    WriteIndent(out, indent + 2); out << "\"env2\": {\n";
    WriteIndent(out, indent + 4); out << "\"attackSec\": " << m.env2.attackSec << ",\n";
    WriteIndent(out, indent + 4); out << "\"decaySec\": " << m.env2.decaySec << ",\n";
    WriteIndent(out, indent + 4); out << "\"sustainLevel\": " << m.env2.sustainLevel << ",\n";
    WriteIndent(out, indent + 4); out << "\"releaseSec\": " << m.env2.releaseSec << "\n";
    WriteIndent(out, indent + 2); out << "},\n";

    WriteIndent(out, indent + 2); out << "\"routes\": {\n";
    for (int i = 0; i < static_cast<int>(m.matrix.routes.size()); i++)
    {
        const ModRoute& r = m.matrix.routes[static_cast<size_t>(i)];
        WriteIndent(out, indent + 4); out << "\"" << i << "\": {\n";
        WriteIndent(out, indent + 6); out << "\"source\": \"" << ModSourceToString(r.source) << "\",\n";
        WriteIndent(out, indent + 6); out << "\"destination\": \"" << ModDestinationToString(r.destination) << "\",\n";
        WriteIndent(out, indent + 6); out << "\"amount\": " << r.amount << ",\n";
        WriteIndent(out, indent + 6); out << "\"enabled\": " << (r.enabled ? "true" : "false") << "\n";
        WriteIndent(out, indent + 4); out << "}";
        out << ((i + 1 < static_cast<int>(m.matrix.routes.size())) ? ",\n" : "\n");
    }
    WriteIndent(out, indent + 2); out << "}\n";
    WriteIndent(out, indent); out << "}";
}

template <typename SmoothingT>
void WriteWaveformSmoothingConfig(std::ostream& out, const SmoothingT& smoothing, int indent)
{
    WriteIndent(out, indent); out << "\"smoothing\": {\n";
    WriteIndent(out, indent + 2); out << "\"enabled\": " << (smoothing.enabled ? "true" : "false") << ",\n";
    WriteIndent(out, indent + 2); out << "\"pitchEnabled\": " << (smoothing.pitchEnabled ? "true" : "false") << ",\n";
    WriteIndent(out, indent + 2); out << "\"ampTimeMs\": " << smoothing.ampTimeMs << ",\n";
    WriteIndent(out, indent + 2); out << "\"pitchTimeMs\": " << smoothing.pitchTimeMs << ",\n";
    WriteIndent(out, indent + 2); out << "\"filterCutoffTimeMs\": " << smoothing.filterCutoffTimeMs << "\n";
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
            WriteIndent(out, indent + 2); out << "\"filterResonance\": " << v.filterResonance << ",\n";
            WriteIndent(out, indent + 2); out << "\"filterKeytrack\": " << v.filterKeytrack << ",\n";
            WriteIndent(out, indent + 2); out << "\"drive\": " << v.drive << ",\n";
            WriteWaveformSmoothingConfig(out, v.smoothing, indent + 2);
            out << ",\n";
            WriteModulationConfig(out, v.modulation, indent + 2);
            out << "\n";
        }
        else if constexpr (std::is_same_v<T, AnalogConfig>)
        {
            WriteIndent(out, indent + 2); out << "\"type\": \"analog\",\n";
            WriteIndent(out, indent + 2); out << "\"wave\": \"" << WaveTypeToString(v.wave) << "\",\n";
            WriteIndent(out, indent + 2); out << "\"unisonVoices\": " << v.unisonVoices << ",\n";
            WriteIndent(out, indent + 2); out << "\"unisonDetuneCents\": " << v.unisonDetuneCents << ",\n";
            WriteIndent(out, indent + 2); out << "\"unisonSpread\": " << v.unisonSpread << ",\n";
            WriteIndent(out, indent + 2); out << "\"subOscLevel\": " << v.subOscLevel << ",\n";
            WriteIndent(out, indent + 2); out << "\"filterMode\": \"" << FilterModeToString(v.filterMode) << "\",\n";
            WriteIndent(out, indent + 2); out << "\"filterCutoffHz\": " << v.filterCutoffHz << ",\n";
            WriteIndent(out, indent + 2); out << "\"filterResonance\": " << v.filterResonance << ",\n";
            WriteIndent(out, indent + 2); out << "\"filterKeytrack\": " << v.filterKeytrack << ",\n";
            WriteIndent(out, indent + 2); out << "\"drive\": " << v.drive << ",\n";
            WriteIndent(out, indent + 2); out << "\"driftDepthCents\": " << v.driftDepthCents << ",\n";
            WriteIndent(out, indent + 2); out << "\"driftRateHz\": " << v.driftRateHz << ",\n";
            WriteWaveformSmoothingConfig(out, v.smoothing, indent + 2);
            out << ",\n";
            WriteModulationConfig(out, v.modulation, indent + 2);
            out << "\n";
        }
        else if constexpr (std::is_same_v<T, NoiseConfig>)
        {
            WriteIndent(out, indent + 2); out << "\"type\": \"" << config::SourceKindToTypeName(config::SourceKind::Noise) << "\",\n";
            WriteIndent(out, indent + 2); out << "\"noise\": \"" << NoiseTypeToString(v.noise) << "\",\n";
            WriteIndent(out, indent + 2); out << "\"filterMode\": \"" << FilterModeToString(v.filterMode) << "\",\n";
            WriteIndent(out, indent + 2); out << "\"filterCutoffHz\": " << v.filterCutoffHz << ",\n";
            WriteIndent(out, indent + 2); out << "\"filterResonance\": " << v.filterResonance << "\n";
        }
        else if constexpr (std::is_same_v<T, FmConfig>)
        {
            WriteIndent(out, indent + 2); out << "\"type\": \"" << config::SourceKindToTypeName(config::SourceKind::Fm) << "\",\n";
            WriteIndent(out, indent + 2); out << "\"algorithm\": " << v.algorithm << ",\n";
            WriteIndent(out, indent + 2); out << "\"feedback\": " << v.feedback << ",\n";
            WriteIndent(out, indent + 2); out << "\"ops\": [\n";
            for (size_t i = 0; i < 4; i++)
            {
                const FmOperator& op = v.ops[i];
                WriteIndent(out, indent + 4); out << "{ \"wave\": \"" << WaveTypeToString(op.wave)
                    << "\", \"ratio\": " << op.ratio
                    << ", \"level\": " << op.level
                    << ", \"index\": " << op.index << " }";
                if (i + 1 < 4) out << ",";
                out << "\n";
            }
            WriteIndent(out, indent + 2); out << "],\n";
            WriteIndent(out, indent + 2); out << "\"filterMode\": \"" << FilterModeToString(v.filterMode) << "\",\n";
            WriteIndent(out, indent + 2); out << "\"filterCutoffHz\": " << v.filterCutoffHz << ",\n";
            WriteIndent(out, indent + 2); out << "\"filterResonance\": " << v.filterResonance << ",\n";
            WriteIndent(out, indent + 2); out << "\"drive\": " << v.drive << ",\n";
            WriteModulationConfig(out, v.modulation, indent + 2);
            out << "\n";
        }
        else if constexpr (std::is_same_v<T, PsgConfig>)
        {
            WriteIndent(out, indent + 2); out << "\"type\": \"" << config::SourceKindToTypeName(config::SourceKind::Psg) << "\",\n";
            WriteIndent(out, indent + 2); out << "\"wave\": \"" << PsgWaveTypeToString(v.wave) << "\",\n";
            WriteIndent(out, indent + 2); out << "\"duty\": " << v.duty << ",\n";
            WriteIndent(out, indent + 2); out << "\"volumeSteps\": " << v.volumeSteps << ",\n";
            WriteIndent(out, indent + 2); out << "\"maxVoices\": " << v.maxVoices << "\n";
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
    WriteIndent(out, 6); out << "\"portamentoTimeSec\": " << cfg.portamentoTimeSec << ",\n";
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
