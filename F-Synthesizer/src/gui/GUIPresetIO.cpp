#include "gui/GUIPresetIO.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <utility>

#include "config/SourceJSON.h"
#include "config/SourceRegistry.h"
#include "gui/GUIActions.h"
#include "gui/GUIConfigUtils.h"
#include "gui/GUIProjectFacade.h"
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

Json AttackLayerToJson(const AttackLayerConfig& layer)
{
    return Json{
        {"enabled", layer.enabled},
        {"type", AttackLayerTypeToString(layer.type)},
        {"level", layer.level},
        {"decaySec", layer.decaySec},
        {"brightness", layer.brightness},
        {"bodyMix", layer.bodyMix},
        {"pitchOffsetSemis", layer.pitchOffsetSemis},
        {"drive", layer.drive},
    };
}

Json BassLayerToJson(const BassLayerConfig& layer)
{
    return Json{
        {"enabled", layer.enabled},
        {"type", BassLayerTypeToString(layer.type)},
        {"level", layer.level},
        {"subLevel", layer.subLevel},
        {"bodyLevel", layer.bodyLevel},
        {"gritLevel", layer.gritLevel},
        {"cutoffHz", layer.cutoffHz},
        {"drive", layer.drive},
        {"pitchOffsetSemis", layer.pitchOffsetSemis},
        {"velocityToDrive", layer.velocityToDrive},
        {"focusHz", layer.focusHz},
        {"focusLevel", layer.focusLevel},
        {"bodySaturation", layer.bodySaturation},
        {"gritTone", layer.gritTone},
        {"attackBoost", layer.attackBoost},
        {"attackDecaySec", layer.attackDecaySec},
    };
}

Json LeadLayerToJson(const LeadLayerConfig& layer)
{
    return Json{
        {"enabled", layer.enabled},
        {"type", LeadLayerTypeToString(layer.type)},
        {"level", layer.level},
        {"edgeLevel", layer.edgeLevel},
        {"bodyLevel", layer.bodyLevel},
        {"detuneCents", layer.detuneCents},
        {"pitchBendSemis", layer.pitchBendSemis},
        {"bendDecaySec", layer.bendDecaySec},
        {"attackBoost", layer.attackBoost},
        {"attackDecaySec", layer.attackDecaySec},
        {"drive", layer.drive},
        {"characterLevel", layer.characterLevel},
        {"characterTone", layer.characterTone},
        {"biteLevel", layer.biteLevel},
        {"biteDecaySec", layer.biteDecaySec},
        {"wobbleDepthCents", layer.wobbleDepthCents},
        {"wobbleRateHz", layer.wobbleRateHz},
    };
}

Json ChordLayerToJson(const ChordLayerConfig& layer)
{
    return Json{
        {"enabled", layer.enabled},
        {"level", layer.level},
        {"intervalsSemis", layer.intervalsSemis},
        {"voiceLevels", layer.voiceLevels},
        {"detuneCents", layer.detuneCents},
        {"spread", layer.spread},
        {"cutoffHz", layer.cutoffHz},
        {"drive", layer.drive},
    };
}

Json PadLayerToJson(const PadLayerConfig& layer)
{
    return Json{
        {"enabled", layer.enabled},
        {"level", layer.level},
        {"octaveLevel", layer.octaveLevel},
        {"detuneCents", layer.detuneCents},
        {"spread", layer.spread},
        {"fadeInSec", layer.fadeInSec},
        {"brightness", layer.brightness},
        {"motionDepth", layer.motionDepth},
        {"motionRateHz", layer.motionRateHz},
        {"cutoffHz", layer.cutoffHz},
        {"drive", layer.drive},
    };
}

Json ExpressionMapToJson(const ExpressionMapConfig& map)
{
    return Json{
        {"enabled", map.enabled},
        {"velocityCurve", map.velocityCurve},
        {"velocityToAmp", map.velocityToAmp},
        {"velocityToBrightness", map.velocityToBrightness},
        {"velocityToFmIndex", map.velocityToFmIndex},
        {"velocityToAttack", map.velocityToAttack},
        {"velocityToBass", map.velocityToBass},
        {"velocityToLead", map.velocityToLead},
        {"velocityToChord", map.velocityToChord},
        {"velocityToPad", map.velocityToPad},
        {"modWheelToBrightness", map.modWheelToBrightness},
        {"modWheelToPad", map.modWheelToPad},
        {"pressureToDrive", map.pressureToDrive},
        {"cc74ToBrightness", map.cc74ToBrightness},
        {"cc74ToPadBrightness", map.cc74ToPadBrightness},
    };
}

