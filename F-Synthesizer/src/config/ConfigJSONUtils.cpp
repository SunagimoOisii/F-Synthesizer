#include "ConfigFileInternal.h"

#include <fstream>
#include <sstream>
#include <type_traits>

#include "third_party/nlohmann/json.hpp"
#include "config/SourceRegistry.h"

namespace config::internal
{
namespace
{
using Json = nlohmann::json;

std::optional<Json> ParseJSONObject(const std::string& text)
{
    try
    {
        Json parsed = Json::parse(text, nullptr, false);
        if (!parsed.is_object())
        {
            return std::nullopt;
        }
        return parsed;
    }
    catch (...)
    {
        return std::nullopt;
    }
}
} // namespace

// 目的: 設定ファイルを文字列ベースで読み書きする共通ユーティリティ群。
// 前提: 読み取りは正規JSONパーサ(nlohmann/json)を使い、書き出しは既存フォーマットを維持する。
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
    const auto root = ParseJSONObject(text);
    if (!root)
    {
        return std::nullopt;
    }
    const auto it = root->find(key);
    if (it == root->end() || !it->is_string())
    {
        return std::nullopt;
    }
    return it->get<std::string>();
}

std::optional<int> ReadJSONInt(const std::string& text, const std::string& key)
{
    const auto root = ParseJSONObject(text);
    if (!root)
    {
        return std::nullopt;
    }
    const auto it = root->find(key);
    if (it == root->end() || !it->is_number_integer())
    {
        return std::nullopt;
    }
    return it->get<int>();
}

std::optional<double> ReadJSONDouble(const std::string& text, const std::string& key)
{
    const auto root = ParseJSONObject(text);
    if (!root)
    {
        return std::nullopt;
    }
    const auto it = root->find(key);
    if (it == root->end() || !it->is_number())
    {
        return std::nullopt;
    }
    return it->get<double>();
}

