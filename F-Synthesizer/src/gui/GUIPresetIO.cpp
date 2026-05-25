#include "gui/GUIPresetIO.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <unordered_map>

#include "config/SourceJSON.h"
#include "config/SourceRegistry.h"
#include "gui/GUIActions.h"
#include "gui/GUIConfigUtils.h"
#include "gui/GUIStateModel.h"
#include "io/PlatformPaths.h"
#include "third_party/nlohmann/json.hpp"

namespace
{
using Json = nlohmann::json;

const char* AttackLayerTypeToString(AttackLayerType type)
{
    switch (type)
    {
    case AttackLayerType::Pick: return "pick";
    case AttackLayerType::Brass: return "brass";
    case AttackLayerType::Metal: return "metal";
    }
    return "pick";
}

const char* BassLayerTypeToString(BassLayerType type)
{
    switch (type)
    {
    case BassLayerType::Sub: return "sub";
    case BassLayerType::Drive: return "drive";
    case BassLayerType::Grit: return "grit";
    }
    return "drive";
}

const char* LeadLayerTypeToString(LeadLayerType type)
{
    switch (type)
    {
    case LeadLayerType::Blade: return "blade";
    case LeadLayerType::Brass: return "brass";
    case LeadLayerType::Edge: return "edge";
    }
    return "blade";
}

void WriteAttackLayerJSON(std::ostream& out, const AttackLayerConfig& layer)
{
    out << "      \"attackLayer\": {\n";
    out << "        \"enabled\": " << (layer.enabled ? "true" : "false") << ",\n";
    out << "        \"type\": \"" << AttackLayerTypeToString(layer.type) << "\",\n";
    out << "        \"level\": " << layer.level << ",\n";
    out << "        \"decaySec\": " << layer.decaySec << ",\n";
    out << "        \"brightness\": " << layer.brightness << ",\n";
    out << "        \"bodyMix\": " << layer.bodyMix << ",\n";
    out << "        \"pitchOffsetSemis\": " << layer.pitchOffsetSemis << ",\n";
    out << "        \"drive\": " << layer.drive << "\n";
    out << "      },\n";
}

void WriteBassLayerJSON(std::ostream& out, const BassLayerConfig& layer)
{
    out << "      \"bassLayer\": {\n";
    out << "        \"enabled\": " << (layer.enabled ? "true" : "false") << ",\n";
    out << "        \"type\": \"" << BassLayerTypeToString(layer.type) << "\",\n";
    out << "        \"level\": " << layer.level << ",\n";
    out << "        \"subLevel\": " << layer.subLevel << ",\n";
    out << "        \"bodyLevel\": " << layer.bodyLevel << ",\n";
    out << "        \"gritLevel\": " << layer.gritLevel << ",\n";
    out << "        \"cutoffHz\": " << layer.cutoffHz << ",\n";
    out << "        \"drive\": " << layer.drive << ",\n";
    out << "        \"pitchOffsetSemis\": " << layer.pitchOffsetSemis << ",\n";
    out << "        \"velocityToDrive\": " << layer.velocityToDrive << ",\n";
    out << "        \"focusHz\": " << layer.focusHz << ",\n";
    out << "        \"focusLevel\": " << layer.focusLevel << ",\n";
    out << "        \"bodySaturation\": " << layer.bodySaturation << ",\n";
    out << "        \"gritTone\": " << layer.gritTone << ",\n";
    out << "        \"attackBoost\": " << layer.attackBoost << ",\n";
    out << "        \"attackDecaySec\": " << layer.attackDecaySec << "\n";
    out << "      },\n";
}

void WriteLeadLayerJSON(std::ostream& out, const LeadLayerConfig& layer)
{
    out << "      \"leadLayer\": {\n";
    out << "        \"enabled\": " << (layer.enabled ? "true" : "false") << ",\n";
    out << "        \"type\": \"" << LeadLayerTypeToString(layer.type) << "\",\n";
    out << "        \"level\": " << layer.level << ",\n";
    out << "        \"edgeLevel\": " << layer.edgeLevel << ",\n";
    out << "        \"bodyLevel\": " << layer.bodyLevel << ",\n";
    out << "        \"detuneCents\": " << layer.detuneCents << ",\n";
    out << "        \"pitchBendSemis\": " << layer.pitchBendSemis << ",\n";
    out << "        \"bendDecaySec\": " << layer.bendDecaySec << ",\n";
    out << "        \"attackBoost\": " << layer.attackBoost << ",\n";
    out << "        \"attackDecaySec\": " << layer.attackDecaySec << ",\n";
    out << "        \"drive\": " << layer.drive << ",\n";
    out << "        \"characterLevel\": " << layer.characterLevel << ",\n";
    out << "        \"characterTone\": " << layer.characterTone << ",\n";
    out << "        \"biteLevel\": " << layer.biteLevel << ",\n";
    out << "        \"biteDecaySec\": " << layer.biteDecaySec << ",\n";
    out << "        \"wobbleDepthCents\": " << layer.wobbleDepthCents << ",\n";
    out << "        \"wobbleRateHz\": " << layer.wobbleRateHz << "\n";
    out << "      },\n";
}

void WriteChordLayerJSON(std::ostream& out, const ChordLayerConfig& layer)
{
    out << "      \"chordLayer\": {\n";
    out << "        \"enabled\": " << (layer.enabled ? "true" : "false") << ",\n";
    out << "        \"level\": " << layer.level << ",\n";
    out << "        \"intervalsSemis\": [";
    for (size_t i = 0; i < layer.intervalsSemis.size(); i++)
    {
        if (i > 0) out << ", ";
        out << layer.intervalsSemis[i];
    }
    out << "],\n";
    out << "        \"voiceLevels\": [";
    for (size_t i = 0; i < layer.voiceLevels.size(); i++)
    {
        if (i > 0) out << ", ";
        out << layer.voiceLevels[i];
    }
    out << "],\n";
    out << "        \"detuneCents\": " << layer.detuneCents << ",\n";
    out << "        \"spread\": " << layer.spread << ",\n";
    out << "        \"cutoffHz\": " << layer.cutoffHz << ",\n";
    out << "        \"drive\": " << layer.drive << "\n";
    out << "      },\n";
}

void WritePadLayerJSON(std::ostream& out, const PadLayerConfig& layer)
{
    out << "      \"padLayer\": {\n";
    out << "        \"enabled\": " << (layer.enabled ? "true" : "false") << ",\n";
    out << "        \"level\": " << layer.level << ",\n";
    out << "        \"octaveLevel\": " << layer.octaveLevel << ",\n";
    out << "        \"detuneCents\": " << layer.detuneCents << ",\n";
    out << "        \"spread\": " << layer.spread << ",\n";
    out << "        \"fadeInSec\": " << layer.fadeInSec << ",\n";
    out << "        \"brightness\": " << layer.brightness << ",\n";
    out << "        \"motionDepth\": " << layer.motionDepth << ",\n";
    out << "        \"motionRateHz\": " << layer.motionRateHz << ",\n";
    out << "        \"cutoffHz\": " << layer.cutoffHz << ",\n";
    out << "        \"drive\": " << layer.drive << "\n";
    out << "      },\n";
}

void WriteExpressionMapJSON(std::ostream& out, const ExpressionMapConfig& map)
{
    out << "      \"expressionMap\": {\n";
    out << "        \"enabled\": " << (map.enabled ? "true" : "false") << ",\n";
    out << "        \"velocityCurve\": " << map.velocityCurve << ",\n";
    out << "        \"velocityToAmp\": " << map.velocityToAmp << ",\n";
    out << "        \"velocityToBrightness\": " << map.velocityToBrightness << ",\n";
    out << "        \"velocityToFmIndex\": " << map.velocityToFmIndex << ",\n";
    out << "        \"velocityToAttack\": " << map.velocityToAttack << ",\n";
    out << "        \"velocityToBass\": " << map.velocityToBass << ",\n";
    out << "        \"velocityToLead\": " << map.velocityToLead << ",\n";
    out << "        \"velocityToChord\": " << map.velocityToChord << ",\n";
    out << "        \"velocityToPad\": " << map.velocityToPad << ",\n";
    out << "        \"modWheelToBrightness\": " << map.modWheelToBrightness << ",\n";
    out << "        \"modWheelToPad\": " << map.modWheelToPad << ",\n";
    out << "        \"pressureToDrive\": " << map.pressureToDrive << ",\n";
    out << "        \"cc74ToBrightness\": " << map.cc74ToBrightness << ",\n";
    out << "        \"cc74ToPadBrightness\": " << map.cc74ToPadBrightness << "\n";
    out << "      },\n";
}

struct PresetMeta
{
    std::vector<std::string> tags{};
    std::string description{};
    std::string displayName{};
    std::string category{};
    bool internalOnly = false;
    config::SourceKind sourceKind = config::SourceKind::Count;
};

std::string ToLower(std::string src)
{
    std::transform(src.begin(), src.end(), src.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return src;
}

config::SourceKind SourceKindFromTypeString(const std::string& type)
{
    const std::string lower = ToLower(type);
    config::SourceKind kind = config::SourceKind::Count;
    if (config::TryParseSourceKind(lower, kind))
    {
        return kind;
    }
    return config::SourceKind::Count;
}

std::string DisplayNameFromPresetName(const std::string& name)
{
    std::string display = name;
    const std::string retroPrefix = "retro_heavy_";
    if (display.rfind(retroPrefix, 0) == 0)
    {
        display = display.substr(retroPrefix.size());
    }
    const std::string demoPrefix = "demo_";
    if (display.rfind(demoPrefix, 0) == 0)
    {
        display = display.substr(demoPrefix.size());
    }
    for (char& c : display)
    {
        if (c == '_')
        {
            c = ' ';
        }
    }
    if (!display.empty())
    {
        display[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(display[0])));
    }
    return display;
}

std::string CategoryFromPresetNameAndTags(const std::string& name, const std::vector<std::string>& tags)
{
    auto has = [&](const char* key) {
        if (ToLower(name).find(key) != std::string::npos)
        {
            return true;
        }
        return std::any_of(tags.begin(), tags.end(), [&](const std::string& tag) {
            return tag.find(key) != std::string::npos;
        });
    };
    if (has("drum") || has("hat")) return "Drums";
    if (has("guitar")) return "Guitar";
    if (has("bass")) return "Bass";
    if (has("string")) return "Strings";
    if (has("brass")) return "Brass";
    if (has("reed")) return "Reed";
    if (has("pipe")) return "Pipe";
    if (has("lead")) return "Lead";
    if (has("pad")) return "Pad";
    if (has("key") || has("organ") || has("bell") || has("pluck")) return "Piano/Keys";
    if (has("sfx") || has("laser") || has("riser")) return "SFX";
    if (has("support")) return "SFX";
    return "Piano/Keys";
}

PresetMeta ReadPresetMeta(const std::filesystem::path& presetPath)
{
    PresetMeta meta{};
    std::ifstream in(presetPath, std::ios::binary);
    if (!in)
    {
        return meta;
    }

    Json root = Json::parse(in, nullptr, false);
    if (root.is_discarded() || !root.is_object())
    {
        return meta;
    }

    if (root.contains("description") && root["description"].is_string())
    {
        meta.description = root["description"].get<std::string>();
    }
    if (root.contains("displayName") && root["displayName"].is_string())
    {
        meta.displayName = root["displayName"].get<std::string>();
    }
    if (root.contains("category") && root["category"].is_string())
    {
        meta.category = root["category"].get<std::string>();
    }
    if (root.contains("internal") && root["internal"].is_boolean())
    {
        meta.internalOnly = root["internal"].get<bool>();
    }
    if (root.contains("tags") && root["tags"].is_array())
    {
        for (const auto& tag : root["tags"])
        {
            if (tag.is_string())
            {
                meta.tags.push_back(ToLower(tag.get<std::string>()));
            }
        }
    }
    for (const std::string& tag : meta.tags)
    {
        if (tag == "demo" || tag == "internal" || tag == "test" || tag == "inspection")
        {
            meta.internalOnly = true;
        }
    }
    const std::string stem = presetPath.stem().string();
    if (stem.rfind("demo_", 0) == 0)
    {
        meta.internalOnly = true;
    }
    if (meta.displayName.empty())
    {
        meta.displayName = DisplayNameFromPresetName(stem);
    }
    if (meta.category.empty())
    {
        meta.category = CategoryFromPresetNameAndTags(stem, meta.tags);
    }

    const Json* projectRoot = &root;
    if (root.contains("project") && root["project"].is_object())
    {
        projectRoot = &root["project"];
    }

    if (projectRoot->contains("instruments") && (*projectRoot)["instruments"].is_object())
    {
        for (auto it = (*projectRoot)["instruments"].begin(); it != (*projectRoot)["instruments"].end(); ++it)
        {
            if (!it.value().is_object())
            {
                continue;
            }
            if (it.value().contains("description") && it.value()["description"].is_string())
            {
                meta.description = it.value()["description"].get<std::string>();
            }
            if (it.value().contains("displayName") && it.value()["displayName"].is_string())
            {
                meta.displayName = it.value()["displayName"].get<std::string>();
            }
            if (it.value().contains("category") && it.value()["category"].is_string())
            {
                meta.category = it.value()["category"].get<std::string>();
            }
            if (it.value().contains("internal") && it.value()["internal"].is_boolean())
            {
                meta.internalOnly = it.value()["internal"].get<bool>();
            }
            if (meta.tags.empty() && it.value().contains("tags") && it.value()["tags"].is_array())
            {
                for (const auto& tag : it.value()["tags"])
                {
                    if (tag.is_string())
                    {
                        meta.tags.push_back(ToLower(tag.get<std::string>()));
                    }
                }
            }
            const auto soundIt = it.value().find("sound");
            if (soundIt == it.value().end() || !soundIt->is_object())
            {
                continue;
            }
            const auto srcIt = soundIt->find("source");
            if (srcIt == soundIt->end() || !srcIt->is_object())
            {
                continue;
            }
            const auto typeIt = srcIt->find("type");
            if (typeIt == srcIt->end() || !typeIt->is_string())
            {
                continue;
            }
            meta.sourceKind = SourceKindFromTypeString(typeIt->get<std::string>());
            if (meta.sourceKind != config::SourceKind::Count)
            {
                break;
            }
        }
    }
    else if (projectRoot->contains("channels") && (*projectRoot)["channels"].is_object())
    {
        for (auto it = (*projectRoot)["channels"].begin(); it != (*projectRoot)["channels"].end(); ++it)
        {
            if (!it.value().is_object())
            {
                continue;
            }
            const auto srcIt = it.value().find("source");
            if (srcIt == it.value().end() || !srcIt->is_object())
            {
                continue;
            }
            const auto typeIt = srcIt->find("type");
            if (typeIt == srcIt->end() || !typeIt->is_string())
            {
                continue;
            }
            meta.sourceKind = SourceKindFromTypeString(typeIt->get<std::string>());
            if (meta.sourceKind != config::SourceKind::Count)
            {
                break;
            }
        }
    }

    return meta;
}

int FindPresetIndex(const GUIState& state, const std::string& name)
{
    for (int i = 0; i < static_cast<int>(state.presetItems.size()); i++)
    {
        if (state.presetItems[i] == name)
        {
            return i;
        }
    }
    return -1;
}

bool PresetMatchesSourceKind(const std::string& presetName, config::SourceKind kind, const PresetMeta& meta)
{
    if (meta.sourceKind != config::SourceKind::Count)
    {
        if (kind == config::SourceKind::Noise && presetName.rfind("psg_noise_", 0) == 0)
        {
            return true;
        }
        return meta.sourceKind == kind;
    }

    switch (kind)
    {
    case config::SourceKind::Waveform:
        return presetName.rfind("wave_", 0) == 0;
    case config::SourceKind::Analog:
        return presetName.rfind("analog_", 0) == 0;
    case config::SourceKind::Fm:
        return presetName.rfind("fm_", 0) == 0;
    case config::SourceKind::Psg:
        return presetName.rfind("psg_", 0) == 0;
    case config::SourceKind::DrumKit:
        return presetName.rfind("drumkit_", 0) == 0;
    case config::SourceKind::Noise:
        return presetName.rfind("noise_", 0) == 0 || presetName.rfind("psg_noise_", 0) == 0;
    case config::SourceKind::Drum:
    case config::SourceKind::Count:
    default:
        return false;
    }
}
} // namespace

