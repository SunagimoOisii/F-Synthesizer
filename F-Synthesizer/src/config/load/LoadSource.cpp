#include "Internal.h"

#include <algorithm>
#include <iomanip>
#include <regex>
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

// 目的: source.lifecycle が SourceKind 固定値と一致するか確認する。
// 前提: lifecycle は任意項目。未指定時は検証をスキップする。
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
    // Drum系では「0以下を未指定扱い」にする項目がある。
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
    if (std::string_view(e.id) == "pulseWidth") { outValue = wf.pulseWidth; return true; }
    if (std::string_view(e.id) == "hardSyncEnabled") { outValue = wf.hardSyncEnabled ? 1.0 : 0.0; return true; }
    if (std::string_view(e.id) == "hardSyncRatio") { outValue = wf.hardSyncRatio; return true; }
    if (std::string_view(e.id) == "ringModEnabled") { outValue = wf.ringModEnabled ? 1.0 : 0.0; return true; }
    if (std::string_view(e.id) == "ringModRatio") { outValue = wf.ringModRatio; return true; }
    if (std::string_view(e.id) == "ringModMix") { outValue = wf.ringModMix; return true; }
    if (std::string_view(e.id) == "filterCutoffHz") { outValue = wf.filterCutoffHz; return true; }
    if (std::string_view(e.id) == "filterResonance") { outValue = wf.filterResonance; return true; }
    if (std::string_view(e.id) == "filterKeytrack") { outValue = wf.filterKeytrack; return true; }
    return false;
}