std::optional<bool> ReadJSONBool(const std::string& text, const std::string& key)
{
    const auto root = ParseJSONObject(text);
    if (!root)
    {
        return std::nullopt;
    }
    const auto it = root->find(key);
    if (it == root->end() || !it->is_boolean())
    {
        return std::nullopt;
    }
    return it->get<bool>();
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
    if (name == "tom")
    {
        outType = DrumType::Tom;
        return true;
    }
    if (name == "rim")
    {
        outType = DrumType::Rim;
        return true;
    }
    if (name == "clap")
    {
        outType = DrumType::Clap;
        return true;
    }
    if (name == "crash")
    {
        outType = DrumType::Crash;
        return true;
    }
    if (name == "ride")
    {
        outType = DrumType::Ride;
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
    if (name == "ladderLowpass")
    {
        outMode = FilterMode::LadderLowPass;
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
    if (name == "none")
    {
        outDestination = ModDestination::None;
        return true;
    }
    if (name == "pitchMul")
    {
        outDestination = ModDestination::Pitch;
        return true;
    }
    if (name == "amp")
    {
        outDestination = ModDestination::Amp;
        return true;
    }
    if (name == "filterCutoffHz")
    {
        outDestination = ModDestination::FilterCutoff;
        return true;
    }
    if (name == "filterResonance")
    {
        outDestination = ModDestination::FilterResonance;
        return true;
    }
    if (name == "pulseWidth")
    {
        outDestination = ModDestination::PulseWidth;
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
    case DrumType::Tom: return "tom";
    case DrumType::Rim: return "rim";
    case DrumType::Clap: return "clap";
    case DrumType::Crash: return "crash";
    case DrumType::Ride: return "ride";
    }
    return "none";
}

std::string AttackLayerTypeToString(AttackLayerType type)
{
    switch (type)
    {
    case AttackLayerType::Pick: return "pick";
    case AttackLayerType::Brass: return "brass";
    case AttackLayerType::Metal: return "metal";
    }
    return "pick";
}

std::string BassLayerTypeToString(BassLayerType type)
{
    switch (type)
    {
    case BassLayerType::Sub: return "sub";
    case BassLayerType::Drive: return "drive";
    case BassLayerType::Grit: return "grit";
    }
    return "drive";
}

std::string LeadLayerTypeToString(LeadLayerType type)
{
    switch (type)
    {
    case LeadLayerType::Blade: return "blade";
    case LeadLayerType::Brass: return "brass";
    case LeadLayerType::Edge: return "edge";
    }
    return "blade";
}

std::string BodyLayerModeToString(BodyLayerConfig::Mode mode)
{
    switch (mode)
    {
    case BodyLayerConfig::Mode::Harmonic: return "harmonic";
    case BodyLayerConfig::Mode::Box: return "box";
    case BodyLayerConfig::Mode::Metal: return "metal";
    }
    return "box";
}

std::string FilterModeToString(FilterMode mode)
{
    switch (mode)
    {
    case FilterMode::Bypass: return "bypass";
    case FilterMode::LowPass: return "lowpass";
    case FilterMode::HighPass: return "highpass";
    case FilterMode::BandPass: return "bandpass";
    case FilterMode::LadderLowPass: return "ladderLowpass";
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
    case ModDestination::PulseWidth: return "pulseWidth";
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
    const auto root = ParseJSONObject(text);
    if (!root)
    {
        err = "invalid object";
        return false;
    }
    const auto it = root->find(key);
    if (it == root->end())
    {
        return true;
    }
    found = true;
    if (!it->is_object())
    {
        err = "key '" + key + "' must be an object";
        return false;
    }
    outObject = it->dump();
    return true;
}

bool ParseTopLevelObjectEntries(
    const std::string& objText,
    const std::function<bool(const std::string&, const std::string&)>& onEntry,
    std::string& err)
{
    // 目的: {"k": {...}, ...} 形式の top-level object を1段だけ走査する。
    const auto root = ParseJSONObject(objText);
    if (!root)
    {
        err = "invalid object";
        return false;
    }
    for (const auto& [entryKey, entryValue] : root->items())
    {
        if (!entryValue.is_object())
        {
            err = "expected object value for key '" + entryKey + "'";
            return false;
        }
        if (!onEntry(entryKey, entryValue.dump()))
        {
            return false;
        }
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
    WriteIndent(out, indent + 2); out << "\"bodyFreq\": " << d.bodyFreq << ",\n";
    WriteIndent(out, indent + 2); out << "\"bodyLevel\": " << d.bodyLevel << ",\n";
    WriteIndent(out, indent + 2); out << "\"bodyDecaySec\": " << d.bodyDecaySec << ",\n";
    WriteIndent(out, indent + 2); out << "\"pitchStart\": " << d.pitchStart << ",\n";
    WriteIndent(out, indent + 2); out << "\"pitchDecaySec\": " << d.pitchDecaySec << ",\n";
    WriteIndent(out, indent + 2); out << "\"transientLevel\": " << d.transientLevel << ",\n";
    WriteIndent(out, indent + 2); out << "\"transientDecaySec\": " << d.transientDecaySec << ",\n";
    WriteIndent(out, indent + 2); out << "\"noiseLevel\": " << d.noiseLevel << ",\n";
    WriteIndent(out, indent + 2); out << "\"snapLevel\": " << d.snapLevel << ",\n";
    WriteIndent(out, indent + 2); out << "\"snapDecaySec\": " << d.snapDecaySec << ",\n";
    WriteIndent(out, indent + 2); out << "\"metalLevel\": " << d.metalLevel << ",\n";
    WriteIndent(out, indent + 2); out << "\"airLevel\": " << d.airLevel << ",\n";
    WriteIndent(out, indent + 2); out << "\"decaySec\": " << d.decaySec << ",\n";
    WriteIndent(out, indent + 2); out << "\"hpCut\": " << d.hpCut << ",\n";
    WriteIndent(out, indent + 2); out << "\"lpCut\": " << d.lpCut << ",\n";
    WriteIndent(out, indent + 2); out << "\"drive\": " << d.drive << ",\n";
    WriteIndent(out, indent + 2); out << "\"noiseColor\": \"" << NoiseTypeToString((NoiseType)d.noiseColor) << "\",\n";
    WriteIndent(out, indent + 2); out << "\"velocityToTone\": " << d.velocityToTone << ",\n";
    WriteIndent(out, indent + 2); out << "\"velocityToDecay\": " << d.velocityToDecay << ",\n";
    WriteIndent(out, indent + 2); out << "\"humanizePitchCents\": " << d.humanizePitchCents << ",\n";
    WriteIndent(out, indent + 2); out << "\"humanizeDecayPct\": " << d.humanizeDecayPct << "\n";
    WriteIndent(out, indent); out << "}";
}

void WriteDrumBusConfig(std::ostream& out, const DrumBusConfig& bus, int indent)
{
    WriteIndent(out, indent); out << "\"drumBus\": {\n";
    WriteIndent(out, indent + 2); out << "\"enabled\": " << (bus.enabled ? "true" : "false") << ",\n";
    WriteIndent(out, indent + 2); out << "\"level\": " << bus.level << ",\n";
    WriteIndent(out, indent + 2); out << "\"attackTrim\": " << bus.attackTrim << ",\n";
    WriteIndent(out, indent + 2); out << "\"sustainLift\": " << bus.sustainLift << ",\n";
    WriteIndent(out, indent + 2); out << "\"glue\": " << bus.glue << ",\n";
    WriteIndent(out, indent + 2); out << "\"presenceCut\": " << bus.presenceCut << ",\n";
    WriteIndent(out, indent + 2); out << "\"lowTighten\": " << bus.lowTighten << ",\n";
    WriteIndent(out, indent + 2); out << "\"roomSend\": " << bus.roomSend << ",\n";
    WriteIndent(out, indent + 2); out << "\"driveTrim\": " << bus.driveTrim << "\n";
    WriteIndent(out, indent); out << "}";
}

bool ShouldWriteDrumBus(const DrumBusConfig& bus)
{
    return bus.enabled ||
        bus.level != 1.0 ||
        bus.attackTrim != 0.0 ||
        bus.sustainLift != 0.0 ||
        bus.glue != 0.0 ||
        bus.presenceCut != 0.0 ||
        bus.lowTighten != 0.0 ||
        bus.roomSend != 0.0 ||
        bus.driveTrim != 0.0;
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
    WriteIndent(out, indent + 4); out << "\"releaseSec\": " << m.env2.releaseSec << ",\n";
    WriteIndent(out, indent + 4); out << "\"curve\": " << m.env2.curve << "\n";
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

void WriteFmOperatorEnv(std::ostream& out, const char* key, const ModEnvelopeConfig& env, int indent)
{
    WriteIndent(out, indent); out << "\"" << key << "\": { ";
    out << "\"attackSec\": " << env.attackSec << ", ";
    out << "\"decaySec\": " << env.decaySec << ", ";
    out << "\"sustainLevel\": " << env.sustainLevel << ", ";
    out << "\"releaseSec\": " << env.releaseSec << ", ";
    out << "\"curve\": " << env.curve << " }";
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

template <typename ArpeggioT>
void WriteArpeggioConfig(std::ostream& out, const ArpeggioT& arpeggio, int indent)
{
    WriteIndent(out, indent); out << "\"arpeggio\": {\n";
    WriteIndent(out, indent + 2); out << "\"enabled\": " << (arpeggio.enabled ? "true" : "false") << ",\n";
    WriteIndent(out, indent + 2); out << "\"rateHz\": " << arpeggio.rateHz << ",\n";
    WriteIndent(out, indent + 2); out << "\"steps\": " << arpeggio.steps << ",\n";
    WriteIndent(out, indent + 2); out << "\"semitones\": [";
    for (size_t i = 0; i < arpeggio.semitones.size(); i++)
    {
        out << arpeggio.semitones[i];
        if (i + 1 < arpeggio.semitones.size())
        {
            out << ", ";
        }
    }
    out << "]\n";
    WriteIndent(out, indent); out << "}";
}

template <typename T>
void WriteWaveformLikeCommonFields(std::ostream& out, const T& v, int indent)
{
    WriteIndent(out, indent); out << "\"unisonVoices\": " << v.unisonVoices << ",\n";
    WriteIndent(out, indent); out << "\"unisonDetuneCents\": " << v.unisonDetuneCents << ",\n";
    WriteIndent(out, indent); out << "\"unisonSpread\": " << v.unisonSpread << ",\n";
    WriteIndent(out, indent); out << "\"subOscLevel\": " << v.subOscLevel << ",\n";
    WriteIndent(out, indent); out << "\"pulseWidth\": " << v.pulseWidth << ",\n";
    WriteIndent(out, indent); out << "\"hardSyncEnabled\": " << (v.hardSyncEnabled ? "true" : "false") << ",\n";
    WriteIndent(out, indent); out << "\"hardSyncRatio\": " << v.hardSyncRatio << ",\n";
    WriteIndent(out, indent); out << "\"ringModEnabled\": " << (v.ringModEnabled ? "true" : "false") << ",\n";
    WriteIndent(out, indent); out << "\"ringModRatio\": " << v.ringModRatio << ",\n";
    WriteIndent(out, indent); out << "\"ringModMix\": " << v.ringModMix << ",\n";
    WriteIndent(out, indent); out << "\"filter\": { \"mode\": \"" << FilterModeToString(v.filterMode) << "\", \"cutoffHz\": " << v.filterCutoffHz << ", \"resonance\": " << v.filterResonance << ", \"keytrack\": " << v.filterKeytrack << ", \"drive\": " << v.filterDrive << " },\n";
    WriteIndent(out, indent); out << "\"drive\": " << v.drive << ",\n";
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
            WriteWaveformLikeCommonFields(out, v, indent + 2);
            WriteArpeggioConfig(out, v.arpeggio, indent + 2); out << ",\n";
            WriteWaveformSmoothingConfig(out, v.smoothing, indent + 2); out << ",\n";
            WriteModulationConfig(out, v.modulation, indent + 2);
            out << "\n";
        }
        else if constexpr (std::is_same_v<T, AnalogConfig>)
        {
            WriteIndent(out, indent + 2); out << "\"type\": \"analog\",\n";
            WriteIndent(out, indent + 2); out << "\"wave\": \"" << WaveTypeToString(v.wave) << "\",\n";
            WriteWaveformLikeCommonFields(out, v, indent + 2);
            WriteIndent(out, indent + 2); out << "\"driftDepthCents\": " << v.driftDepthCents << ",\n";
            WriteIndent(out, indent + 2); out << "\"driftRateHz\": " << v.driftRateHz << ",\n";
            WriteArpeggioConfig(out, v.arpeggio, indent + 2); out << ",\n";
            WriteWaveformSmoothingConfig(out, v.smoothing, indent + 2); out << ",\n";
            WriteModulationConfig(out, v.modulation, indent + 2);
            out << "\n";
        }
        else if constexpr (std::is_same_v<T, NoiseConfig>)
        {
            WriteIndent(out, indent + 2); out << "\"type\": \"" << config::SourceKindToTypeName(config::SourceKind::Noise) << "\",\n";
            WriteIndent(out, indent + 2); out << "\"noise\": \"" << NoiseTypeToString(v.noise) << "\",\n";
            WriteIndent(out, indent + 2); out << "\"filter\": { \"mode\": \"" << FilterModeToString(v.filterMode) << "\", \"cutoffHz\": " << v.filterCutoffHz << ", \"resonance\": " << v.filterResonance << ", \"drive\": " << v.filterDrive << " }\n";
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
                WriteIndent(out, indent + 4); out << "{\n";
                WriteIndent(out, indent + 6); out << "\"wave\": \"" << WaveTypeToString(op.wave) << "\",\n";
                WriteIndent(out, indent + 6); out << "\"ratio\": " << op.ratio << ",\n";
                WriteIndent(out, indent + 6); out << "\"level\": " << op.level << ",\n";
                WriteIndent(out, indent + 6); out << "\"index\": " << op.index << ",\n";
                WriteFmOperatorEnv(out, "levelEnv", op.levelEnv, indent + 6); out << ",\n";
                WriteFmOperatorEnv(out, "indexEnv", op.indexEnv, indent + 6); out << "\n";
                WriteIndent(out, indent + 4); out << "}";
                if (i + 1 < 4) out << ",";
                out << "\n";
            }
            WriteIndent(out, indent + 2); out << "],\n";
            WriteIndent(out, indent + 2); out << "\"filter\": { \"mode\": \"" << FilterModeToString(v.filterMode) << "\", \"cutoffHz\": " << v.filterCutoffHz << ", \"resonance\": " << v.filterResonance << ", \"drive\": " << v.filterDrive << " },\n";
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
            if (ShouldWriteDrumBus(v.drumBus))
            {
                WriteDrumBusConfig(out, v.drumBus, indent + 2);
                out << ",\n";
            }
            if (v.velocityCeiling != 1.0 || v.velocityCurve != 1.0)
            {
                WriteIndent(out, indent + 2); out << "\"velocityCeiling\": " << v.velocityCeiling << ",\n";
                WriteIndent(out, indent + 2); out << "\"velocityCurve\": " << v.velocityCurve << ",\n";
            }
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

void WriteInstrumentSoundConfig(std::ostream& out, int ch, const InstrumentSoundConfig& cfg, bool withComma)
{
    WriteIndent(out, 4); out << "\"" << ch << "\": {\n";
    WriteIndent(out, 6); out << "\"amp\": " << cfg.amp << ",\n";
    WriteIndent(out, 6); out << "\"attackSec\": " << cfg.attackSec << ",\n";
    WriteIndent(out, 6); out << "\"decaySec\": " << cfg.decaySec << ",\n";
    WriteIndent(out, 6); out << "\"sustainLevel\": " << cfg.sustainLevel << ",\n";
    WriteIndent(out, 6); out << "\"releaseSec\": " << cfg.releaseSec << ",\n";
    WriteIndent(out, 6); out << "\"portamentoTimeSec\": " << cfg.portamentoTimeSec << ",\n";
    WriteIndent(out, 6); out << "\"layers\": {\n";
    const auto& attack = cfg.attackLayer;
    WriteIndent(out, 8); out << "\"attack\": { \"enabled\": " << (attack.enabled ? "true" : "false") << ", \"type\": \"" << AttackLayerTypeToString(attack.type) << "\", \"level\": " << attack.level << ", \"decaySec\": " << attack.decaySec << ", \"brightness\": " << attack.brightness << ", \"bodyMix\": " << attack.bodyMix << ", \"pitchOffsetSemis\": " << attack.pitchOffsetSemis << ", \"drive\": " << attack.drive << " },\n";
    const auto& bass = cfg.bassLayer;
    WriteIndent(out, 8); out << "\"bass\": { \"enabled\": " << (bass.enabled ? "true" : "false") << ", \"type\": \"" << BassLayerTypeToString(bass.type) << "\", \"level\": " << bass.level << ", \"subLevel\": " << bass.subLevel << ", \"bodyLevel\": " << bass.bodyLevel << ", \"gritLevel\": " << bass.gritLevel << ", \"cutoffHz\": " << bass.cutoffHz << ", \"drive\": " << bass.drive << ", \"pitchOffsetSemis\": " << bass.pitchOffsetSemis << ", \"velocityToDrive\": " << bass.velocityToDrive << ", \"focusHz\": " << bass.focusHz << ", \"focusLevel\": " << bass.focusLevel << ", \"bodySaturation\": " << bass.bodySaturation << ", \"gritTone\": " << bass.gritTone << ", \"attackBoost\": " << bass.attackBoost << ", \"attackDecaySec\": " << bass.attackDecaySec << " },\n";
    const auto& lead = cfg.leadLayer;
    WriteIndent(out, 8); out << "\"lead\": { \"enabled\": " << (lead.enabled ? "true" : "false") << ", \"type\": \"" << LeadLayerTypeToString(lead.type) << "\", \"level\": " << lead.level << ", \"edgeLevel\": " << lead.edgeLevel << ", \"bodyLevel\": " << lead.bodyLevel << ", \"detuneCents\": " << lead.detuneCents << ", \"pitchBendSemis\": " << lead.pitchBendSemis << ", \"bendDecaySec\": " << lead.bendDecaySec << ", \"attackBoost\": " << lead.attackBoost << ", \"attackDecaySec\": " << lead.attackDecaySec << ", \"drive\": " << lead.drive << ", \"characterLevel\": " << lead.characterLevel << ", \"characterTone\": " << lead.characterTone << ", \"biteLevel\": " << lead.biteLevel << ", \"biteDecaySec\": " << lead.biteDecaySec << ", \"wobbleDepthCents\": " << lead.wobbleDepthCents << ", \"wobbleRateHz\": " << lead.wobbleRateHz << " },\n";
    const auto& chord = cfg.chordLayer;
    WriteIndent(out, 8); out << "\"chord\": { \"enabled\": " << (chord.enabled ? "true" : "false") << ", \"level\": " << chord.level << ", \"intervalsSemis\": [";
    for (size_t v = 0; v < chord.intervalsSemis.size(); v++) { if (v > 0) out << ", "; out << chord.intervalsSemis[v]; }
    out << "], \"voiceLevels\": [";
    for (size_t v = 0; v < chord.voiceLevels.size(); v++) { if (v > 0) out << ", "; out << chord.voiceLevels[v]; }
    out << "], \"detuneCents\": " << chord.detuneCents << ", \"spread\": " << chord.spread << ", \"cutoffHz\": " << chord.cutoffHz << ", \"drive\": " << chord.drive << " },\n";
    const auto& pad = cfg.padLayer;
    WriteIndent(out, 8); out << "\"pad\": { \"enabled\": " << (pad.enabled ? "true" : "false") << ", \"level\": " << pad.level << ", \"octaveLevel\": " << pad.octaveLevel << ", \"detuneCents\": " << pad.detuneCents << ", \"spread\": " << pad.spread << ", \"fadeInSec\": " << pad.fadeInSec << ", \"brightness\": " << pad.brightness << ", \"motionDepth\": " << pad.motionDepth << ", \"motionRateHz\": " << pad.motionRateHz << ", \"cutoffHz\": " << pad.cutoffHz << ", \"drive\": " << pad.drive << " },\n";
    const auto& pluck = cfg.pluckLayer;
    WriteIndent(out, 8); out << "\"pluck\": { \"enabled\": " << (pluck.enabled ? "true" : "false") << ", \"level\": " << pluck.level << ", \"decaySec\": " << pluck.decaySec << ", \"brightness\": " << pluck.brightness << ", \"noiseMix\": " << pluck.noiseMix << ", \"pitchOffsetSemis\": " << pluck.pitchOffsetSemis << ", \"bodySend\": " << pluck.bodySend << ", \"drive\": " << pluck.drive << " },\n";
    const auto& stringLayer = cfg.stringLayer;
    WriteIndent(out, 8); out << "\"string\": { \"enabled\": " << (stringLayer.enabled ? "true" : "false") << ", \"level\": " << stringLayer.level << ", \"bowLevel\": " << stringLayer.bowLevel << ", \"detuneCents\": " << stringLayer.detuneCents << ", \"spread\": " << stringLayer.spread << ", \"fadeInSec\": " << stringLayer.fadeInSec << ", \"brightness\": " << stringLayer.brightness << ", \"motionDepth\": " << stringLayer.motionDepth << ", \"motionRateHz\": " << stringLayer.motionRateHz << ", \"bodySend\": " << stringLayer.bodySend << ", \"drive\": " << stringLayer.drive << " },\n";
    const auto& body = cfg.bodyLayer;
    WriteIndent(out, 8); out << "\"body\": { \"enabled\": " << (body.enabled ? "true" : "false") << ", \"mode\": \"" << BodyLayerModeToString(body.mode) << "\", \"mix\": " << body.mix << ", \"size\": " << body.size << ", \"tone\": " << body.tone << ", \"damping\": " << body.damping << ", \"stereo\": " << body.stereo << ", \"drive\": " << body.drive << " },\n";
    const auto& harmonic = cfg.harmonicLayer;
    WriteIndent(out, 8); out << "\"harmonic\": { \"enabled\": " << (harmonic.enabled ? "true" : "false") << ", \"level\": " << harmonic.level << ", \"harmonicLevels\": [";
    for (size_t h = 0; h < harmonic.harmonicLevels.size(); h++) { if (h > 0) out << ", "; out << harmonic.harmonicLevels[h]; }
    out << "], \"brightness\": " << harmonic.brightness << ", \"keyClick\": " << harmonic.keyClick << ", \"attackSec\": " << harmonic.attackSec << ", \"releaseDamp\": " << harmonic.releaseDamp << ", \"drive\": " << harmonic.drive << ", \"stereo\": " << harmonic.stereo << " },\n";
    const auto& powerChord = cfg.powerChordLayer;
    WriteIndent(out, 8); out << "\"powerChord\": { \"enabled\": " << (powerChord.enabled ? "true" : "false") << ", \"level\": " << powerChord.level << ", \"fifthLevel\": " << powerChord.fifthLevel << ", \"octaveLevel\": " << powerChord.octaveLevel << ", \"detuneCents\": " << powerChord.detuneCents << ", \"spread\": " << powerChord.spread << ", \"tone\": " << powerChord.tone << ", \"drive\": " << powerChord.drive << " },\n";
    const auto& chug = cfg.chugLayer;
    WriteIndent(out, 8); out << "\"chug\": { \"enabled\": " << (chug.enabled ? "true" : "false") << ", \"level\": " << chug.level << ", \"decaySec\": " << chug.decaySec << ", \"lowPunch\": " << chug.lowPunch << ", \"pick\": " << chug.pick << ", \"tone\": " << chug.tone << ", \"tightness\": " << chug.tightness << ", \"drive\": " << chug.drive << " },\n";
    const auto& ampCab = cfg.ampCabLayer;
    WriteIndent(out, 8); out << "\"ampCab\": { \"enabled\": " << (ampCab.enabled ? "true" : "false") << ", \"drive\": " << ampCab.drive << ", \"tone\": " << ampCab.tone << ", \"cabLow\": " << ampCab.cabLow << ", \"cabHigh\": " << ampCab.cabHigh << ", \"presence\": " << ampCab.presence << ", \"output\": " << ampCab.output << " }\n";
    WriteIndent(out, 6); out << "},\n";
    if (cfg.expressionMap.enabled)
    {
        const auto& map = cfg.expressionMap;
        WriteIndent(out, 6); out << "\"expressionMap\": {\n";
        WriteIndent(out, 8); out << "\"enabled\": true,\n";
        WriteIndent(out, 8); out << "\"velocityCurve\": " << map.velocityCurve << ",\n";
        WriteIndent(out, 8); out << "\"velocityToAmp\": " << map.velocityToAmp << ",\n";
        WriteIndent(out, 8); out << "\"velocityToBrightness\": " << map.velocityToBrightness << ",\n";
        WriteIndent(out, 8); out << "\"velocityToFmIndex\": " << map.velocityToFmIndex << ",\n";
        WriteIndent(out, 8); out << "\"velocityToAttack\": " << map.velocityToAttack << ",\n";
        WriteIndent(out, 8); out << "\"velocityToBass\": " << map.velocityToBass << ",\n";
        WriteIndent(out, 8); out << "\"velocityToLead\": " << map.velocityToLead << ",\n";
        WriteIndent(out, 8); out << "\"velocityToChord\": " << map.velocityToChord << ",\n";
        WriteIndent(out, 8); out << "\"velocityToPad\": " << map.velocityToPad << ",\n";
        WriteIndent(out, 8); out << "\"velocityToPluck\": " << map.velocityToPluck << ",\n";
        WriteIndent(out, 8); out << "\"velocityToString\": " << map.velocityToString << ",\n";
        WriteIndent(out, 8); out << "\"velocityToBody\": " << map.velocityToBody << ",\n";
        WriteIndent(out, 8); out << "\"modWheelToBrightness\": " << map.modWheelToBrightness << ",\n";
        WriteIndent(out, 8); out << "\"modWheelToPad\": " << map.modWheelToPad << ",\n";
        WriteIndent(out, 8); out << "\"modWheelToString\": " << map.modWheelToString << ",\n";
        WriteIndent(out, 8); out << "\"pressureToDrive\": " << map.pressureToDrive << ",\n";
        WriteIndent(out, 8); out << "\"pressureToFilterDrive\": " << map.pressureToFilterDrive << ",\n";
        WriteIndent(out, 8); out << "\"cc74ToBrightness\": " << map.cc74ToBrightness << ",\n";
        WriteIndent(out, 8); out << "\"cc74ToPadBrightness\": " << map.cc74ToPadBrightness << ",\n";
        WriteIndent(out, 8); out << "\"cc74ToStringBrightness\": " << map.cc74ToStringBrightness << "\n";
        WriteIndent(out, 6); out << "},\n";
    }
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
