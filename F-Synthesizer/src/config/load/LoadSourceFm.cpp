#include "Internal.h"

#include <algorithm>

#include "../ConfigFileInternal.h"

namespace config::internal::load
{
bool ParseFmSource(const std::string& sourceObjText, SourceConfig& outSource, std::string& err)
{
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
} // namespace config::internal::load
