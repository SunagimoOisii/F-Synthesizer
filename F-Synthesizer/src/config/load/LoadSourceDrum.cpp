#include "Internal.h"

#include <algorithm>

#include "../ConfigFileInternal.h"

namespace config::internal::load
{
namespace
{
bool ParseDrumBusObject(const std::string& busObjText, DrumBusConfig& bus)
{
    if (auto v = ReadJSONBool(busObjText, "enabled")) bus.enabled = *v;
    if (auto v = ReadJSONDouble(busObjText, "level")) bus.level = std::clamp(*v, 0.0, 2.0);
    if (auto v = ReadJSONDouble(busObjText, "attackTrim")) bus.attackTrim = std::clamp(*v, 0.0, 1.0);
    if (auto v = ReadJSONDouble(busObjText, "sustainLift")) bus.sustainLift = std::clamp(*v, 0.0, 1.0);
    if (auto v = ReadJSONDouble(busObjText, "glue")) bus.glue = std::clamp(*v, 0.0, 1.0);
    if (auto v = ReadJSONDouble(busObjText, "presenceCut")) bus.presenceCut = std::clamp(*v, 0.0, 1.0);
    if (auto v = ReadJSONDouble(busObjText, "lowTighten")) bus.lowTighten = std::clamp(*v, 0.0, 1.0);
    if (auto v = ReadJSONDouble(busObjText, "roomSend")) bus.roomSend = std::clamp(*v, 0.0, 1.0);
    if (auto v = ReadJSONDouble(busObjText, "driveTrim")) bus.driveTrim = std::clamp(*v, 0.0, 1.0);
    return true;
}
} // namespace

bool ParseDrumKitSource(const std::string& sourceObjText, SourceConfig& outSource, std::string& err)
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
    std::string drumBusObj;
    bool drumBusFound = false;
    if (!ExtractObjectForKey(sourceObjText, "drumBus", drumBusObj, drumBusFound, err))
    {
        return false;
    }
    if (drumBusFound)
    {
        ParseDrumBusObject(drumBusObj, kit.drumBus);
    }
    if (auto v = ReadJSONDouble(sourceObjText, "velocityCeiling"))
    {
        kit.velocityCeiling = std::clamp(*v, 0.0, 1.0);
    }
    if (auto v = ReadJSONDouble(sourceObjText, "velocityCurve"))
    {
        kit.velocityCurve = std::clamp(*v, 0.2, 3.0);
    }
    outSource = kit;
    return true;
}
} // namespace config::internal::load