bool SourceToJson(const SourceConfig& source, Json& out, std::string& err)
{
    std::ostringstream serialized;
    config::WriteSourceJSON(serialized, source, 0);
    Json parsed = Json::parse("{" + serialized.str() + "}", nullptr, false);
    if (parsed.is_discarded() || !parsed.is_object() || !parsed.contains("source") || !parsed["source"].is_object())
    {
        err = "failed to serialize source config";
        return false;
    }
    out = std::move(parsed["source"]);
    return true;
}

bool ChannelSoundToJson(const InstrumentSoundConfig& config, Json& out, std::string& err)
{
    out = Json{
        {"amp", config.amp},
        {"attackSec", config.attackSec},
        {"decaySec", config.decaySec},
        {"sustainLevel", config.sustainLevel},
        {"releaseSec", config.releaseSec},
    };
    if (config.attackLayer.enabled)
    {
        out["attackLayer"] = AttackLayerToJson(config.attackLayer);
    }
    if (config.bassLayer.enabled)
    {
        out["bassLayer"] = BassLayerToJson(config.bassLayer);
    }
    if (config.leadLayer.enabled)
    {
        out["leadLayer"] = LeadLayerToJson(config.leadLayer);
    }
    if (config.chordLayer.enabled)
    {
        out["chordLayer"] = ChordLayerToJson(config.chordLayer);
    }
    if (config.padLayer.enabled)
    {
        out["padLayer"] = PadLayerToJson(config.padLayer);
    }
    if (config.expressionMap.enabled)
    {
        out["expressionMap"] = ExpressionMapToJson(config.expressionMap);
    }
    Json source = Json::object();
    if (!SourceToJson(config.source, source, err))
    {
        return false;
    }
    out["source"] = std::move(source);
    return true;
}

