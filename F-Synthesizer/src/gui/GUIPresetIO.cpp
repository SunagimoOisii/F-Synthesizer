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
    out << "        \"velocityToDrive\": " << layer.velocityToDrive << "\n";
    out << "      },\n";
}

struct PresetMeta
{
    std::vector<std::string> tags{};
    std::string description{};
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
    if (lower == "waveform")
    {
        return config::SourceKind::Waveform;
    }
    if (lower == "analog")
    {
        return config::SourceKind::Analog;
    }
    if (lower == "fm")
    {
        return config::SourceKind::Fm;
    }
    if (lower == "psg")
    {
        return config::SourceKind::Psg;
    }
    if (lower == "drumkit")
    {
        return config::SourceKind::DrumKit;
    }
    if (lower == "noise")
    {
        return config::SourceKind::Noise;
    }
    return config::SourceKind::Count;
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

    if (root.contains("channels") && root["channels"].is_object())
    {
        for (auto it = root["channels"].begin(); it != root["channels"].end(); ++it)
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
    std::ofstream out(presetPath, std::ios::binary | std::ios::trunc);
    if (!out)
    {
        err = "failed to open preset file";
        return false;
    }

    out << "{\n";
    out << "  \"channels\": {\n";

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
        out << "    \"" << ch << "\": {\n";
        out << "      \"amp\": " << cur.amp << ",\n";
        out << "      \"attackSec\": " << cur.attackSec << ",\n";
        out << "      \"decaySec\": " << cur.decaySec << ",\n";
        out << "      \"sustainLevel\": " << cur.sustainLevel << ",\n";
        out << "      \"releaseSec\": " << cur.releaseSec << ",\n";
        if (cur.attackLayer.enabled)
        {
            WriteAttackLayerJSON(out, cur.attackLayer);
        }
        if (cur.bassLayer.enabled)
        {
            WriteBassLayerJSON(out, cur.bassLayer);
        }
        config::WriteSourceJSON(out, cur.source, 6);
        out << "\n    }";
    }

    out << "\n  }\n";
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
        for (const auto& ent : std::filesystem::directory_iterator(dir, ec))
        {
            if (ec) break;
            if (!ent.is_regular_file()) continue;
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
    if (std::filesystem::exists(basePath))
    {
        if (!LoadConfigFile(basePath, cfg, err))
        {
            err = "failed to load base config: " + err;
            return false;
        }
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
    state.presetItems.reserve(all.size());
    state.presetItemTags.reserve(all.size());
    state.presetItemDescriptions.reserve(all.size());
    for (const auto& name : all)
    {
        const auto it = metaByName.find(name);
        const PresetMeta* meta = (it != metaByName.end()) ? &it->second : nullptr;
        if (meta != nullptr && PresetMatchesSourceKind(name, kind, *meta))
        {
            state.presetItems.push_back(name);
            state.presetItemTags.push_back(meta->tags);
            state.presetItemDescriptions.push_back(meta->description);
        }
    }
    const bool allowUnfilteredFallback = (kind == config::SourceKind::Count);
    if (state.presetItems.empty() && allowUnfilteredFallback)
    {
        state.presetItems = std::move(all);
        state.presetItemTags.clear();
        state.presetItemDescriptions.clear();
        state.presetItemTags.reserve(state.presetItems.size());
        state.presetItemDescriptions.reserve(state.presetItems.size());
        for (const auto& name : state.presetItems)
        {
            const auto it = metaByName.find(name);
            if (it != metaByName.end())
            {
                state.presetItemTags.push_back(it->second.tags);
                state.presetItemDescriptions.push_back(it->second.description);
            }
            else
            {
                state.presetItemTags.emplace_back();
                state.presetItemDescriptions.emplace_back();
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