namespace gui
{
bool SavePresetDiffFile(const GUIPresetSnapshot& snapshot, const std::filesystem::path& presetPath, std::string& err)
{
    AppConfig base = DefaultConfig();
    if (!base.channelConfigs)
    {
        err = "default channel configs are not initialized";
        return false;
    }

    std::error_code ec;
    std::filesystem::create_directories(presetPath.parent_path(), ec);
    if (ec)
    {
        err = "failed to create preset directory: " + ec.message();
        return false;
    }
    std::ofstream out(presetPath, std::ios::binary | std::ios::trunc);
    if (!out)
    {
        err = "failed to open preset file";
        return false;
    }

    const std::string stem = presetPath.stem().string();

    out << "{\n";
    out << "  \"format\": \"projectModel.v3\",\n";
    out << "  \"project\": {\n";
    out << "    \"instruments\": {\n";

    bool first = true;
    for (int ch = 0; ch < 16; ch++)
    {
        const ChannelConfig& cur = snapshot.channelConfigs[ch];
        const ChannelConfig& def = (*base.channelConfigs)[ch];
        if (ChannelConfigEquals(cur, def))
        {
            continue;
        }
        if (!first) out << ",\n";
        first = false;
        out << "      \"" << stem << "__ch" << ch << "\": {\n";
        out << "        \"displayName\": \"\",\n";
        out << "        \"category\": \"\",\n";
        out << "        \"internal\": false,\n";
        out << "        \"tags\": [],\n";
        out << "        \"description\": \"\",\n";
        out << "        \"recommendedRange\": { \"low\": 48, \"high\": 84, \"preview\": 60 },\n";
        out << "        \"macroHints\": [],\n";
        out << "        \"sound\": {\n";
        out << "        \"amp\": " << cur.amp << ",\n";
        out << "        \"attackSec\": " << cur.attackSec << ",\n";
        out << "        \"decaySec\": " << cur.decaySec << ",\n";
        out << "        \"sustainLevel\": " << cur.sustainLevel << ",\n";
        out << "        \"releaseSec\": " << cur.releaseSec << ",\n";
        if (cur.attackLayer.enabled)
        {
            WriteAttackLayerJSON(out, cur.attackLayer);
        }
        if (cur.bassLayer.enabled)
        {
            WriteBassLayerJSON(out, cur.bassLayer);
        }
        if (cur.leadLayer.enabled)
        {
            WriteLeadLayerJSON(out, cur.leadLayer);
        }
        if (cur.chordLayer.enabled)
        {
            WriteChordLayerJSON(out, cur.chordLayer);
        }
        if (cur.padLayer.enabled)
        {
            WritePadLayerJSON(out, cur.padLayer);
        }
        if (cur.expressionMap.enabled)
        {
            WriteExpressionMapJSON(out, cur.expressionMap);
        }
        config::WriteSourceJSON(out, cur.source, 8);
        out << "\n        }\n";
        out << "      }";
    }

    out << "\n    },\n";
    out << "    \"channels\": {\n";
    first = true;
    for (int ch = 0; ch < 16; ch++)
    {
        const ChannelConfig& cur = snapshot.channelConfigs[ch];
        const ChannelConfig& def = (*base.channelConfigs)[ch];
        if (ChannelConfigEquals(cur, def))
        {
            continue;
        }
        if (!first) out << ",\n";
        first = false;
        out << "      \"" << ch << "\": {\n";
        out << "        \"instrumentId\": \"" << stem << "__ch" << ch << "\"\n";
        out << "      }";
    }
    out << "\n    }\n";
    out << "  }\n";
    out << "}\n";
    return true;
}

std::vector<std::string> CollectPresetItems(const std::filesystem::path& projectRoot)
{
    std::vector<std::string> names;
    const std::filesystem::path dir = projectRoot / "config" / "presets";

    std::error_code ec;
    if (std::filesystem::exists(dir, ec))
    {
        std::filesystem::directory_iterator it(dir, ec);
        const std::filesystem::directory_iterator end;
        for (; it != end && !ec; it.increment(ec))
        {
            std::error_code fileEc;
            if (!it->is_regular_file(fileEc) || fileEc) continue;
            const auto& ent = *it;
            if (ent.path().extension() != ".json") continue;
            names.push_back(ent.path().stem().string());
        }
    }

    std::sort(names.begin(), names.end());
    const auto it = std::find(names.begin(), names.end(), "retro_heavy_fm_brass_ensemble");
    if (it != names.end() && it != names.begin())
    {
        std::rotate(names.begin(), it, it + 1);
    }
    return names;
}

bool LoadPresetConfig(
    const std::filesystem::path& projectRoot,
    const std::string& presetName,
    AppConfig& cfg,
    std::string& err)
{
    const std::filesystem::path basePath = projectRoot / "config" / "base.json";
    const std::filesystem::path presetPath = projectRoot / "config" / "presets" / (presetName + ".json");

    cfg = DefaultConfig();
    std::error_code existsEc;
    if (std::filesystem::exists(basePath, existsEc))
    {
        if (!LoadConfigFile(basePath, cfg, err))
        {
            err = "failed to load base config: " + err;
            return false;
        }
    }
    else if (existsEc)
    {
        err = "failed to inspect base config: " + existsEc.message();
        return false;
    }
    if (!LoadConfigFile(presetPath, cfg, err))
    {
        err = "failed to load preset config: " + err;
        return false;
    }
    return true;
}

void RefreshPresetItems(GUIState& state, const std::string& preferName)
{
    const std::filesystem::path projectRoot = FindProjectRootPath();
    std::vector<std::string> all = CollectPresetItems(FindProjectRootPath());
    std::unordered_map<std::string, PresetMeta> metaByName;
    metaByName.reserve(all.size());
    for (const auto& name : all)
    {
        const std::filesystem::path presetPath = projectRoot / "config" / "presets" / (name + ".json");
        metaByName.emplace(name, ReadPresetMeta(presetPath));
    }

    EnsureChannelConfigs(state);
    const int slot = std::clamp(state.selectedSoundSlot, 0, 15);
    const config::SourceKind kind = config::SourceConfigKind((*state.channelConfigs)[slot].source);
    state.presetItems.clear();
    state.presetItemTags.clear();
    state.presetItemDescriptions.clear();
    state.presetItemDisplayNames.clear();
    state.presetItemCategories.clear();
    state.presetItemInternal.clear();
    state.presetItems.reserve(all.size());
    state.presetItemTags.reserve(all.size());
    state.presetItemDescriptions.reserve(all.size());
    state.presetItemDisplayNames.reserve(all.size());
    state.presetItemCategories.reserve(all.size());
    state.presetItemInternal.reserve(all.size());
    const bool playMode = (state.UIModeTab == 0);
    const bool advancedMode = (state.UIModeTab == 3);
    for (const auto& name : all)
    {
        const auto it = metaByName.find(name);
        const PresetMeta* meta = (it != metaByName.end()) ? &it->second : nullptr;
        const bool includeByMode = meta != nullptr
            && ((playMode && !meta->internalOnly)
                || advancedMode
                || (!playMode && !advancedMode && !meta->internalOnly && PresetMatchesSourceKind(name, kind, *meta)));
        if (includeByMode)
        {
            state.presetItems.push_back(name);
            state.presetItemTags.push_back(meta->tags);
            state.presetItemDescriptions.push_back(meta->description);
            state.presetItemDisplayNames.push_back(meta->displayName);
            state.presetItemCategories.push_back(meta->category);
            state.presetItemInternal.push_back(meta->internalOnly);
        }
    }
    const bool allowUnfilteredFallback = (kind == config::SourceKind::Count);
    if (state.presetItems.empty() && allowUnfilteredFallback)
    {
        state.presetItems = std::move(all);
        state.presetItemTags.clear();
        state.presetItemDescriptions.clear();
        state.presetItemDisplayNames.clear();
        state.presetItemCategories.clear();
        state.presetItemInternal.clear();
        state.presetItemTags.reserve(state.presetItems.size());
        state.presetItemDescriptions.reserve(state.presetItems.size());
        state.presetItemDisplayNames.reserve(state.presetItems.size());
        state.presetItemCategories.reserve(state.presetItems.size());
        state.presetItemInternal.reserve(state.presetItems.size());
        for (const auto& name : state.presetItems)
        {
            const auto it = metaByName.find(name);
            if (it != metaByName.end())
            {
                state.presetItemTags.push_back(it->second.tags);
                state.presetItemDescriptions.push_back(it->second.description);
                state.presetItemDisplayNames.push_back(it->second.displayName);
                state.presetItemCategories.push_back(it->second.category);
                state.presetItemInternal.push_back(it->second.internalOnly);
            }
            else
            {
                state.presetItemTags.emplace_back();
                state.presetItemDescriptions.emplace_back();
                state.presetItemDisplayNames.push_back(DisplayNameFromPresetName(name));
                state.presetItemCategories.push_back(CategoryFromPresetNameAndTags(name, state.presetItemTags.back()));
                state.presetItemInternal.push_back(name.rfind("demo_", 0) == 0);
            }
        }
    }
    int idx = FindPresetIndex(state, preferName);
    if (idx < 0)
    {
        idx = FindPresetIndex(state, "retro_heavy_fm_brass_ensemble");
    }
    if (state.presetItems.empty())
    {
        state.presetIndex = -1;
    }
    else
    {
        state.presetIndex = (idx >= 0) ? idx : 0;
    }
}

bool ApplySelectedPresetPaths(GUIState& state, std::string& err)
{
    if (state.presetItems.empty())
    {
        err = "preset list is empty";
        return false;
    }
    if (state.presetIndex < 0 || state.presetIndex >= static_cast<int>(state.presetItems.size()))
    {
        err = "invalid preset index";
        return false;
    }

    const std::string& presetName = state.presetItems[state.presetIndex];
    strncpy_s(state.presetName, sizeof(state.presetName), presetName.c_str(), _TRUNCATE);
    AppConfig cfg{};
    if (!LoadPresetConfig(FindProjectRootPath(), presetName, cfg, err))
    {
        return false;
    }

    EnsureChannelConfigs(state);
    if (cfg.channelConfigs)
    {
        AppConfig def = DefaultConfig();
        std::vector<int> changedChannels;
        if (def.channelConfigs)
        {
            for (int ch = 0; ch < 16; ch++)
            {
                if (!ChannelConfigEquals((*cfg.channelConfigs)[ch], (*def.channelConfigs)[ch]))
                {
                    changedChannels.push_back(ch);
                }
            }
        }

        if (changedChannels.size() == 1)
        {
            const int dstSlot = std::clamp(state.selectedSoundSlot, 0, 15);
            const int srcCh = changedChannels.front();
            (*state.channelConfigs)[dstSlot] = (*cfg.channelConfigs)[srcCh];
        }
        else
        {
            *state.channelConfigs = *cfg.channelConfigs;
        }
    }
    return true;
}

bool SavePresetDiffFromState(const GUIState& state, const std::filesystem::path& presetPath, std::string& err)
{
    if (!state.channelConfigs)
    {
        err = "channel configs are not initialized";
        return false;
    }
    GUIPresetSnapshot snapshot{};
    snapshot.channelConfigs = *state.channelConfigs;
    return SavePresetDiffFile(snapshot, presetPath, err);
}
} // namespace gui
