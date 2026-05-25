#include "Internal.h"

#include <algorithm>

#include "../ConfigFileInternal.h"

#include "third_party/nlohmann/json.hpp"
#include "io/PlatformPaths.h"

namespace config::internal::load
{
namespace
{
using Json = nlohmann::json;

constexpr const char* kProjectModelFormat = "projectModel.v2";

bool ReadOptionalBool(const Json& obj, const char* key, const std::string& path, bool& out, std::string& err)
{
    const auto it = obj.find(key);
    if (it == obj.end())
    {
        return true;
    }
    if (!it->is_boolean())
    {
        err = path + "." + key + " must be boolean";
        return false;
    }
    out = it->get<bool>();
    return true;
}

bool ReadOptionalInt(const Json& obj, const char* key, const std::string& path, int& out, std::string& err)
{
    const auto it = obj.find(key);
    if (it == obj.end())
    {
        return true;
    }
    if (!it->is_number_integer())
    {
        err = path + "." + key + " must be integer";
        return false;
    }
    out = it->get<int>();
    return true;
}

bool ReadOptionalDouble(const Json& obj, const char* key, const std::string& path, double& out, std::string& err)
{
    const auto it = obj.find(key);
    if (it == obj.end())
    {
        return true;
    }
    if (!it->is_number())
    {
        err = path + "." + key + " must be number";
        return false;
    }
    out = it->get<double>();
    return true;
}

bool ReadOptionalString(const Json& obj, const char* key, const std::string& path, std::string& out, std::string& err)
{
    const auto it = obj.find(key);
    if (it == obj.end())
    {
        return true;
    }
    if (!it->is_string())
    {
        err = path + "." + key + " must be string";
        return false;
    }
    out = it->get<std::string>();
    return true;
}

bool ValidateProjectModelFormat(const Json& root, std::string& err)
{
    const auto it = root.find("format");
    if (it == root.end())
    {
        err = "format is required";
        return false;
    }
    if (!it->is_string())
    {
        err = "format must be string";
        return false;
    }
    const std::string format = it->get<std::string>();
    if (format != kProjectModelFormat)
    {
        err = "unsupported format '" + format + "'; expected projectModel.v2";
        return false;
    }
    return true;
}

bool ParseEffectsObject(const Json& configRoot, AppConfig& cfg, std::string& err)
{
    const Json* effects = nullptr;
    auto effectsIt = configRoot.find("effects");
    if (effectsIt != configRoot.end())
    {
        if (!effectsIt->is_object())
        {
            err = "effects must be object";
            return false;
        }
        effects = &(*effectsIt);
    }
    else
    {
        auto masterEffectsIt = configRoot.find("masterEffects");
        if (masterEffectsIt != configRoot.end())
        {
            if (!masterEffectsIt->is_object())
            {
                err = "masterEffects must be object";
                return false;
            }
            effects = &(*masterEffectsIt);
        }
    }
    if (effects == nullptr)
    {
        return true;
    }

    auto getEffectObject = [&](const char* key, const Json*& out) -> bool
    {
        out = nullptr;
        const auto it = effects->find(key);
        if (it == effects->end())
        {
            return true;
        }
        if (!it->is_object())
        {
            err = std::string("effects.") + key + " must be object";
            return false;
        }
        out = &(*it);
        return true;
    };

    const Json* obj = nullptr;
    if (!getEffectObject("reverb", obj))
    {
        return false;
    }
    if (obj != nullptr)
    {
        double v = 0.0;
        if (!ReadOptionalBool(*obj, "enabled", "effects.reverb", cfg.masterEffects.reverb.enabled, err)) return false;
        if (!ReadOptionalDouble(*obj, "mix", "effects.reverb", v, err)) return false;
        if (obj->contains("mix")) cfg.masterEffects.reverb.mix = std::clamp(v, 0.0, 1.0);
        if (!ReadOptionalDouble(*obj, "roomSize", "effects.reverb", v, err)) return false;
        if (obj->contains("roomSize")) cfg.masterEffects.reverb.roomSize = std::clamp(v, 0.1, 1.0);
        if (!ReadOptionalDouble(*obj, "damping", "effects.reverb", v, err)) return false;
        if (obj->contains("damping")) cfg.masterEffects.reverb.damping = std::clamp(v, 0.0, 1.0);
    }

    if (!getEffectObject("delay", obj))
    {
        return false;
    }
    if (obj != nullptr)
    {
        double v = 0.0;
        if (!ReadOptionalBool(*obj, "enabled", "effects.delay", cfg.masterEffects.delay.enabled, err)) return false;
        if (!ReadOptionalDouble(*obj, "mix", "effects.delay", v, err)) return false;
        if (obj->contains("mix")) cfg.masterEffects.delay.mix = std::clamp(v, 0.0, 1.0);
        if (!ReadOptionalDouble(*obj, "timeSec", "effects.delay", v, err)) return false;
        if (obj->contains("timeSec")) cfg.masterEffects.delay.timeSec = std::clamp(v, 0.01, 2.0);
        if (!ReadOptionalDouble(*obj, "feedback", "effects.delay", v, err)) return false;
        if (obj->contains("feedback")) cfg.masterEffects.delay.feedback = std::clamp(v, 0.0, 0.95);
        if (!ReadOptionalBool(*obj, "tempoSync", "effects.delay", cfg.masterEffects.delay.tempoSync, err)) return false;
        if (!ReadOptionalDouble(*obj, "syncBeats", "effects.delay", v, err)) return false;
        if (obj->contains("syncBeats")) cfg.masterEffects.delay.syncBeats = std::clamp(v, 0.125, 4.0);
    }

    if (!getEffectObject("chorus", obj))
    {
        return false;
    }
    if (obj != nullptr)
    {
        double v = 0.0;
        if (!ReadOptionalBool(*obj, "enabled", "effects.chorus", cfg.masterEffects.chorus.enabled, err)) return false;
        if (!ReadOptionalDouble(*obj, "mix", "effects.chorus", v, err)) return false;
        if (obj->contains("mix")) cfg.masterEffects.chorus.mix = std::clamp(v, 0.0, 1.0);
        if (!ReadOptionalDouble(*obj, "baseDelayMs", "effects.chorus", v, err)) return false;
        if (obj->contains("baseDelayMs")) cfg.masterEffects.chorus.baseDelayMs = std::clamp(v, 2.0, 40.0);
        if (!ReadOptionalDouble(*obj, "depthMs", "effects.chorus", v, err)) return false;
        if (obj->contains("depthMs")) cfg.masterEffects.chorus.depthMs = std::clamp(v, 0.0, 20.0);
        if (!ReadOptionalDouble(*obj, "rateHz", "effects.chorus", v, err)) return false;
        if (obj->contains("rateHz")) cfg.masterEffects.chorus.rateHz = std::clamp(v, 0.05, 8.0);
        if (!ReadOptionalDouble(*obj, "feedback", "effects.chorus", v, err)) return false;
        if (obj->contains("feedback")) cfg.masterEffects.chorus.feedback = std::clamp(v, 0.0, 0.9);
    }

    if (!getEffectObject("flanger", obj))
    {
        return false;
    }
    if (obj != nullptr)
    {
        double v = 0.0;
        if (!ReadOptionalBool(*obj, "enabled", "effects.flanger", cfg.masterEffects.flanger.enabled, err)) return false;
        if (!ReadOptionalDouble(*obj, "mix", "effects.flanger", v, err)) return false;
        if (obj->contains("mix")) cfg.masterEffects.flanger.mix = std::clamp(v, 0.0, 1.0);
        if (!ReadOptionalDouble(*obj, "baseDelayMs", "effects.flanger", v, err)) return false;
        if (obj->contains("baseDelayMs")) cfg.masterEffects.flanger.baseDelayMs = std::clamp(v, 0.1, 8.0);
        if (!ReadOptionalDouble(*obj, "depthMs", "effects.flanger", v, err)) return false;
        if (obj->contains("depthMs")) cfg.masterEffects.flanger.depthMs = std::clamp(v, 0.0, 5.0);
        if (!ReadOptionalDouble(*obj, "rateHz", "effects.flanger", v, err)) return false;
        if (obj->contains("rateHz")) cfg.masterEffects.flanger.rateHz = std::clamp(v, 0.05, 8.0);
        if (!ReadOptionalDouble(*obj, "feedback", "effects.flanger", v, err)) return false;
        if (obj->contains("feedback")) cfg.masterEffects.flanger.feedback = std::clamp(v, 0.0, 0.95);
    }

    if (!getEffectObject("bitCrusher", obj))
    {
        return false;
    }
    if (obj != nullptr)
    {
        int bits = cfg.masterEffects.bitCrusher.bits;
        if (!ReadOptionalInt(*obj, "bits", "effects.bitCrusher", bits, err)) return false;
        cfg.masterEffects.bitCrusher.bits = std::clamp(bits, 1, 16);
    }

    if (!getEffectObject("sampleRateReducer", obj))
    {
        return false;
    }
    if (obj != nullptr)
    {
        double ratio = cfg.masterEffects.sampleRateReducer.ratio;
        if (!ReadOptionalDouble(*obj, "ratio", "effects.sampleRateReducer", ratio, err)) return false;
        cfg.masterEffects.sampleRateReducer.ratio = std::clamp(ratio, 0.0, 1.0);
    }

    return true;
}
} // namespace

bool LoadConfigFromText(
    const std::string& text,
    const std::filesystem::path& baseDir,
    AppConfig& cfg,
    std::string& err)
{
    Json root = Json::parse(text, nullptr, false);
    if (root.is_discarded() || !root.is_object())
    {
        err = "config root must be object";
        return false;
    }
    if (!ValidateProjectModelFormat(root, err))
    {
        return false;
    }
    const auto projectIt = root.find("project");
    if (projectIt == root.end())
    {
        err = "project is required";
        return false;
    }
    if (!projectIt->is_object())
    {
        err = "project must be object";
        return false;
    }
    const Json& configRootJson = *projectIt;
    const std::string configRoot = configRootJson.dump();

    std::string pathValue;
    if (!ReadOptionalString(configRootJson, "midiPath", "project", pathValue, err)) return false;
    if (configRootJson.contains("midiPath"))
    {
        cfg.midiPath = ResolvePathFromBase(baseDir, pathValue);
    }
    if (!ReadOptionalString(configRootJson, "wavPath", "project", pathValue, err)) return false;
    if (configRootJson.contains("wavPath"))
    {
        cfg.wavPath = ResolvePathFromBase(baseDir, pathValue);
    }
    if (!ReadOptionalInt(configRootJson, "targetChannel", "project", cfg.targetChannel, err)) return false;
    if (!ReadOptionalInt(configRootJson, "initialSeconds", "project", cfg.initialSeconds, err)) return false;
    if (!ReadOptionalInt(configRootJson, "bits", "project", cfg.bits, err)) return false;
    if (!ReadOptionalInt(configRootJson, "sampleRate", "project", cfg.sampleRate, err)) return false;
    if (!ReadOptionalDouble(configRootJson, "extraReleaseSec", "project", cfg.extraReleaseSec, err)) return false;
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
        // Writer実装の現行制約に合わせて早期に失敗させる。
        err = "bits must be 16";
        return false;
    }
    if (!LoadChannelsDiff(configRoot, cfg, err))
    {
        return false;
    }
    if (!LoadChannelMixDiff(configRoot, cfg, err))
    {
        return false;
    }
    if (!ParseEffectsObject(configRootJson, cfg, err))
    {
        return false;
    }

    return true;
}
} // namespace config::internal::load
