#include "Internal.h"

#include <iomanip>
#include <sstream>

#include "../ConfigFileInternal.h"

#include "config/SourceRegistry.h"

namespace config::internal::load
{
namespace
{
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

bool ValidateLifecycleContract(
    const std::string& sourceObjText,
    SourceKind sourceKind,
    std::string& err)
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

bool WaveformSchemaValue(const WaveformConfig& wf, const SourceParameterSchemaEntry& e, double& outValue)
{
    if (std::string_view(e.id) == "unisonVoices") { outValue = static_cast<double>(wf.unisonVoices); return true; }
    if (std::string_view(e.id) == "unisonDetuneCents") { outValue = wf.unisonDetuneCents; return true; }
    if (std::string_view(e.id) == "unisonSpread") { outValue = wf.unisonSpread; return true; }
    if (std::string_view(e.id) == "subOscLevel") { outValue = wf.subOscLevel; return true; }
    if (std::string_view(e.id) == "filterCutoffHz") { outValue = wf.filterCutoffHz; return true; }
    if (std::string_view(e.id) == "filterResonance") { outValue = wf.filterResonance; return true; }
    return false;
}

bool FmSchemaValue(const FmConfig& fm, const SourceParameterSchemaEntry& e, double& outValue)
{
    if (std::string_view(e.id) == "carrierWave") { outValue = static_cast<double>(fm.carrierWave); return true; }
    if (std::string_view(e.id) == "modWave") { outValue = static_cast<double>(fm.modWave); return true; }
    if (std::string_view(e.id) == "carrierRatio") { outValue = fm.carrierRatio; return true; }
    if (std::string_view(e.id) == "modRatio") { outValue = fm.modRatio; return true; }
    if (std::string_view(e.id) == "index") { outValue = fm.index; return true; }
    if (std::string_view(e.id) == "outLevel") { outValue = fm.outLevel; return true; }
    return false;
}

bool DrumSchemaValue(const DrumConfig& drum, const SourceParameterSchemaEntry& e, double& outValue)
{
    if (std::string_view(e.id) == "drumType") { outValue = static_cast<double>(drum.type); return true; }
    if (std::string_view(e.id) == "gain") { outValue = drum.gain; return true; }
    if (std::string_view(e.id) == "baseFreq") { outValue = drum.baseFreq; return true; }
    if (std::string_view(e.id) == "pitchDrop") { outValue = drum.pitchDrop; return true; }
    if (std::string_view(e.id) == "pitchDecaySec") { outValue = drum.pitchDecaySec; return true; }
    if (std::string_view(e.id) == "toneFreq") { outValue = drum.toneFreq; return true; }
    if (std::string_view(e.id) == "toneLevel") { outValue = drum.toneLevel; return true; }
    if (std::string_view(e.id) == "noiseLevel") { outValue = drum.noiseLevel; return true; }
    if (std::string_view(e.id) == "hpCut") { outValue = drum.hpCut; return true; }
    if (std::string_view(e.id) == "lpCut") { outValue = drum.lpCut; return true; }
    if (std::string_view(e.id) == "toneWave") { outValue = static_cast<double>(drum.toneWave); return true; }
    if (std::string_view(e.id) == "noiseType") { outValue = static_cast<double>(drum.noiseType); return true; }
    return false;
}

bool IsDrumOptionalWhenNonPositive(std::string_view id)
{
    return id == "baseFreq"
        || id == "pitchDrop"
        || id == "pitchDecaySec"
        || id == "toneFreq"
        || id == "toneLevel"
        || id == "noiseLevel"
        || id == "hpCut"
        || id == "lpCut";
}

bool ValidateWaveformBySchema(const WaveformConfig& wf, std::string& err)
{
    const SourceParameterSchemaEntry* schema = nullptr;
    size_t schemaCount = 0;
    if (!TryGetParameterSchema(SourceKind::Waveform, schema, schemaCount) || schema == nullptr)
    {
        err = "waveform schema is not defined";
        return false;
    }

    for (size_t i = 0; i < schemaCount; i++)
    {
        const SourceParameterSchemaEntry& e = schema[i];
        double value = 0.0;
        if (!WaveformSchemaValue(wf, e, value))
        {
            err = "waveform schema has unknown id: " + std::string(e.id);
            return false;
        }

        if (!ValidateSchemaRange("waveform", e, value, false, err))
        {
            return false;
        }
    }
    return true;
}

bool ValidateFmBySchema(const FmConfig& fm, std::string& err)
{
    const SourceParameterSchemaEntry* schema = nullptr;
    size_t schemaCount = 0;
    if (!TryGetParameterSchema(SourceKind::Fm, schema, schemaCount) || schema == nullptr)
    {
        err = "fm schema is not defined";
        return false;
    }

    for (size_t i = 0; i < schemaCount; i++)
    {
        const SourceParameterSchemaEntry& e = schema[i];
        double value = 0.0;
        if (!FmSchemaValue(fm, e, value))
        {
            err = "fm schema has unknown id: " + std::string(e.id);
            return false;
        }
        if (!ValidateSchemaRange("fm", e, value, false, err))
        {
            return false;
        }
    }

    // ratio は 0.0 を許容しない（無音化と不正設定の早期検出）。
    if (fm.carrierRatio <= 0.0)
    {
        err = "fm.carrierRatio must be in range 0.0<..32.0";
        return false;
    }
    if (fm.modRatio <= 0.0)
    {
        err = "fm.modRatio must be in range 0.0<..32.0";
        return false;
    }
    return true;
}

bool ValidateDrumBySchema(const DrumConfig& drum, std::string& err)
{
    const SourceParameterSchemaEntry* schema = nullptr;
    size_t schemaCount = 0;
    if (!TryGetParameterSchema(SourceKind::Drum, schema, schemaCount) || schema == nullptr)
    {
        err = "drum schema is not defined";
        return false;
    }

    for (size_t i = 0; i < schemaCount; i++)
    {
        const SourceParameterSchemaEntry& e = schema[i];
        double value = 0.0;
        if (!DrumSchemaValue(drum, e, value))
        {
            err = "drum schema has unknown id: " + std::string(e.id);
            return false;
        }
        if (!ValidateSchemaRange("drum", e, value, IsDrumOptionalWhenNonPositive(e.id), err))
        {
            return false;
        }
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
    if (auto v = ReadJSONDouble(text, "baseFreq")) drum.baseFreq = *v;
    if (auto v = ReadJSONDouble(text, "pitchDrop")) drum.pitchDrop = *v;
    if (auto v = ReadJSONDouble(text, "pitchDecaySec")) drum.pitchDecaySec = *v;
    if (auto v = ReadJSONDouble(text, "toneFreq")) drum.toneFreq = *v;
    if (auto v = ReadJSONDouble(text, "toneLevel")) drum.toneLevel = *v;
    if (auto v = ReadJSONDouble(text, "noiseLevel")) drum.noiseLevel = *v;
    if (auto v = ReadJSONDouble(text, "hpCut")) drum.hpCut = *v;
    if (auto v = ReadJSONDouble(text, "lpCut")) drum.lpCut = *v;
    if (auto v = ReadJSONString(text, "toneWave"))
    {
        WaveType w{};
        if (!TryParseWaveType(*v, w))
        {
            err = "invalid toneWave: " + *v;
            return false;
        }
        drum.toneWave = (int)w;
    }
    if (auto v = ReadJSONString(text, "noiseType"))
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

} // namespace

bool ParseSourceObject(const std::string& sourceObjText, SourceConfig& outSource, std::string& err)
{
    const auto type = ReadJSONString(sourceObjText, "type");
    if (!type)
    {
        err = "source.type is required";
        return false;
    }

    SourceKind sourceKind{};
    if (!TryParseSourceKind(*type, sourceKind))
    {
        err = "unknown source.type: " + *type;
        return false;
    }
    if (!ValidateLifecycleContract(sourceObjText, sourceKind, err))
    {
        return false;
    }

    switch (sourceKind)
    {
    case SourceKind::Waveform:
    {
        auto wave = ReadJSONString(sourceObjText, "wave");
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
        WaveformConfig wf{};
        wf.wave = w;
        if (auto v = ReadJSONInt(sourceObjText, "unisonVoices"))
        {
            wf.unisonVoices = *v;
        }
        if (auto v = ReadJSONDouble(sourceObjText, "unisonDetuneCents"))
        {
            wf.unisonDetuneCents = *v;
        }
        if (auto v = ReadJSONDouble(sourceObjText, "unisonSpread"))
        {
            wf.unisonSpread = *v;
        }
        if (auto v = ReadJSONDouble(sourceObjText, "subOscLevel"))
        {
            wf.subOscLevel = *v;
        }
        if (auto v = ReadJSONString(sourceObjText, "filterMode"))
        {
            FilterMode mode{};
            if (!TryParseFilterMode(*v, mode))
            {
                err = "invalid waveform.filterMode: " + *v;
                return false;
            }
            wf.filterMode = mode;
        }
        if (auto v = ReadJSONDouble(sourceObjText, "filterCutoffHz"))
        {
            wf.filterCutoffHz = *v;
        }
        if (auto v = ReadJSONDouble(sourceObjText, "filterResonance"))
        {
            wf.filterResonance = *v;
        }
        std::string smoothingObj;
        bool foundSmoothing = false;
        if (!ExtractObjectForKey(sourceObjText, "smoothing", smoothingObj, foundSmoothing, err))
        {
            return false;
        }
        if (foundSmoothing)
        {
            if (!ParseWaveformSmoothingObject(smoothingObj, wf.smoothing))
            {
                err = "invalid waveform.smoothing object";
                return false;
            }
        }
        std::string modulationObj;
        bool foundModulation = false;
        if (!ExtractObjectForKey(sourceObjText, "modulation", modulationObj, foundModulation, err))
        {
            return false;
        }
        if (foundModulation)
        {
            if (!ParseModulationObject(modulationObj, wf.modulation, err))
            {
                return false;
            }
        }
        if (!ValidateWaveformBySchema(wf, err))
        {
            return false;
        }
        if (!ValidateModulation(wf.modulation, err))
        {
            return false;
        }
        if (!ValidateWaveformSmoothing(wf.smoothing, err))
        {
            return false;
        }
        outSource = wf;
        return true;
    }
    case SourceKind::Noise:
    {
        auto noise = ReadJSONString(sourceObjText, "noise");
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
    case SourceKind::Fm:
    {
        auto carrier = ReadJSONString(sourceObjText, "carrierWave");
        auto mod = ReadJSONString(sourceObjText, "modWave");
        auto carrierRatio = ReadJSONDouble(sourceObjText, "carrierRatio");
        auto modRatio = ReadJSONDouble(sourceObjText, "modRatio");
        auto index = ReadJSONDouble(sourceObjText, "index");
        auto outLevel = ReadJSONDouble(sourceObjText, "outLevel");
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
        FmConfig fm{ cw, mw, *carrierRatio, *modRatio, *index, *outLevel };
        if (!ValidateFmBySchema(fm, err))
        {
            return false;
        }
        outSource = fm;
        return true;
    }
    case SourceKind::Drum:
    {
        DrumConfig drum{};
        if (!ParseDrumConfigObject(sourceObjText, drum, err))
        {
            return false;
        }
        if (!ValidateDrumBySchema(drum, err))
        {
            return false;
        }
        outSource = drum;
        return true;
    }
    case SourceKind::DrumKit:
    {
        // drumkit は差分上書き前提のため、未指定noteは None 初期値を維持する。
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
                if (!ValidateDrumBySchema(d, err))
                {
                    err = "drumkit note " + k + ": " + err;
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
    case SourceKind::Count:
        break;
    }

    err = "unknown source.type: " + *type;
    return false;
}
} // namespace config::internal::load
