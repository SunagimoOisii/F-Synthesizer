#include "Internal.h"

#include <algorithm>

#include "../ConfigFileInternal.h"

namespace config::internal::load
{
namespace
{
bool ParseFmOperatorEnvObject(const std::string& text, ModEnvelopeConfig& env)
{
    if (auto v = ReadJSONDouble(text, "attackSec")) env.attackSec = std::clamp(*v, 0.0, 10.0);
    if (auto v = ReadJSONDouble(text, "decaySec")) env.decaySec = std::clamp(*v, 0.0, 10.0);
    if (auto v = ReadJSONDouble(text, "sustainLevel")) env.sustainLevel = std::clamp(*v, 0.0, 1.0);
    if (auto v = ReadJSONDouble(text, "releaseSec")) env.releaseSec = std::clamp(*v, 0.0, 10.0);
    if (auto v = ReadJSONDouble(text, "curve")) env.curve = std::clamp(*v, 0.0, 1.0);
    return true;
}
} // namespace

bool ParseFmSource(const std::string& sourceObjText, SourceConfig& outSource, std::string& err)
{
    FmConfig fm{};
    if (auto v = ReadJSONInt(sourceObjText, "chip")) fm.chip = std::clamp(*v, 0, 1);
    if (auto v = ReadJSONDouble(sourceObjText, "brightness")) fm.brightness = std::clamp(*v, 0.0, 1.0);
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
            std::string levelEnvObj;
            bool foundLevelEnv = false;
            if (!ExtractObjectForKey(opObj, "levelEnv", levelEnvObj, foundLevelEnv, err))
            {
                return false;
            }
            if (foundLevelEnv)
            {
                ParseFmOperatorEnvObject(levelEnvObj, fm.ops[opIndex].levelEnv);
            }
            std::string indexEnvObj;
            bool foundIndexEnv = false;
            if (!ExtractObjectForKey(opObj, "indexEnv", indexEnvObj, foundIndexEnv, err))
            {
                return false;
            }
            if (foundIndexEnv)
            {
                ParseFmOperatorEnvObject(indexEnvObj, fm.ops[opIndex].indexEnv);
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
        err = "fm source requires 'ops' array";
        return false;
    }

    std::string filterObj;
    bool foundFilter = false;
    if (!ExtractObjectForKey(sourceObjText, "filter", filterObj, foundFilter, err))
    {
        return false;
    }
    const std::string& filterText = foundFilter ? filterObj : sourceObjText;
    if (auto v = ReadJSONString(filterText, "mode"))
    {
        FilterMode mode{};
        if (!TryParseFilterMode(*v, mode))
        {
            err = "invalid fm.filterMode: " + *v;
            return false;
        }
        fm.filterMode = mode;
    }
    if (auto v = ReadJSONDouble(filterText, "cutoffHz"))
    {
        fm.filterCutoffHz = *v;
    }
    if (auto v = ReadJSONDouble(filterText, "resonance"))
    {
        fm.filterResonance = *v;
    }
    if (auto v = ReadJSONDouble(filterText, "drive"))
    {
        fm.filterDrive = std::clamp(*v, 0.0, 1.0);
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
    // Old projects used different numbering for their four software algorithms.
    if (!ReadJSONInt(sourceObjText, "chip"))
    {
        constexpr int legacyAlgorithms[] = { 4, 4, 5, 0, 4, 5, 6, 7 };
        fm.algorithm = legacyAlgorithms[fm.algorithm];
    }
    outSource = fm;
    return true;
}
} // namespace config::internal::load
