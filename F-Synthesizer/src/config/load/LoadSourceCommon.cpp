#include "Internal.h"

#include <algorithm>
#include <iomanip>
#include <optional>
#include <sstream>
#include <string_view>

#include "../ConfigFileInternal.h"

#include "third_party/nlohmann/json.hpp"
#include "config/SourceRegistry.h"

namespace config::internal::load
{
namespace
{
using Json = nlohmann::json;

std::optional<Json> ParseJSONObject(const std::string& text)
{
    Json parsed = Json::parse(text, nullptr, false);
    if (parsed.is_discarded() || !parsed.is_object())
    {
        return std::nullopt;
    }
    return parsed;
}

std::optional<Json> ParseJSONArray(const std::string& text)
{
    Json parsed = Json::parse(text, nullptr, false);
    if (parsed.is_discarded() || !parsed.is_array())
    {
        return std::nullopt;
    }
    return parsed;
}

const char* RetriggerToString(SourceLifecycleRetrigger retrigger)
{
    switch (retrigger)
    {
    case SourceLifecycleRetrigger::Restart: return "restart";
    case SourceLifecycleRetrigger::Stack: return "stack";
    }
    return "restart";
}

bool TryParseRetrigger(const std::string& name, SourceLifecycleRetrigger& out)
{
    if (name == "restart")
    {
        out = SourceLifecycleRetrigger::Restart;
        return true;
    }
    if (name == "stack")
    {
        out = SourceLifecycleRetrigger::Stack;
        return true;
    }
    return false;
}

const char* StealToString(SourceLifecycleSteal steal)
{
    switch (steal)
    {
    case SourceLifecycleSteal::Oldest: return "oldest";
    case SourceLifecycleSteal::RejectNew: return "rejectNew";
    }
    return "oldest";
}

bool TryParseSteal(const std::string& name, SourceLifecycleSteal& out)
{
    if (name == "oldest")
    {
        out = SourceLifecycleSteal::Oldest;
        return true;
    }
    if (name == "rejectNew")
    {
        out = SourceLifecycleSteal::RejectNew;
        return true;
    }
    return false;
}

std::string FormatSchemaValue(double value, SourceParameterType type)
{
    if (type == SourceParameterType::Int)
    {
        return std::to_string(static_cast<int>(value));
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << value;
    return oss.str();
}

bool ValidateSchemaRange(
    const char* prefix,
    const SourceParameterSchemaEntry& e,
    double value,
    bool optionalWhenNonPositive,
    std::string& err)
{
    if (optionalWhenNonPositive && value <= 0.0)
    {
        return true;
    }

    if (value < e.minValue || value > e.maxValue)
    {
        err = std::string(prefix) + "." + std::string(e.id) + " must be in range "
            + FormatSchemaValue(e.minValue, e.type) + ".." + FormatSchemaValue(e.maxValue, e.type);
        if (optionalWhenNonPositive)
        {
            err += " when specified";
        }
        return false;
    }
    return true;
}

template <typename T>
bool WaveformLikeSchemaValue(const T& cfg, const SourceParameterSchemaEntry& e, double& outValue)
{
    if (std::string_view(e.id) == "unisonVoices") { outValue = static_cast<double>(cfg.unisonVoices); return true; }
    if (std::string_view(e.id) == "unisonDetuneCents") { outValue = cfg.unisonDetuneCents; return true; }
    if (std::string_view(e.id) == "unisonSpread") { outValue = cfg.unisonSpread; return true; }
    if (std::string_view(e.id) == "subOscLevel") { outValue = cfg.subOscLevel; return true; }
    if (std::string_view(e.id) == "pulseWidth") { outValue = cfg.pulseWidth; return true; }
    if (std::string_view(e.id) == "hardSyncEnabled") { outValue = cfg.hardSyncEnabled ? 1.0 : 0.0; return true; }
    if (std::string_view(e.id) == "hardSyncRatio") { outValue = cfg.hardSyncRatio; return true; }
    if (std::string_view(e.id) == "ringModEnabled") { outValue = cfg.ringModEnabled ? 1.0 : 0.0; return true; }
    if (std::string_view(e.id) == "ringModRatio") { outValue = cfg.ringModRatio; return true; }
    if (std::string_view(e.id) == "ringModMix") { outValue = cfg.ringModMix; return true; }
    if (std::string_view(e.id) == "filterCutoffHz") { outValue = cfg.filterCutoffHz; return true; }
    if (std::string_view(e.id) == "filterResonance") { outValue = cfg.filterResonance; return true; }
    if (std::string_view(e.id) == "filterKeytrack") { outValue = cfg.filterKeytrack; return true; }
    return false;
}

bool WaveformSchemaValue(const WaveformConfig& wf, const SourceParameterSchemaEntry& e, double& outValue)
{
    return WaveformLikeSchemaValue(wf, e, outValue);
}

bool AnalogSchemaValue(const AnalogConfig& analog, const SourceParameterSchemaEntry& e, double& outValue)
{
    if (WaveformLikeSchemaValue(analog, e, outValue)) { return true; }
    if (std::string_view(e.id) == "driftDepthCents") { outValue = analog.driftDepthCents; return true; }
    if (std::string_view(e.id) == "driftRateHz") { outValue = analog.driftRateHz; return true; }
    return false;
}

bool FmSchemaValue(const FmConfig& fm, const SourceParameterSchemaEntry& e, double& outValue)
{
    if (std::string_view(e.id) == "algorithm") { outValue = static_cast<double>(fm.algorithm); return true; }
    if (std::string_view(e.id) == "feedback") { outValue = fm.feedback; return true; }
    if (std::string_view(e.id) == "op1Wave") { outValue = static_cast<double>(fm.ops[0].wave); return true; }
    if (std::string_view(e.id) == "op1Ratio") { outValue = fm.ops[0].ratio; return true; }
    if (std::string_view(e.id) == "op1Level") { outValue = fm.ops[0].level; return true; }
    if (std::string_view(e.id) == "op1Index") { outValue = fm.ops[0].index; return true; }
    if (std::string_view(e.id) == "op2Wave") { outValue = static_cast<double>(fm.ops[1].wave); return true; }
    if (std::string_view(e.id) == "op2Ratio") { outValue = fm.ops[1].ratio; return true; }
    if (std::string_view(e.id) == "op2Level") { outValue = fm.ops[1].level; return true; }
    if (std::string_view(e.id) == "op2Index") { outValue = fm.ops[1].index; return true; }
    if (std::string_view(e.id) == "op3Wave") { outValue = static_cast<double>(fm.ops[2].wave); return true; }
    if (std::string_view(e.id) == "op3Ratio") { outValue = fm.ops[2].ratio; return true; }
    if (std::string_view(e.id) == "op3Level") { outValue = fm.ops[2].level; return true; }
    if (std::string_view(e.id) == "op3Index") { outValue = fm.ops[2].index; return true; }
    if (std::string_view(e.id) == "op4Wave") { outValue = static_cast<double>(fm.ops[3].wave); return true; }
    if (std::string_view(e.id) == "op4Ratio") { outValue = fm.ops[3].ratio; return true; }
    if (std::string_view(e.id) == "op4Level") { outValue = fm.ops[3].level; return true; }
    if (std::string_view(e.id) == "op4Index") { outValue = fm.ops[3].index; return true; }
    if (std::string_view(e.id) == "filterCutoffHz") { outValue = fm.filterCutoffHz; return true; }
    if (std::string_view(e.id) == "filterResonance") { outValue = fm.filterResonance; return true; }
    return false;
}

bool DrumSchemaValue(const DrumConfig& drum, const SourceParameterSchemaEntry& e, double& outValue)
{
    if (std::string_view(e.id) == "drumType") { outValue = static_cast<double>(drum.type); return true; }
    if (std::string_view(e.id) == "gain") { outValue = drum.gain; return true; }
    if (std::string_view(e.id) == "bodyFreq") { outValue = drum.bodyFreq; return true; }
    if (std::string_view(e.id) == "bodyLevel") { outValue = drum.bodyLevel; return true; }
    if (std::string_view(e.id) == "bodyDecaySec") { outValue = drum.bodyDecaySec; return true; }
    if (std::string_view(e.id) == "pitchStart") { outValue = drum.pitchStart; return true; }
    if (std::string_view(e.id) == "pitchDecaySec") { outValue = drum.pitchDecaySec; return true; }
    if (std::string_view(e.id) == "transientLevel") { outValue = drum.transientLevel; return true; }
    if (std::string_view(e.id) == "transientDecaySec") { outValue = drum.transientDecaySec; return true; }
    if (std::string_view(e.id) == "noiseLevel") { outValue = drum.noiseLevel; return true; }
    if (std::string_view(e.id) == "snapLevel") { outValue = drum.snapLevel; return true; }
    if (std::string_view(e.id) == "snapDecaySec") { outValue = drum.snapDecaySec; return true; }
    if (std::string_view(e.id) == "metalLevel") { outValue = drum.metalLevel; return true; }
    if (std::string_view(e.id) == "airLevel") { outValue = drum.airLevel; return true; }
    if (std::string_view(e.id) == "decaySec") { outValue = drum.decaySec; return true; }
    if (std::string_view(e.id) == "hpCut") { outValue = drum.hpCut; return true; }
    if (std::string_view(e.id) == "lpCut") { outValue = drum.lpCut; return true; }
    if (std::string_view(e.id) == "drive") { outValue = drum.drive; return true; }
    if (std::string_view(e.id) == "noiseColor") { outValue = static_cast<double>(drum.noiseColor); return true; }
    if (std::string_view(e.id) == "velocityToTone") { outValue = drum.velocityToTone; return true; }
    if (std::string_view(e.id) == "velocityToDecay") { outValue = drum.velocityToDecay; return true; }
    if (std::string_view(e.id) == "humanizePitchCents") { outValue = drum.humanizePitchCents; return true; }
    if (std::string_view(e.id) == "humanizeDecayPct") { outValue = drum.humanizeDecayPct; return true; }
    return false;
}

bool NoiseSchemaValue(const NoiseConfig& noise, const SourceParameterSchemaEntry& e, double& outValue)
{
    if (std::string_view(e.id) == "noise") { outValue = static_cast<double>(noise.noise); return true; }
    if (std::string_view(e.id) == "filterCutoffHz") { outValue = noise.filterCutoffHz; return true; }
    if (std::string_view(e.id) == "filterResonance") { outValue = noise.filterResonance; return true; }
    return false;
}

bool IsDrumOptionalWhenNonPositive(std::string_view id)
{
    return id == "bodyFreq"
        || id == "bodyLevel"
        || id == "bodyDecaySec"
        || id == "pitchStart"
        || id == "pitchDecaySec"
        || id == "transientLevel"
        || id == "transientDecaySec"
        || id == "noiseLevel"
        || id == "snapLevel"
        || id == "snapDecaySec"
        || id == "metalLevel"
        || id == "airLevel"
        || id == "decaySec"
        || id == "hpCut"
        || id == "lpCut";
}

template <typename SchemaValueFn, typename OptionalPredicateFn>
bool ValidateSourceBySchemaCommon(
    SourceKind sourceKind,
    const char* sourcePrefix,
    bool requireNonEmptySchema,
    const SchemaValueFn& schemaValueFn,
    const OptionalPredicateFn& optionalPredicateFn,
    std::string& err)
{
    const SourceParameterSchemaEntry* schema = nullptr;
    size_t schemaCount = 0;
    if (!TryGetParameterSchema(sourceKind, schema, schemaCount) || schema == nullptr || (requireNonEmptySchema && schemaCount == 0))
    {
        err = std::string(sourcePrefix) + " schema is not defined";
        return false;
    }

    for (size_t i = 0; i < schemaCount; i++)
    {
        const SourceParameterSchemaEntry& e = schema[i];
        double value = 0.0;
        if (!schemaValueFn(e, value))
        {
            err = std::string(sourcePrefix) + " schema has unknown id: " + std::string(e.id);
            return false;
        }
        if (!ValidateSchemaRange(sourcePrefix, e, value, optionalPredicateFn(std::string_view(e.id)), err))
        {
            return false;
        }
    }
    return true;
}

template <typename T>
bool ParseWaveformLikeCommonFieldsImpl(const std::string& text, T& cfg, std::string& err)
{
    if (auto v = ReadJSONInt(text, "unisonVoices")) { cfg.unisonVoices = *v; }
    if (auto v = ReadJSONDouble(text, "unisonDetuneCents")) { cfg.unisonDetuneCents = *v; }
    if (auto v = ReadJSONDouble(text, "unisonSpread")) { cfg.unisonSpread = *v; }
    if (auto v = ReadJSONDouble(text, "subOscLevel")) { cfg.subOscLevel = *v; }
    if (auto v = ReadJSONDouble(text, "pulseWidth")) { cfg.pulseWidth = std::clamp(*v, 0.05, 0.95); }
    if (auto v = ReadJSONBool(text, "hardSyncEnabled")) { cfg.hardSyncEnabled = *v; }
    if (auto v = ReadJSONDouble(text, "hardSyncRatio")) { cfg.hardSyncRatio = std::clamp(*v, 0.5, 8.0); }
    if (auto v = ReadJSONBool(text, "ringModEnabled")) { cfg.ringModEnabled = *v; }
    if (auto v = ReadJSONDouble(text, "ringModRatio")) { cfg.ringModRatio = std::clamp(*v, 0.125, 16.0); }
    if (auto v = ReadJSONDouble(text, "ringModMix")) { cfg.ringModMix = std::clamp(*v, 0.0, 1.0); }
    if (auto v = ReadJSONString(text, "filterMode"))
    {
        FilterMode mode{};
        if (!TryParseFilterMode(*v, mode)) { err = "invalid filterMode: " + *v; return false; }
        cfg.filterMode = mode;
    }
    if (auto v = ReadJSONDouble(text, "filterCutoffHz")) { cfg.filterCutoffHz = *v; }
    if (auto v = ReadJSONDouble(text, "filterResonance")) { cfg.filterResonance = *v; }
    if (auto v = ReadJSONDouble(text, "filterKeytrack")) { cfg.filterKeytrack = *v; }
    if (auto v = ReadJSONDouble(text, "drive")) { cfg.drive = std::clamp(*v, 0.0, 1.0); }

    std::string arpeggioObj;
    bool foundArpeggio = false;
    if (!ExtractObjectForKey(text, "arpeggio", arpeggioObj, foundArpeggio, err)) { return false; }
    if (foundArpeggio)
    {
        if (auto v = ReadJSONBool(arpeggioObj, "enabled")) { cfg.arpeggio.enabled = *v; }
        if (auto v = ReadJSONDouble(arpeggioObj, "rateHz")) { cfg.arpeggio.rateHz = std::clamp(*v, 0.5, 100.0); }
        if (auto v = ReadJSONInt(arpeggioObj, "steps")) { cfg.arpeggio.steps = std::clamp(*v, 1, 8); }

        std::string semitonesArray;
        bool foundSemitones = false;
        if (!ExtractArrayForKey(arpeggioObj, "semitones", semitonesArray, foundSemitones, err)) { return false; }
        if (foundSemitones)
        {
            if (!ParseTopLevelIntArrayElements(semitonesArray, [&](size_t semitoneIndex, int value) {
                if (semitoneIndex >= cfg.arpeggio.semitones.size()) { return true; }
                cfg.arpeggio.semitones[semitoneIndex] = std::clamp(value, -24, 24);
                return true;
                }, err))
            {
                return false;
            }
        }
    }

    std::string smoothingObj;
    bool foundSmoothing = false;
    if (!ExtractObjectForKey(text, "smoothing", smoothingObj, foundSmoothing, err)) { return false; }
    if (foundSmoothing)
    {
        if (!ParseWaveformSmoothingObject(smoothingObj, cfg.smoothing))
        {
            err = "invalid smoothing object";
            return false;
        }
    }

    std::string modulationObj;
    bool foundModulation = false;
    if (!ExtractObjectForKey(text, "modulation", modulationObj, foundModulation, err)) { return false; }
    if (foundModulation)
    {
        if (!ParseModulationObject(modulationObj, cfg.modulation, err)) { return false; }
    }
    return true;
}
} // namespace

bool ValidateLifecycleContract(const std::string& sourceObjText, SourceKind sourceKind, std::string& err)
{
    std::string lifecycleObj;
    bool foundLifecycle = false;
    if (!ExtractObjectForKey(sourceObjText, "lifecycle", lifecycleObj, foundLifecycle, err))
    {
        return false;
    }
    if (!foundLifecycle)
    {
        return true;
    }

    const SourceLifecyclePolicy expected = SourceLifecycleOf(sourceKind);
    if (auto v = ReadJSONString(lifecycleObj, "retrigger"))
    {
        SourceLifecycleRetrigger parsed{};
        if (!TryParseRetrigger(*v, parsed))
        {
            err = "source.lifecycle.retrigger must be restart/stack";
            return false;
        }
        if (parsed != expected.retrigger)
        {
            err = "source.lifecycle.retrigger must be '" + std::string(RetriggerToString(expected.retrigger))
                + "' for source.type=" + std::string(SourceKindToTypeName(sourceKind));
            return false;
        }
    }
    if (auto v = ReadJSONString(lifecycleObj, "steal"))
    {
        SourceLifecycleSteal parsed{};
        if (!TryParseSteal(*v, parsed))
        {
            err = "source.lifecycle.steal must be oldest/rejectNew";
            return false;
        }
        if (parsed != expected.steal)
        {
            err = "source.lifecycle.steal must be '" + std::string(StealToString(expected.steal))
                + "' for source.type=" + std::string(SourceKindToTypeName(sourceKind));
            return false;
        }
    }
    if (auto v = ReadJSONBool(lifecycleObj, "noteOffEntersRelease"))
    {
        if (*v != expected.noteOffEntersRelease)
        {
            err = "source.lifecycle.noteOffEntersRelease must be "
                + std::string(expected.noteOffEntersRelease ? "true" : "false")
                + " for source.type=" + std::string(SourceKindToTypeName(sourceKind));
            return false;
        }
    }
    if (auto v = ReadJSONBool(lifecycleObj, "oneShotEndsAutomatically"))
    {
        if (*v != expected.oneShotEndsAutomatically)
        {
            err = "source.lifecycle.oneShotEndsAutomatically must be "
                + std::string(expected.oneShotEndsAutomatically ? "true" : "false")
                + " for source.type=" + std::string(SourceKindToTypeName(sourceKind));
            return false;
        }
    }

    return true;
}

bool ParseWaveformCommonFields(const std::string& text, WaveformConfig& cfg, std::string& err)
{
    return ParseWaveformLikeCommonFieldsImpl(text, cfg, err);
}

bool ParseAnalogCommonFields(const std::string& text, AnalogConfig& cfg, std::string& err)
{
    return ParseWaveformLikeCommonFieldsImpl(text, cfg, err);
}

bool ValidateNoiseBySchema(const NoiseConfig& noise, std::string& err)
{
    return ValidateSourceBySchemaCommon(
        SourceKind::Noise,
        "noise",
        true,
        [&](const SourceParameterSchemaEntry& e, double& outValue) { return NoiseSchemaValue(noise, e, outValue); },
        [&](std::string_view) { return false; },
        err);
}

bool ValidateWaveformBySchema(const WaveformConfig& wf, std::string& err)
{
    return ValidateSourceBySchemaCommon(
        SourceKind::Waveform,
        "waveform",
        false,
        [&](const SourceParameterSchemaEntry& e, double& outValue) { return WaveformSchemaValue(wf, e, outValue); },
        [&](std::string_view) { return false; },
        err);
}

bool ValidateAnalogBySchema(const AnalogConfig& analog, std::string& err)
{
    return ValidateSourceBySchemaCommon(
        SourceKind::Analog,
        "analog",
        false,
        [&](const SourceParameterSchemaEntry& e, double& outValue) { return AnalogSchemaValue(analog, e, outValue); },
        [&](std::string_view) { return false; },
        err);
}

bool ValidateFmBySchema(const FmConfig& fm, std::string& err)
{
    return ValidateSourceBySchemaCommon(
        SourceKind::Fm,
        "fm",
        false,
        [&](const SourceParameterSchemaEntry& e, double& outValue) { return FmSchemaValue(fm, e, outValue); },
        [&](std::string_view) { return false; },
        err);
}

bool ValidateDrumBySchema(const DrumConfig& drum, std::string& err)
{
    if (!ValidateSourceBySchemaCommon(
        SourceKind::Drum,
        "drum",
        false,
        [&](const SourceParameterSchemaEntry& e, double& outValue) { return DrumSchemaValue(drum, e, outValue); },
        [&](std::string_view id) { return IsDrumOptionalWhenNonPositive(id); },
        err))
    {
        return false;
    }

    if (drum.hpCut > 0.0 && drum.lpCut > 0.0 && drum.hpCut >= drum.lpCut)
    {
        err = "drum.hpCut must be less than drum.lpCut when both are specified";
        return false;
    }
    return true;
}

bool ParseDrumConfigObject(const std::string& text, DrumConfig& drum, std::string& err)
{
    if (auto t = ReadJSONString(text, "drumType"))
    {
        DrumType dt{};
        if (!TryParseDrumType(*t, dt))
        {
            err = "invalid drumType: " + *t;
            return false;
        }
        drum.type = dt;
    }
    if (auto v = ReadJSONDouble(text, "gain")) drum.gain = *v;
    if (auto v = ReadJSONDouble(text, "bodyFreq")) drum.bodyFreq = *v;
    if (auto v = ReadJSONDouble(text, "bodyLevel")) drum.bodyLevel = *v;
    if (auto v = ReadJSONDouble(text, "bodyDecaySec")) drum.bodyDecaySec = *v;
    if (auto v = ReadJSONDouble(text, "pitchStart")) drum.pitchStart = *v;
    if (auto v = ReadJSONDouble(text, "pitchDecaySec")) drum.pitchDecaySec = *v;
    if (auto v = ReadJSONDouble(text, "transientLevel")) drum.transientLevel = *v;
    if (auto v = ReadJSONDouble(text, "transientDecaySec")) drum.transientDecaySec = *v;
    if (auto v = ReadJSONDouble(text, "noiseLevel")) drum.noiseLevel = *v;
    if (auto v = ReadJSONDouble(text, "snapLevel")) drum.snapLevel = *v;
    if (auto v = ReadJSONDouble(text, "snapDecaySec")) drum.snapDecaySec = *v;
    if (auto v = ReadJSONDouble(text, "metalLevel")) drum.metalLevel = *v;
    if (auto v = ReadJSONDouble(text, "airLevel")) drum.airLevel = *v;
    if (auto v = ReadJSONDouble(text, "decaySec")) drum.decaySec = *v;
    if (auto v = ReadJSONDouble(text, "hpCut")) drum.hpCut = *v;
    if (auto v = ReadJSONDouble(text, "lpCut")) drum.lpCut = *v;
    if (auto v = ReadJSONDouble(text, "drive")) drum.drive = std::clamp(*v, 0.0, 1.0);
    if (auto v = ReadJSONDouble(text, "velocityToTone")) drum.velocityToTone = std::clamp(*v, 0.0, 1.0);
    if (auto v = ReadJSONDouble(text, "velocityToDecay")) drum.velocityToDecay = std::clamp(*v, -1.0, 1.0);
    if (auto v = ReadJSONDouble(text, "humanizePitchCents")) drum.humanizePitchCents = std::clamp(*v, 0.0, 50.0);
    if (auto v = ReadJSONDouble(text, "humanizeDecayPct")) drum.humanizeDecayPct = std::clamp(*v, 0.0, 1.0);
    if (auto v = ReadJSONString(text, "noiseColor"))
    {
        NoiseType n{};
        if (!TryParseNoiseType(*v, n))
        {
            err = "invalid noiseColor: " + *v;
            return false;
        }
        drum.noiseColor = static_cast<int>(n);
    }
    return true;
}

bool ExtractArrayForKey(const std::string& text, const std::string& key, std::string& outArray, bool& found, std::string& err)
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
    if (!it->is_array())
    {
        err = "key '" + key + "' must be an array";
        return false;
    }

