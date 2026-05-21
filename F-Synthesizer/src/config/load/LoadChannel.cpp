#include "Internal.h"

#include <algorithm>

#include "../ConfigFileInternal.h"

namespace config::internal::load
{
namespace
{
bool TryParseAttackLayerType(const std::string& name, AttackLayerType& outType)
{
    if (name == "pick")
    {
        outType = AttackLayerType::Pick;
        return true;
    }
    if (name == "brass")
    {
        outType = AttackLayerType::Brass;
        return true;
    }
    if (name == "metal")
    {
        outType = AttackLayerType::Metal;
        return true;
    }
    return false;
}

bool TryParseBassLayerType(const std::string& name, BassLayerType& outType)
{
    if (name == "sub")
    {
        outType = BassLayerType::Sub;
        return true;
    }
    if (name == "drive")
    {
        outType = BassLayerType::Drive;
        return true;
    }
    if (name == "grit")
    {
        outType = BassLayerType::Grit;
        return true;
    }
    return false;
}

bool TryParseLeadLayerType(const std::string& name, LeadLayerType& outType)
{
    if (name == "blade")
    {
        outType = LeadLayerType::Blade;
        return true;
    }
    if (name == "brass")
    {
        outType = LeadLayerType::Brass;
        return true;
    }
    if (name == "edge")
    {
        outType = LeadLayerType::Edge;
        return true;
    }
    return false;
}

bool ParseAttackLayerObject(const std::string& layerObjText, AttackLayerConfig& layer, std::string& err)
{
    if (auto v = ReadJSONBool(layerObjText, "enabled")) layer.enabled = *v;
    if (auto v = ReadJSONString(layerObjText, "type"))
    {
        AttackLayerType parsed{};
        if (!TryParseAttackLayerType(*v, parsed))
        {
            err = "invalid attackLayer.type: " + *v;
            return false;
        }
        layer.type = parsed;
    }
    if (auto v = ReadJSONDouble(layerObjText, "level")) layer.level = std::clamp(*v, 0.0, 1.0);
    if (auto v = ReadJSONDouble(layerObjText, "decaySec")) layer.decaySec = std::clamp(*v, 0.001, 0.25);
    if (auto v = ReadJSONDouble(layerObjText, "brightness")) layer.brightness = std::clamp(*v, 0.0, 1.0);
    if (auto v = ReadJSONDouble(layerObjText, "bodyMix")) layer.bodyMix = std::clamp(*v, 0.0, 1.0);
    if (auto v = ReadJSONDouble(layerObjText, "pitchOffsetSemis")) layer.pitchOffsetSemis = std::clamp(*v, -24.0, 24.0);
    if (auto v = ReadJSONDouble(layerObjText, "drive")) layer.drive = std::clamp(*v, 0.0, 1.0);
    return true;
}

bool ParseBassLayerObject(const std::string& layerObjText, BassLayerConfig& layer, std::string& err)
{
    if (auto v = ReadJSONBool(layerObjText, "enabled")) layer.enabled = *v;
    if (auto v = ReadJSONString(layerObjText, "type"))
    {
        BassLayerType parsed{};
        if (!TryParseBassLayerType(*v, parsed))
        {
            err = "invalid bassLayer.type: " + *v;
            return false;
        }
        layer.type = parsed;
    }
    if (auto v = ReadJSONDouble(layerObjText, "level")) layer.level = std::clamp(*v, 0.0, 1.0);
    if (auto v = ReadJSONDouble(layerObjText, "subLevel")) layer.subLevel = std::clamp(*v, 0.0, 1.0);
    if (auto v = ReadJSONDouble(layerObjText, "bodyLevel")) layer.bodyLevel = std::clamp(*v, 0.0, 1.0);
    if (auto v = ReadJSONDouble(layerObjText, "gritLevel")) layer.gritLevel = std::clamp(*v, 0.0, 1.0);
    if (auto v = ReadJSONDouble(layerObjText, "cutoffHz")) layer.cutoffHz = std::clamp(*v, 40.0, 8000.0);
    if (auto v = ReadJSONDouble(layerObjText, "drive")) layer.drive = std::clamp(*v, 0.0, 1.0);
    if (auto v = ReadJSONDouble(layerObjText, "pitchOffsetSemis")) layer.pitchOffsetSemis = std::clamp(*v, -24.0, 24.0);
    if (auto v = ReadJSONDouble(layerObjText, "velocityToDrive")) layer.velocityToDrive = std::clamp(*v, 0.0, 1.0);
    if (auto v = ReadJSONDouble(layerObjText, "focusHz")) layer.focusHz = std::clamp(*v, 60.0, 1200.0);
    if (auto v = ReadJSONDouble(layerObjText, "focusLevel")) layer.focusLevel = std::clamp(*v, 0.0, 1.0);
    if (auto v = ReadJSONDouble(layerObjText, "bodySaturation")) layer.bodySaturation = std::clamp(*v, 0.0, 1.0);
    if (auto v = ReadJSONDouble(layerObjText, "gritTone")) layer.gritTone = std::clamp(*v, 0.0, 1.0);
    if (auto v = ReadJSONDouble(layerObjText, "attackBoost")) layer.attackBoost = std::clamp(*v, 0.0, 1.0);
    if (auto v = ReadJSONDouble(layerObjText, "attackDecaySec")) layer.attackDecaySec = std::clamp(*v, 0.005, 0.25);
    return true;
}

bool ParseLeadLayerObject(const std::string& layerObjText, LeadLayerConfig& layer, std::string& err)
{
    if (auto v = ReadJSONBool(layerObjText, "enabled")) layer.enabled = *v;
    if (auto v = ReadJSONString(layerObjText, "type"))
    {
        LeadLayerType parsed{};
        if (!TryParseLeadLayerType(*v, parsed))
        {
            err = "invalid leadLayer.type: " + *v;
            return false;
        }
        layer.type = parsed;
    }
    if (auto v = ReadJSONDouble(layerObjText, "level")) layer.level = std::clamp(*v, 0.0, 1.0);
    if (auto v = ReadJSONDouble(layerObjText, "edgeLevel")) layer.edgeLevel = std::clamp(*v, 0.0, 1.0);
    if (auto v = ReadJSONDouble(layerObjText, "bodyLevel")) layer.bodyLevel = std::clamp(*v, 0.0, 1.0);
    if (auto v = ReadJSONDouble(layerObjText, "detuneCents")) layer.detuneCents = std::clamp(*v, -50.0, 50.0);
    if (auto v = ReadJSONDouble(layerObjText, "pitchBendSemis")) layer.pitchBendSemis = std::clamp(*v, -12.0, 12.0);
    if (auto v = ReadJSONDouble(layerObjText, "bendDecaySec")) layer.bendDecaySec = std::clamp(*v, 0.005, 0.25);
    if (auto v = ReadJSONDouble(layerObjText, "attackBoost")) layer.attackBoost = std::clamp(*v, 0.0, 1.0);
    if (auto v = ReadJSONDouble(layerObjText, "attackDecaySec")) layer.attackDecaySec = std::clamp(*v, 0.005, 0.25);
    if (auto v = ReadJSONDouble(layerObjText, "drive")) layer.drive = std::clamp(*v, 0.0, 1.0);
    return true;
}

bool ParseChannelObject(const std::string& channelObjText, ChannelConfig& cfg, std::string& err)
{
    if (auto v = ReadJSONDouble(channelObjText, "amp")) cfg.amp = *v;
    if (auto v = ReadJSONDouble(channelObjText, "attackSec")) cfg.attackSec = *v;
    if (auto v = ReadJSONDouble(channelObjText, "decaySec")) cfg.decaySec = *v;
    if (auto v = ReadJSONDouble(channelObjText, "sustainLevel")) cfg.sustainLevel = *v;
    if (auto v = ReadJSONDouble(channelObjText, "releaseSec")) cfg.releaseSec = *v;
    if (auto v = ReadJSONDouble(channelObjText, "portamentoTimeSec"))
    {
        cfg.portamentoTimeSec = std::clamp(*v, 0.0, 30.0);
    }
    std::string attackLayerObj;
    bool foundAttackLayer = false;
    if (!ExtractObjectForKey(channelObjText, "attackLayer", attackLayerObj, foundAttackLayer, err))
    {
        return false;
    }
    if (foundAttackLayer && !ParseAttackLayerObject(attackLayerObj, cfg.attackLayer, err))
    {
        return false;
    }

    std::string bassLayerObj;
    bool foundBassLayer = false;
    if (!ExtractObjectForKey(channelObjText, "bassLayer", bassLayerObj, foundBassLayer, err))
    {
        return false;
    }
    if (foundBassLayer && !ParseBassLayerObject(bassLayerObj, cfg.bassLayer, err))
    {
        return false;
    }

    std::string leadLayerObj;
    bool foundLeadLayer = false;
    if (!ExtractObjectForKey(channelObjText, "leadLayer", leadLayerObj, foundLeadLayer, err))
    {
        return false;
    }
    if (foundLeadLayer && !ParseLeadLayerObject(leadLayerObj, cfg.leadLayer, err))
    {
        return false;
    }

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
