#include "Internal.h"

#include "../ConfigFileInternal.h"

namespace config::internal::load
{
bool ParseDrumSource(const std::string& sourceObjText, SourceConfig& outSource, std::string& err)
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
    outSource = kit;
    return true;
}
} // namespace config::internal::load