    found = true;
    outArray = it->dump();
    return true;
}

bool ParseTopLevelArrayObjectEntries(
    const std::string& arrText,
    const std::function<bool(size_t, const std::string&)>& onEntry,
    std::string& err)
{
    const auto root = ParseJSONArray(arrText);
    if (!root)
    {
        err = "invalid array";
        return false;
    }
    size_t index = 0;
    for (const auto& item : *root)
    {
        if (!item.is_object())
        {
            err = "array item must be object";
            return false;
        }
        if (!onEntry(index, item.dump()))
        {
            return false;
        }
        ++index;
    }
    return true;
}

bool ParseTopLevelIntArrayElements(
    const std::string& arrText,
    const std::function<bool(size_t, int)>& onElement,
    std::string& err)
{
    const auto root = ParseJSONArray(arrText);
    if (!root)
    {
        err = "invalid integer array";
        return false;
    }
    size_t index = 0;
    for (const auto& item : *root)
    {
        if (!item.is_number_integer())
        {
            err = "integer array contains non-integer value";
            return false;
        }
        if (!onElement(index, item.get<int>()))
        {
            return false;
        }
        ++index;
    }
    return true;
}

bool ValidateSmoothingSupport(const std::string& sourceObjText, SourceKind sourceKind, std::string& err)
{
    std::string smoothingObj;
    bool foundSmoothing = false;
    if (!ExtractObjectForKey(sourceObjText, "smoothing", smoothingObj, foundSmoothing, err))
    {
        return false;
    }
    if (!foundSmoothing)
    {
        return true;
    }
    if (sourceKind == SourceKind::Waveform || sourceKind == SourceKind::Analog)
    {
        return true;
    }

    err = "source.smoothing is allowed only for source.type=waveform/analog";
    return false;
}
} // namespace config::internal::load
