#include "Internal.h"

#include <algorithm>

#include "../ConfigFileInternal.h"

#include "io/PlatformPaths.h"

namespace config::internal::load
{
namespace
{
bool ParseEffectsObject(const std::string& text, AppConfig& cfg, std::string& err)
{
    std::string effectsObj;
    bool foundEffects = false;
    if (!ExtractObjectForKey(text, "effects", effectsObj, foundEffects, err))
    {
        return false;
    }
    if (!foundEffects)
    {
        return true;
    }

    std::string obj;
    bool found = false;
    if (!ExtractObjectForKey(effectsObj, "reverb", obj, found, err))
    {
        return false;
    }
    if (found)
    {
        if (auto v = ReadJSONBool(obj, "enabled")) cfg.masterEffects.reverb.enabled = *v;
        if (auto v = ReadJSONDouble(obj, "mix")) cfg.masterEffects.reverb.mix = std::clamp(*v, 0.0, 1.0);
        if (auto v = ReadJSONDouble(obj, "roomSize")) cfg.masterEffects.reverb.roomSize = std::clamp(*v, 0.1, 1.0);
        if (auto v = ReadJSONDouble(obj, "damping")) cfg.masterEffects.reverb.damping = std::clamp(*v, 0.0, 1.0);
    }

    if (!ExtractObjectForKey(effectsObj, "delay", obj, found, err))
    {
        return false;
    }
    if (found)
    {
        if (auto v = ReadJSONBool(obj, "enabled")) cfg.masterEffects.delay.enabled = *v;
        if (auto v = ReadJSONDouble(obj, "mix")) cfg.masterEffects.delay.mix = std::clamp(*v, 0.0, 1.0);
        if (auto v = ReadJSONDouble(obj, "timeSec")) cfg.masterEffects.delay.timeSec = std::clamp(*v, 0.01, 2.0);
        if (auto v = ReadJSONDouble(obj, "feedback")) cfg.masterEffects.delay.feedback = std::clamp(*v, 0.0, 0.95);
        if (auto v = ReadJSONBool(obj, "tempoSync")) cfg.masterEffects.delay.tempoSync = *v;
        if (auto v = ReadJSONDouble(obj, "syncBeats")) cfg.masterEffects.delay.syncBeats = std::clamp(*v, 0.125, 4.0);
    }

    if (!ExtractObjectForKey(effectsObj, "chorus", obj, found, err))
    {
        return false;
    }
    if (found)
    {
        if (auto v = ReadJSONBool(obj, "enabled")) cfg.masterEffects.chorus.enabled = *v;
        if (auto v = ReadJSONDouble(obj, "mix")) cfg.masterEffects.chorus.mix = std::clamp(*v, 0.0, 1.0);
        if (auto v = ReadJSONDouble(obj, "baseDelayMs")) cfg.masterEffects.chorus.baseDelayMs = std::clamp(*v, 2.0, 40.0);
        if (auto v = ReadJSONDouble(obj, "depthMs")) cfg.masterEffects.chorus.depthMs = std::clamp(*v, 0.0, 20.0);
        if (auto v = ReadJSONDouble(obj, "rateHz")) cfg.masterEffects.chorus.rateHz = std::clamp(*v, 0.05, 8.0);
        if (auto v = ReadJSONDouble(obj, "feedback")) cfg.masterEffects.chorus.feedback = std::clamp(*v, 0.0, 0.9);
    }

    if (!ExtractObjectForKey(effectsObj, "bitCrusher", obj, found, err))
    {
        return false;
    }
    if (found)
    {
        if (auto v = ReadJSONInt(obj, "bits")) cfg.masterEffects.bitCrusher.bits = std::clamp(*v, 1, 16);
    }

    if (!ExtractObjectForKey(effectsObj, "sampleRateReducer", obj, found, err))
    {
        return false;
    }
    if (found)
    {
        if (auto v = ReadJSONDouble(obj, "ratio")) cfg.masterEffects.sampleRateReducer.ratio = std::clamp(*v, 0.0, 1.0);
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
    if (auto v = ReadJSONString(text, "midiPath"))
    {
        cfg.midiPath = ResolvePathFromBase(baseDir, *v);
    }
    if (auto v = ReadJSONString(text, "wavPath"))
    {
        cfg.wavPath = ResolvePathFromBase(baseDir, *v);
    }
    if (auto v = ReadJSONInt(text, "targetChannel"))
    {
        cfg.targetChannel = *v;
    }
    if (auto v = ReadJSONInt(text, "initialSeconds"))
    {
        cfg.initialSeconds = *v;
    }
    if (auto v = ReadJSONInt(text, "bits"))
    {
        cfg.bits = *v;
    }
    if (auto v = ReadJSONInt(text, "sampleRate"))
    {
        cfg.sampleRate = *v;
    }
    if (auto v = ReadJSONDouble(text, "extraReleaseSec"))
    {
        cfg.extraReleaseSec = *v;
    }
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
    if (!LoadChannelsDiff(text, cfg, err))
    {
        return false;
    }
    if (!LoadChannelMixDiff(text, cfg, err))
    {
        return false;
    }
    if (!ParseEffectsObject(text, cfg, err))
    {
        return false;
    }

    return true;
}
} // namespace config::internal::load
