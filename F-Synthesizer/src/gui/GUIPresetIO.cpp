#include "gui/GUIPresetIO.h"

#include <algorithm>
#include <fstream>

#include "config/SourceJSON.h"
#include "gui/GUIConfigUtils.h"

namespace gui
{
namespace
{
const char* WaveToPresetString(WaveType wave)
{
    switch (wave)
    {
    case WaveType::Sine: return "sine";
    case WaveType::Square: return "square";
    case WaveType::Saw: return "saw";
    case WaveType::Triangle: return "triangle";
    }
    return "saw";
}
} // namespace

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
    out << "  \"defaultWave\": \"" << WaveToPresetString(WaveFromIndex(snapshot.defaultWave)) << "\",\n";
    out << "  \"channels\": {\n";

    // 既定値との差分のみを書き出し、presetファイルの保守コストを抑える。
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
    // 既定の操作の流れを分かりやすくするため、basic_wave を先頭に寄せる。
    const auto it = std::find(names.begin(), names.end(), "basic_wave");
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

    // base -> preset の順で後から読んだ値を優先し、CLI経路と同じ合成規則にする。
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
} // namespace gui