struct PresetMeta
{
    std::vector<std::string> tags{};
    std::string description{};
    std::string displayName{};
    std::string category{};
    GUIPresetItem::RecommendedRange recommendedRange{};
    std::vector<GUIPresetItem::MacroHint> macroHints{};
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
    if (has("lead")) return "Lead";
    if (has("string") || has("brass") || has("reed")) return "Pad";
    if (has("pad")) return "Pad";
    if (has("key") || has("organ") || has("bell") || has("pluck")) return "Keys";
    if (has("sfx") || has("laser") || has("riser")) return "SFX";
    if (has("support") || has("pipe")) return "Support";
    return "Keys";
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
            if (it.value().contains("recommendedRange") && it.value()["recommendedRange"].is_object())
            {
                const auto& range = it.value()["recommendedRange"];
                if (range.contains("low") && range["low"].is_number_integer()
                    && range.contains("high") && range["high"].is_number_integer()
                    && range.contains("preview") && range["preview"].is_number_integer())
                {
                    meta.recommendedRange.low = std::clamp(range["low"].get<int>(), 0, 127);
                    meta.recommendedRange.high = std::clamp(range["high"].get<int>(), 0, 127);
                    meta.recommendedRange.preview = std::clamp(range["preview"].get<int>(), 0, 127);
                    meta.recommendedRange.available = true;
                }
            }
            if (it.value().contains("macroHints") && it.value()["macroHints"].is_array())
            {
                meta.macroHints.clear();
                for (const auto& hint : it.value()["macroHints"])
                {
                    if (!hint.is_object())
                    {
                        continue;
                    }
                    GUIPresetItem::MacroHint out{};
                    if (hint.contains("id") && hint["id"].is_string())
                    {
                        out.id = hint["id"].get<std::string>();
                    }
                    if (hint.contains("label") && hint["label"].is_string())
                    {
                        out.label = hint["label"].get<std::string>();
                    }
                    if (hint.contains("description") && hint["description"].is_string())
                    {
                        out.description = hint["description"].get<std::string>();
                    }
                    if (!out.id.empty())
                    {
                        meta.macroHints.push_back(std::move(out));
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
        if (state.presetItems[i].name == name)
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

std::array<InstrumentSoundConfig, 16> SoundSlotsFromProject(const ProjectModel& project)
{
    std::array<InstrumentSoundConfig, 16> sounds{};
    if (!project.instruments || !project.projectChannels)
    {
        return sounds;
    }
    for (int ch = 0; ch < 16; ch++)
    {
        const auto& channel = (*project.projectChannels)[ch];
        const auto it = project.instruments->find(channel.instrumentId);
        if (it != project.instruments->end())
        {
            sounds[static_cast<size_t>(ch)] = it->second.sound;
        }
    }
    return sounds;
}

bool LoadPresetProject(
    const std::filesystem::path& projectRoot,
    const std::string& presetName,
    ProjectModel& project,
    std::string& err)
{
    const std::filesystem::path basePath = projectRoot / "config" / "base.json";
    const std::filesystem::path presetPath = projectRoot / "config" / "presets" / (presetName + ".json");

    project = DefaultProjectModel();
    std::error_code existsEc;
    if (std::filesystem::exists(basePath, existsEc))
    {
        if (!LoadProjectModelFile(basePath, project, err))
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
    if (!LoadProjectModelFile(presetPath, project, err))
    {
        err = "failed to load preset config: " + err;
        return false;
    }
    return true;
}
} // namespace

namespace gui
{
bool SavePresetDiffFile(const GUIPresetSnapshot& snapshot, const std::filesystem::path& presetPath, std::string& err)
{
    const std::array<InstrumentSoundConfig, 16> defaultSounds = SoundSlotsFromProject(DefaultProjectModel());

    std::error_code ec;
    std::filesystem::create_directories(presetPath.parent_path(), ec);
    if (ec)
    {
        err = "failed to create preset directory: " + ec.message();
        return false;
    }
    const std::string stem = presetPath.stem().string();

    Json instruments = Json::object();
    Json channels = Json::object();
    for (int ch = 0; ch < 16; ch++)
    {
        const InstrumentSoundConfig& cur = snapshot.soundSlots[ch];
        const InstrumentSoundConfig& def = defaultSounds[static_cast<size_t>(ch)];
        if (InstrumentSoundConfigEquals(cur, def))
        {
            continue;
        }
        Json sound = Json::object();
        if (!ChannelSoundToJson(cur, sound, err))
        {
            return false;
        }
        const std::string instrumentId = stem + "__ch" + std::to_string(ch);
        instruments[instrumentId] = Json{
            {"displayName", ""},
            {"category", ""},
            {"internal", false},
            {"tags", Json::array()},
            {"description", ""},
            {"recommendedRange", Json{{"low", 48}, {"high", 84}, {"preview", 60}}},
            {"macroHints", Json::array()},
            {"sound", std::move(sound)},
        };
        channels[std::to_string(ch)] = Json{
            {"instrumentId", instrumentId},
        };
    }

    Json root = Json{
        {"format", "projectModel.v3"},
        {"project", Json{
            {"instruments", std::move(instruments)},
            {"channels", std::move(channels)},
        }},
    };

    std::ofstream out(presetPath, std::ios::binary | std::ios::trunc);
    if (!out)
    {
        err = "failed to open preset file";
        return false;
    }
    out << root.dump(2) << '\n';
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
    const auto it = std::find(names.begin(), names.end(), "sound_lead_blade");
    if (it != names.end() && it != names.begin())
    {
        std::rotate(names.begin(), it, it + 1);
    }
    return names;
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

    EnsureSoundSlots(state);
    const int slot = std::clamp(state.selectedSoundSlot, 0, 15);
    const config::SourceKind kind = config::SourceConfigKind(ReadSoundSlot(state, slot).source);
    state.presetItems.clear();
    state.presetItems.reserve(all.size());
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
            GUIPresetItem item{};
            item.name = name;
            item.tags = meta->tags;
            item.description = meta->description;
            item.displayName = meta->displayName;
            item.category = meta->category;
            item.recommendedRange = meta->recommendedRange;
            item.macroHints = meta->macroHints;
            item.internalOnly = meta->internalOnly;
            state.presetItems.push_back(std::move(item));
        }
    }
    const bool allowUnfilteredFallback = (kind == config::SourceKind::Count);
    if (state.presetItems.empty() && allowUnfilteredFallback)
    {
        state.presetItems.clear();
        state.presetItems.reserve(all.size());
        for (const auto& name : all)
        {
            GUIPresetItem item{};
            item.name = name;
            const auto it = metaByName.find(name);
            if (it != metaByName.end())
            {
                item.tags = it->second.tags;
                item.description = it->second.description;
                item.displayName = it->second.displayName;
                item.category = it->second.category;
                item.recommendedRange = it->second.recommendedRange;
                item.macroHints = it->second.macroHints;
                item.internalOnly = it->second.internalOnly;
            }
            else
            {
                item.displayName = DisplayNameFromPresetName(name);
                item.category = CategoryFromPresetNameAndTags(name, item.tags);
                item.internalOnly = name.rfind("demo_", 0) == 0;
            }
            state.presetItems.push_back(std::move(item));
        }
    }
    int idx = FindPresetIndex(state, preferName);
    if (idx < 0)
    {
        idx = FindPresetIndex(state, "sound_lead_blade");
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

    const std::string& presetName = state.presetItems[state.presetIndex].name;
    const GUIPresetItem& selectedItem = state.presetItems[state.presetIndex];
    strncpy_s(state.presetName, sizeof(state.presetName), presetName.c_str(), _TRUNCATE);
    if (selectedItem.recommendedRange.available)
    {
        state.tonePreviewNoteNumber = selectedItem.recommendedRange.preview;
    }
    ProjectModel presetProject{};
    if (!LoadPresetProject(FindProjectRootPath(), presetName, presetProject, err))
    {
        return false;
    }

    EnsureSoundSlots(state);
    const std::array<InstrumentSoundConfig, 16> presetSounds = SoundSlotsFromProject(presetProject);
    const std::array<InstrumentSoundConfig, 16> defaultSounds = SoundSlotsFromProject(DefaultProjectModel());
    if (presetProject.instruments && presetProject.projectChannels)
    {
        std::vector<int> changedChannels;
        for (int ch = 0; ch < 16; ch++)
        {
            if (!InstrumentSoundConfigEquals(presetSounds[static_cast<size_t>(ch)], defaultSounds[static_cast<size_t>(ch)]))
            {
                changedChannels.push_back(ch);
            }
        }

        if (changedChannels.size() == 1)
        {
            const int dstSlot = std::clamp(state.selectedSoundSlot, 0, 15);
            const int srcCh = changedChannels.front();
            MutableSoundSlot(state, dstSlot) = presetSounds[static_cast<size_t>(srcCh)];
            state.soundSlotDisplayNames[static_cast<size_t>(dstSlot)] =
                selectedItem.displayName.empty() ? selectedItem.name : selectedItem.displayName;
            if (selectedItem.recommendedRange.available
                && config::UsesDrumKitNoteSelection(MutableSoundSlot(state, dstSlot).source))
            {
                state.selectedDrumNote = selectedItem.recommendedRange.preview;
            }
        }
        else
        {
            MutableSoundSlots(state) = presetSounds;
            const std::string label = selectedItem.displayName.empty() ? selectedItem.name : selectedItem.displayName;
            for (std::string& slotName : state.soundSlotDisplayNames)
            {
                slotName = label;
            }
            const int slot = std::clamp(state.selectedSoundSlot, 0, 15);
            if (selectedItem.recommendedRange.available
                && config::UsesDrumKitNoteSelection(MutableSoundSlot(state, slot).source))
            {
                state.selectedDrumNote = selectedItem.recommendedRange.preview;
            }
        }
    }
    return true;
}

bool SavePresetDiffFromState(const GUIState& state, const std::filesystem::path& presetPath, std::string& err)
{
    GUIPresetSnapshot snapshot{};
    for (int ch = 0; ch < 16; ch++)
    {
        snapshot.soundSlots[static_cast<size_t>(ch)] = ReadSoundSlot(state, ch);
    }
    return SavePresetDiffFile(snapshot, presetPath, err);
}
} // namespace gui
