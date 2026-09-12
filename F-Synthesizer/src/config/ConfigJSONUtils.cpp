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

const Json* AsJSONObject(const Json& value)
{
    return value.is_object() ? &value : nullptr;
}
} // namespace

// File input is parsed once at the boundary; field readers use parsed objects.
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

std::optional<std::string> ReadJSONString(const Json& text, const std::string& key)
{
    const auto root = AsJSONObject(text);
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

std::optional<int> ReadJSONInt(const Json& text, const std::string& key)
{
    const auto root = AsJSONObject(text);
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

std::optional<double> ReadJSONDouble(const Json& text, const std::string& key)
{
    const auto root = AsJSONObject(text);
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

std::optional<bool> ReadJSONBool(const Json& text, const std::string& key)
{
    const auto root = AsJSONObject(text);
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
    if (name == "bell") { outType = DrumType::Bell; return true; }
    if (name == "shaker") { outType = DrumType::Shaker; return true; }
    if (name == "scrape") { outType = DrumType::Scrape; return true; }
    if (name == "whistle") { outType = DrumType::Whistle; return true; }
    if (name == "woodblock") { outType = DrumType::Woodblock; return true; }
    if (name == "cuica") { outType = DrumType::Cuica; return true; }
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
    if (name == "vocal")
    {
        outMode = FilterMode::Vocal;
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
    case DrumType::Bell: return "bell";
    case DrumType::Shaker: return "shaker";
    case DrumType::Scrape: return "scrape";
    case DrumType::Whistle: return "whistle";
    case DrumType::Woodblock: return "woodblock";
    case DrumType::Cuica: return "cuica";
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
    case FilterMode::Vocal: return "vocal";
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

bool ExtractObjectForKey(const Json& text, const std::string& key, Json& outObject, bool& found, std::string& err)
{
    found = false;
    const auto root = AsJSONObject(text);
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
    outObject = *it;
    return true;
}

bool ParseTopLevelObjectEntries(
    const Json& objText,
    const std::function<bool(const std::string&, const Json&)>& onEntry,
    std::string& err)
{
    // 目的: {"k": {...}, ...} 形式の top-level object を1段だけ走査する。
    const auto root = AsJSONObject(objText);
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
        if (!onEntry(entryKey, entryValue))
        {
            return false;
        }
    }

    return true;
}

} // namespace config::internal
