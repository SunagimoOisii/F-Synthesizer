#include "Internal.h"

#include "../ConfigFileInternal.h"

namespace config::internal::load
{
namespace
{
bool ParseChannelObject(const std::string& channelObjText, ChannelConfig& cfg, std::string& err)
{
    if (auto v = ReadJSONDouble(channelObjText, "amp")) cfg.amp = *v;
    if (auto v = ReadJSONDouble(channelObjText, "attackSec")) cfg.attackSec = *v;
    if (auto v = ReadJSONDouble(channelObjText, "decaySec")) cfg.decaySec = *v;
    if (auto v = ReadJSONDouble(channelObjText, "sustainLevel")) cfg.sustainLevel = *v;
    if (auto v = ReadJSONDouble(channelObjText, "releaseSec")) cfg.releaseSec = *v;

    std::string sourceObj;
    bool found = false;
    if (!ExtractObjectForKey(channelObjText, "source", sourceObj, found, err))
    {
        return false;
    }
    if (found)
    {
        SourceConfig s = cfg.source;
        if (!ParseSourceObject(sourceObj, s, err))
        {
            return false;
        }
        cfg.source = s;
    }
    return true;
}

bool ParseChannelMixObject(const std::string& mixObjText, ChannelMixState& mix, std::string& err)
{
    if (auto v = ReadJSONBool(mixObjText, "mute")) mix.mute = *v;
    if (auto v = ReadJSONBool(mixObjText, "solo")) mix.solo = *v;
    if (auto v = ReadJSONDouble(mixObjText, "level")) mix.level = *v;
    if (auto v = ReadJSONDouble(mixObjText, "pan")) mix.pan = *v;
    if (auto v = ReadJSONDouble(mixObjText, "gain")) mix.gain = *v;

    if (mix.level < 0.0 || mix.level > 2.0)
    {
        err = "level must be in range 0.0..2.0";
        return false;
    }
    if (mix.pan < -1.0 || mix.pan > 1.0)
    {
        err = "pan must be in range -1.0..1.0";
        return false;
    }
    if (mix.gain < 0.0 || mix.gain > 4.0)
    {
        err = "gain must be in range 0.0..4.0";
        return false;
    }
    return true;
}
} // namespace

bool LoadChannelsDiff(const std::string& text, AppConfig& cfg, std::string& err)
{
    std::string channelsObj;
    bool found = false;
    if (!ExtractObjectForKey(text, "channels", channelsObj, found, err))
    {
        return false;
    }
    if (!found)
    {
        return true;
    }

    // 既定値を基底に差分だけを適用し、preset互換を維持する。
    auto table = MakeMutableChannelConfigs(cfg);
    if (!ParseTopLevelObjectEntries(channelsObj, [&](const std::string& k, const std::string& valueObj) {
        int ch = -1;
        try
        {
            ch = std::stoi(k);
        }
        catch (...)
        {
            err = "invalid channel key: " + k;
            return false;
        }
        if (ch < 0 || ch > 15)
        {
            err = "channel key out of range: " + k;
            return false;
        }
        ChannelConfig chCfg = (*table)[ch];
        if (!ParseChannelObject(valueObj, chCfg, err))
        {
            err = "channel " + k + ": " + err;
            return false;
        }
        (*table)[ch] = chCfg;
        return true;
        }, err))
    {
        return false;
    }

    cfg.channelConfigs = table;
    return true;
}

bool LoadChannelMixDiff(const std::string& text, AppConfig& cfg, std::string& err)
{
    std::string mixObj;
    bool found = false;
    if (!ExtractObjectForKey(text, "channelMix", mixObj, found, err))
    {
        return false;
    }
    if (!found)
    {
        return true;
    }

    // channelMix も channels と同じく「差分マージ」を採用する。
    auto table = MakeMutableChannelMixStates(cfg);
    if (!ParseTopLevelObjectEntries(mixObj, [&](const std::string& k, const std::string& valueObj) {
        int ch = -1;
        try
        {
            ch = std::stoi(k);
        }
        catch (...)
        {
            err = "invalid channelMix key: " + k;
            return false;
        }
        if (ch < 0 || ch > 15)
        {
            err = "channelMix key out of range: " + k;
            return false;
        }
        ChannelMixState mix = (*table)[ch];
        if (!ParseChannelMixObject(valueObj, mix, err))
        {
            err = "channelMix " + k + ": " + err;
            return false;
        }
        (*table)[ch] = mix;
        return true;
        }, err))
    {
        return false;
    }

    cfg.channelMixStates = table;
    return true;
}
} // namespace config::internal::load