bool AnalogSchemaValue(const AnalogConfig& analog, const SourceParameterSchemaEntry& e, double& outValue)
{
    if (std::string_view(e.id) == "unisonVoices") { outValue = static_cast<double>(analog.unisonVoices); return true; }
    if (std::string_view(e.id) == "unisonDetuneCents") { outValue = analog.unisonDetuneCents; return true; }
    if (std::string_view(e.id) == "unisonSpread") { outValue = analog.unisonSpread; return true; }
    if (std::string_view(e.id) == "subOscLevel") { outValue = analog.subOscLevel; return true; }
    if (std::string_view(e.id) == "pulseWidth") { outValue = analog.pulseWidth; return true; }
    if (std::string_view(e.id) == "hardSyncEnabled") { outValue = analog.hardSyncEnabled ? 1.0 : 0.0; return true; }
    if (std::string_view(e.id) == "hardSyncRatio") { outValue = analog.hardSyncRatio; return true; }
    if (std::string_view(e.id) == "ringModEnabled") { outValue = analog.ringModEnabled ? 1.0 : 0.0; return true; }
    if (std::string_view(e.id) == "ringModRatio") { outValue = analog.ringModRatio; return true; }
    if (std::string_view(e.id) == "ringModMix") { outValue = analog.ringModMix; return true; }
    if (std::string_view(e.id) == "filterCutoffHz") { outValue = analog.filterCutoffHz; return true; }
    if (std::string_view(e.id) == "filterResonance") { outValue = analog.filterResonance; return true; }
    if (std::string_view(e.id) == "filterKeytrack") { outValue = analog.filterKeytrack; return true; }
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

bool NoiseSchemaValue(const NoiseConfig& noise, const SourceParameterSchemaEntry& e, double& outValue)
{
    if (std::string_view(e.id) == "noise") { outValue = static_cast<double>(noise.noise); return true; }
    if (std::string_view(e.id) == "filterCutoffHz") { outValue = noise.filterCutoffHz; return true; }
    if (std::string_view(e.id) == "filterResonance") { outValue = noise.filterResonance; return true; }
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

bool ValidateNoiseBySchema(const NoiseConfig& noise, std::string& err)
{
    const SourceParameterSchemaEntry* schema = nullptr;
    size_t schemaCount = 0;
    if (!TryGetParameterSchema(SourceKind::Noise, schema, schemaCount) || schema == nullptr || schemaCount == 0)
    {
        err = "noise schema is not defined";
        return false;
    }

    for (size_t i = 0; i < schemaCount; i++)
    {
        const SourceParameterSchemaEntry& e = schema[i];
        double value = 0.0;
        if (!NoiseSchemaValue(noise, e, value))
        {
            err = "noise schema has unknown id: " + std::string(e.id);
            return false;
        }
        if (!ValidateSchemaRange("noise", e, value, false, err))
        {
            return false;
        }
    }
    return true;
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

bool ValidateAnalogBySchema(const AnalogConfig& analog, std::string& err)
{
    const SourceParameterSchemaEntry* schema = nullptr;
    size_t schemaCount = 0;
    if (!TryGetParameterSchema(SourceKind::Analog, schema, schemaCount) || schema == nullptr)
    {
        err = "analog schema is not defined";
        return false;
    }

    for (size_t i = 0; i < schemaCount; i++)
    {
        const SourceParameterSchemaEntry& e = schema[i];
        double value = 0.0;
        if (!AnalogSchemaValue(analog, e, value))
        {
            err = "analog schema has unknown id: " + std::string(e.id);
            return false;
        }

        if (!ValidateSchemaRange("analog", e, value, false, err))
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
    return true;
}

bool ExtractArrayAt(const std::string& text, size_t openBracketPos, std::string& outArray, std::string& err)
{
    // 目的: openBracketPos から対応する ']' までを切り出す。
    if (openBracketPos >= text.size() || text[openBracketPos] != '[')
    {
        err = "invalid array start";
        return false;
    }
    int depth = 0;
    bool inString = false;
    bool escaped = false;
    for (size_t i = openBracketPos; i < text.size(); i++)
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
        if (c == '[')
        {
            depth++;
        }
        else if (c == ']')
        {
            depth--;
            if (depth == 0)
            {
                outArray = text.substr(openBracketPos, i - openBracketPos + 1);
                return true;
            }
        }
    }
    err = "unterminated array";
    return false;
}

bool ExtractArrayForKey(const std::string& text, const std::string& key, std::string& outArray, bool& found, std::string& err)
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
    if (pos >= text.size() || text[pos] != '[')
    {
        err = "key '" + key + "' must be an array";
        return false;
    }
    return ExtractArrayAt(text, pos, outArray, err);
}

bool ParseTopLevelArrayObjectEntries(
    const std::string& arrText,
    const std::function<bool(size_t, const std::string&)>& onEntry,
    std::string& err)
{
    // 目的: [{...}, {...}] 形式の配列を1段だけ走査する。
    if (arrText.size() < 2 || arrText.front() != '[' || arrText.back() != ']')
    {
        err = "invalid array";
        return false;
    }

    size_t i = 1;
    size_t index = 0;
    while (i + 1 < arrText.size())
    {
        while (i < arrText.size() && (arrText[i] == ' ' || arrText[i] == '\t' || arrText[i] == '\r' || arrText[i] == '\n' || arrText[i] == ','))
        {
            i++;
        }
        if (i >= arrText.size() || arrText[i] == ']')
        {
            break;
        }
        if (arrText[i] != '{')
        {
            err = "array item must be object";
            return false;
        }
        std::string valueObj;
        if (!ExtractObjectAt(arrText, i, valueObj, err))
        {
            return false;
        }
        if (!onEntry(index, valueObj))
        {
            return false;
        }
        i += valueObj.size();
        index++;
    }

    return true;
}

bool ParseTopLevelIntArrayElements(
    const std::string& arrText,
    const std::function<bool(size_t, int)>& onElement,
    std::string& err)
{
    if (arrText.size() < 2 || arrText.front() != '[' || arrText.back() != ']')
    {
        err = "invalid integer array";
        return false;
    }

    const std::regex intPat("-?\\d+");
    size_t index = 0;
    for (std::sregex_iterator it(arrText.begin(), arrText.end(), intPat), end; it != end; ++it, ++index)
    {
        const int value = std::stoi((*it)[0].str());
        if (!onElement(index, value))
        {
            return false;
        }
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

bool ValidateSmoothingSupport(
    const std::string& sourceObjText,
    SourceKind sourceKind,
    std::string& err)
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

} // namespace

bool ParseSourceObject(const std::string& sourceObjText, SourceConfig& outSource, std::string& err)
{
    // 呼び出し経路: LoadConfigFromText -> ParseSourceObject。
    // 目的: source.type ごとの必須キー検証と SourceConfig 構築をここで完結させる。
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
    if (!ValidateSmoothingSupport(sourceObjText, sourceKind, err))
    {
        return false;
    }

    switch (sourceKind)
    {
    case SourceKind::Waveform:
    {
        // Waveform は modulation/smoothing を含むため、サブオブジェクトを段階解析する。
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
        if (auto v = ReadJSONDouble(sourceObjText, "pulseWidth"))
        {
            wf.pulseWidth = std::clamp(*v, 0.05, 0.95);
        }
        if (auto v = ReadJSONBool(sourceObjText, "hardSyncEnabled"))
        {
            wf.hardSyncEnabled = *v;
        }
        if (auto v = ReadJSONDouble(sourceObjText, "hardSyncRatio"))
        {
            wf.hardSyncRatio = std::clamp(*v, 0.5, 8.0);
        }
        if (auto v = ReadJSONBool(sourceObjText, "ringModEnabled"))
        {
            wf.ringModEnabled = *v;
        }
        if (auto v = ReadJSONDouble(sourceObjText, "ringModRatio"))
        {
            wf.ringModRatio = std::clamp(*v, 0.125, 16.0);
        }
        if (auto v = ReadJSONDouble(sourceObjText, "ringModMix"))
        {
            wf.ringModMix = std::clamp(*v, 0.0, 1.0);
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
        if (auto v = ReadJSONDouble(sourceObjText, "filterKeytrack"))
        {
            wf.filterKeytrack = *v;
        }
        if (auto v = ReadJSONDouble(sourceObjText, "drive"))
        {
            wf.drive = std::clamp(*v, 0.0, 1.0);
        }
        std::string arpeggioObj;
        bool foundArpeggio = false;
        if (!ExtractObjectForKey(sourceObjText, "arpeggio", arpeggioObj, foundArpeggio, err))
        {
            return false;
        }
        if (foundArpeggio)
        {
            if (auto v = ReadJSONBool(arpeggioObj, "enabled"))
            {
                wf.arpeggio.enabled = *v;
            }
            if (auto v = ReadJSONDouble(arpeggioObj, "rateHz"))
            {
                wf.arpeggio.rateHz = std::clamp(*v, 0.5, 100.0);
            }
            if (auto v = ReadJSONInt(arpeggioObj, "steps"))
            {
                wf.arpeggio.steps = std::clamp(*v, 1, 8);
            }

            std::string semitonesArray;
            bool foundSemitones = false;
            if (!ExtractArrayForKey(arpeggioObj, "semitones", semitonesArray, foundSemitones, err))
            {
                return false;
            }
            if (foundSemitones)
            {
                if (!ParseTopLevelIntArrayElements(semitonesArray, [&](size_t semitoneIndex, int value) {
                    if (semitoneIndex >= wf.arpeggio.semitones.size())
                    {
                        return true;
                    }
                    wf.arpeggio.semitones[semitoneIndex] = std::clamp(value, -24, 24);
                    return true;
                    }, err))
                {
                    return false;
                }
            }
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
        if (!ValidateModulation(wf.modulation, false, "waveform.modulation", err))
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
    case SourceKind::Analog:
    {
        auto wave = ReadJSONString(sourceObjText, "wave");
        if (!wave)
        {
            err = "analog source requires 'wave'";
            return false;
        }
        WaveType w{};
        if (!TryParseWaveType(*wave, w))
        {
            err = "invalid wave: " + *wave;
            return false;
        }
        AnalogConfig analog{};
        analog.wave = w;
        if (auto v = ReadJSONInt(sourceObjText, "unisonVoices"))
        {
            analog.unisonVoices = *v;
        }
        if (auto v = ReadJSONDouble(sourceObjText, "unisonDetuneCents"))
        {
            analog.unisonDetuneCents = *v;
        }
        if (auto v = ReadJSONDouble(sourceObjText, "unisonSpread"))
        {
            analog.unisonSpread = *v;
        }
        if (auto v = ReadJSONDouble(sourceObjText, "subOscLevel"))
        {
            analog.subOscLevel = *v;
        }
        if (auto v = ReadJSONDouble(sourceObjText, "pulseWidth"))
        {
            analog.pulseWidth = std::clamp(*v, 0.05, 0.95);
        }
        if (auto v = ReadJSONBool(sourceObjText, "hardSyncEnabled"))
        {
            analog.hardSyncEnabled = *v;
        }
        if (auto v = ReadJSONDouble(sourceObjText, "hardSyncRatio"))
        {
            analog.hardSyncRatio = std::clamp(*v, 0.5, 8.0);
        }
        if (auto v = ReadJSONBool(sourceObjText, "ringModEnabled"))
        {
            analog.ringModEnabled = *v;
        }
        if (auto v = ReadJSONDouble(sourceObjText, "ringModRatio"))
        {
            analog.ringModRatio = std::clamp(*v, 0.125, 16.0);
        }
        if (auto v = ReadJSONDouble(sourceObjText, "ringModMix"))
        {
            analog.ringModMix = std::clamp(*v, 0.0, 1.0);
        }
        if (auto v = ReadJSONString(sourceObjText, "filterMode"))
        {
            FilterMode mode{};
            if (!TryParseFilterMode(*v, mode))
            {
                err = "invalid analog.filterMode: " + *v;
                return false;
            }
            analog.filterMode = mode;
        }
        if (auto v = ReadJSONDouble(sourceObjText, "filterCutoffHz"))
        {
            analog.filterCutoffHz = *v;
        }
        if (auto v = ReadJSONDouble(sourceObjText, "filterResonance"))
        {
            analog.filterResonance = *v;
        }
        if (auto v = ReadJSONDouble(sourceObjText, "filterKeytrack"))
        {
            analog.filterKeytrack = *v;
        }
        if (auto v = ReadJSONDouble(sourceObjText, "drive"))
        {
            analog.drive = std::clamp(*v, 0.0, 1.0);
        }
        if (auto v = ReadJSONDouble(sourceObjText, "driftDepthCents"))
        {
            analog.driftDepthCents = *v;
        }
        if (auto v = ReadJSONDouble(sourceObjText, "driftRateHz"))
        {
            analog.driftRateHz = *v;
        }
        std::string arpeggioObj;
        bool foundArpeggio = false;
        if (!ExtractObjectForKey(sourceObjText, "arpeggio", arpeggioObj, foundArpeggio, err))
        {
            return false;
        }
        if (foundArpeggio)
        {
            if (auto v = ReadJSONBool(arpeggioObj, "enabled"))
            {
                analog.arpeggio.enabled = *v;
            }
            if (auto v = ReadJSONDouble(arpeggioObj, "rateHz"))
            {
                analog.arpeggio.rateHz = std::clamp(*v, 0.5, 100.0);
            }
            if (auto v = ReadJSONInt(arpeggioObj, "steps"))
            {
                analog.arpeggio.steps = std::clamp(*v, 1, 8);
            }

            std::string semitonesArray;
            bool foundSemitones = false;
            if (!ExtractArrayForKey(arpeggioObj, "semitones", semitonesArray, foundSemitones, err))
            {
                return false;
            }
            if (foundSemitones)
            {
                if (!ParseTopLevelIntArrayElements(semitonesArray, [&](size_t semitoneIndex, int value) {
                    if (semitoneIndex >= analog.arpeggio.semitones.size())
                    {
                        return true;
                    }
                    analog.arpeggio.semitones[semitoneIndex] = std::clamp(value, -24, 24);
                    return true;
                    }, err))
                {
                    return false;
                }
            }
        }
        std::string smoothingObj;
        bool foundSmoothing = false;
        if (!ExtractObjectForKey(sourceObjText, "smoothing", smoothingObj, foundSmoothing, err))
        {
            return false;
        }
        if (foundSmoothing)
        {
            if (!ParseWaveformSmoothingObject(smoothingObj, analog.smoothing))
            {
                err = "invalid analog.smoothing object";
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
            if (!ParseModulationObject(modulationObj, analog.modulation, err))
            {
                return false;
            }
        }
        if (!ValidateAnalogBySchema(analog, err))
        {
            return false;
        }
        if (!ValidateModulation(analog.modulation, false, "analog.modulation", err))
        {
            return false;
        }
        if (!ValidateWaveformSmoothing(analog.smoothing, err))
        {
            return false;
        }
        outSource = analog;
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
        NoiseConfig nz{};
        nz.noise = n;
        if (auto v = ReadJSONString(sourceObjText, "filterMode"))
        {
            FilterMode mode{};
            if (!TryParseFilterMode(*v, mode))
            {
                err = "invalid noise.filterMode: " + *v;
                return false;
            }
            nz.filterMode = mode;
        }
        if (auto v = ReadJSONDouble(sourceObjText, "filterCutoffHz"))
        {
            nz.filterCutoffHz = *v;
        }
        if (auto v = ReadJSONDouble(sourceObjText, "filterResonance"))
        {
            nz.filterResonance = *v;
        }
        if (!ValidateNoiseBySchema(nz, err))
        {
            return false;
        }
        outSource = nz;
        return true;
    }
    case SourceKind::Fm:
    {
        // FM は新4オペ形式(ops)を優先し、旧2オペ形式を自動変換する。
        FmConfig fm{};
        fm.algorithm = 0;
        fm.feedback = 0.0;
        for (int i = 0; i < 4; i++)
        {
            fm.ops[i].wave = WaveType::Sine;
            fm.ops[i].ratio = 1.0;
            fm.ops[i].level = 0.0;
            fm.ops[i].index = 0.0;
        }

        std::string opsArray;
        bool foundOps = false;
        if (!ExtractArrayForKey(sourceObjText, "ops", opsArray, foundOps, err))
        {
            return false;
        }
        if (foundOps)
        {
            if (auto v = ReadJSONInt(sourceObjText, "algorithm"))
            {
                fm.algorithm = *v;
            }
            if (fm.algorithm < 0 || fm.algorithm > 7)
            {
                err = "fm.algorithm must be 0..7";
                return false;
            }
            if (auto v = ReadJSONDouble(sourceObjText, "feedback"))
            {
                fm.feedback = std::clamp(*v, 0.0, 1.0);
            }

            size_t opCount = 0;
            if (!ParseTopLevelArrayObjectEntries(opsArray, [&](size_t opIndex, const std::string& opObj) {
                if (opIndex >= 4)
                {
                    err = "fm.ops must contain 1..4 elements";
                    return false;
                }

                const auto wave = ReadJSONString(opObj, "wave");
                if (!wave)
                {
                    err = "fm.ops[" + std::to_string(opIndex) + "].wave is required";
                    return false;
                }
                WaveType parsedWave{};
                if (!TryParseWaveType(*wave, parsedWave))
                {
                    err = "invalid fm.ops[" + std::to_string(opIndex) + "].wave: " + *wave;
                    return false;
                }
                fm.ops[opIndex].wave = parsedWave;

                const auto ratio = ReadJSONDouble(opObj, "ratio");
                if (!ratio)
                {
                    err = "fm.ops[" + std::to_string(opIndex) + "].ratio is required";
                    return false;
                }
                if (*ratio <= 0.0)
                {
                    err = "fm.ops[" + std::to_string(opIndex) + "].ratio must be > 0.0";
                    return false;
                }
                fm.ops[opIndex].ratio = std::clamp(*ratio, 0.0, 32.0);

                if (const auto level = ReadJSONDouble(opObj, "level"))
                {
                    fm.ops[opIndex].level = std::clamp(*level, 0.0, 1.0);
                }
                if (const auto index = ReadJSONDouble(opObj, "index"))
                {
                    fm.ops[opIndex].index = std::clamp(*index, 0.0, 32.0);
                }

                opCount = opIndex + 1;
                return true;
                }, err))
            {
                return false;
            }
            if (opCount < 1 || opCount > 4)
            {
                err = "fm.ops must contain 1..4 elements";
                return false;
            }
        }
        else
        {
            const auto carrier = ReadJSONString(sourceObjText, "carrierWave");
            if (!carrier)
            {
                err = "fm source requires 'ops' or carrierWave/modWave/carrierRatio/modRatio/index/outLevel";
                return false;
            }
            const auto mod = ReadJSONString(sourceObjText, "modWave");
            const auto carrierRatio = ReadJSONDouble(sourceObjText, "carrierRatio");
            const auto modRatio = ReadJSONDouble(sourceObjText, "modRatio");
            const auto index = ReadJSONDouble(sourceObjText, "index");
            const auto outLevel = ReadJSONDouble(sourceObjText, "outLevel");
            if (!mod || !carrierRatio || !modRatio || !index || !outLevel)
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
            if (*carrierRatio <= 0.0)
            {
                err = "fm.carrierRatio must be in range 0.0<..32.0";
                return false;
            }
            if (*modRatio <= 0.0)
            {
                err = "fm.modRatio must be in range 0.0<..32.0";
                return false;
            }

            fm.algorithm = 0;
            fm.feedback = 0.0;
            fm.ops[0].wave = mw;
            fm.ops[0].ratio = *modRatio;
            fm.ops[0].level = 1.0;
            fm.ops[0].index = *index;
            fm.ops[1].wave = cw;
            fm.ops[1].ratio = *carrierRatio;
            fm.ops[1].level = *outLevel;
            fm.ops[1].index = 0.0;
        }

        if (auto v = ReadJSONString(sourceObjText, "filterMode"))
        {
            FilterMode mode{};
            if (!TryParseFilterMode(*v, mode))
            {
                err = "invalid fm.filterMode: " + *v;
                return false;
            }
            fm.filterMode = mode;
        }
        if (auto v = ReadJSONDouble(sourceObjText, "filterCutoffHz"))
        {
            fm.filterCutoffHz = *v;
        }
        if (auto v = ReadJSONDouble(sourceObjText, "filterResonance"))
        {
            fm.filterResonance = *v;
        }
        if (auto v = ReadJSONDouble(sourceObjText, "drive"))
        {
            fm.drive = std::clamp(*v, 0.0, 1.0);
        }
        std::string modulationObj;
        bool foundModulation = false;
        if (!ExtractObjectForKey(sourceObjText, "modulation", modulationObj, foundModulation, err))
        {
            return false;
        }
        if (foundModulation)
        {
            if (!ParseModulationObject(modulationObj, fm.modulation, err))
            {
                return false;
            }
        }
        if (!ValidateFmBySchema(fm, err))
        {
            return false;
        }
        if (!ValidateModulation(fm.modulation, true, "fm.modulation", err))
        {
            return false;
        }
        outSource = fm;
        return true;
    }
    case SourceKind::Drum:
    {
        // 旧フォーマット("type": "drum")を drumkit(note 60)へ自動変換してロードする。
        DrumConfig drum{};
        if (!ParseDrumConfigObject(sourceObjText, drum, err))
        {
            return false;
        }
        if (!ValidateDrumBySchema(drum, err))
        {
            return false;
        }
        DrumKitConfig kit{};
        for (auto& d : kit.map) d.type = DrumType::None;
        kit.map[60] = drum;
        outSource = kit;
        return true;
    }
    case SourceKind::DrumKit:
    {
        // DrumKit は差分上書き方式。未指定ノートは DrumType::None のまま保持する。
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
    case SourceKind::Psg:
    {
        PsgConfig psg{};

        const auto waveStr = ReadJSONString(sourceObjText, "wave");
        if (!waveStr)
        {
            err = "psg source requires 'wave'";
            return false;
        }
        if (*waveStr == "square")        psg.wave = PsgWaveType::Square;
        else if (*waveStr == "pulse")    psg.wave = PsgWaveType::Pulse;
        else if (*waveStr == "triangle") psg.wave = PsgWaveType::Triangle;
        else if (*waveStr == "noise")    psg.wave = PsgWaveType::Noise;
        else
        {
            err = "invalid psg wave: " + *waveStr + " (square/pulse/triangle/noise)";
            return false;
        }

        if (auto v = ReadJSONInt(sourceObjText, "duty"))
        {
            if (*v < 0 || *v > 7) { err = "psg.duty must be 0..7"; return false; }
            psg.duty = *v;
        }
        if (auto v = ReadJSONInt(sourceObjText, "volumeSteps"))
        {
            if (*v < 0 || *v > 15) { err = "psg.volumeSteps must be 0..15"; return false; }
            psg.volumeSteps = *v;
        }
        if (auto v = ReadJSONInt(sourceObjText, "maxVoices"))
        {
            if (*v < 1 || *v > 8) { err = "psg.maxVoices must be 1..8"; return false; }
            psg.maxVoices = *v;
        }

        outSource = psg;
        return true;
    }
    case SourceKind::Count:
        break;
    }

    err = "unknown source.type: " + *type;
    return false;
}
} // namespace config::internal::load
