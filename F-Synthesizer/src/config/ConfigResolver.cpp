#include "config/ConfigResolver.h"

#include <filesystem>
#include <string>

#include "io/PlatformPaths.h"

bool ResolveRuntimeConfig(const CLIOptions& options, ResolvedRuntimeConfig& outResolved, std::string& err)
{
    outResolved = ResolvedRuntimeConfig{};
    outResolved.config = DefaultConfig();

    const std::filesystem::path projectRoot = FindProjectRootPath();
    // 優先順:
    // 1) --config 明示
    // 2) --preset 未指定時の default.json
    // 3) --preset 指定時の base + preset 合成
    // 4) 何も無ければ base + retro_heavy_fm_lead_brasswall を自動適用
    if (!options.configPath.empty())
    {
        outResolved.selectedConfigPath = options.configPath;
    }
    else if (options.presetName.empty())
    {
        const std::filesystem::path autoConfigPath = projectRoot / "config" / "default.json";
        if (std::filesystem::exists(autoConfigPath))
        {
            outResolved.selectedConfigPath = autoConfigPath;
        }
    }

    if (!outResolved.selectedConfigPath.empty())
    {
        std::string loadErr;
        if (!LoadConfigFile(outResolved.selectedConfigPath, outResolved.config, loadErr))
        {
            err = "Failed to load config: " + PathToUtf8(outResolved.selectedConfigPath) + " (" + loadErr + ")";
            return false;
        }
        outResolved.infoLines.push_back("Config Path: " + PathToUtf8(outResolved.selectedConfigPath));
        return true;
    }

    if (!options.presetName.empty())
    {
        // presetは base.json を先に適用し、preset側で後勝ち上書きする。
        const std::filesystem::path basePath = projectRoot / "config" / "base.json";
        const std::filesystem::path presetPath = projectRoot / "config" / "presets" / (options.presetName + ".json");
        if (!std::filesystem::exists(basePath))
        {
            err = "Base config not found: " + PathToUtf8(basePath);
            return false;
        }
        if (!std::filesystem::exists(presetPath))
        {
            err = "Preset config not found: " + PathToUtf8(presetPath);
            return false;
        }

        std::string loadErr;
        if (!LoadConfigFile(basePath, outResolved.config, loadErr))
        {
            err = "Failed to load base config: " + PathToUtf8(basePath) + " (" + loadErr + ")";
            return false;
        }
        if (!LoadConfigFile(presetPath, outResolved.config, loadErr))
        {
            err = "Failed to load preset config: " + PathToUtf8(presetPath) + " (" + loadErr + ")";
            return false;
        }

        outResolved.infoLines.push_back("Preset: " + options.presetName);
        outResolved.infoLines.push_back("Base Config Path: " + PathToUtf8(basePath));
        outResolved.infoLines.push_back("Preset Config Path: " + PathToUtf8(presetPath));
        return true;
    }

    const std::filesystem::path basePath = projectRoot / "config" / "base.json";
    const std::filesystem::path fallbackPresetPath = projectRoot / "config" / "presets" / "retro_heavy_fm_lead_brasswall.json";
    if (std::filesystem::exists(basePath) && std::filesystem::exists(fallbackPresetPath))
    {
        std::string loadErr;
        if (!LoadConfigFile(basePath, outResolved.config, loadErr))
        {
            err = "Failed to load base config: " + PathToUtf8(basePath) + " (" + loadErr + ")";
            return false;
        }
        if (!LoadConfigFile(fallbackPresetPath, outResolved.config, loadErr))
        {
            err = "Failed to load preset config: " + PathToUtf8(fallbackPresetPath) + " (" + loadErr + ")";
            return false;
        }
        outResolved.infoLines.push_back("Preset: retro_heavy_fm_lead_brasswall (auto)");
        outResolved.infoLines.push_back("Base Config Path: " + PathToUtf8(basePath));
        outResolved.infoLines.push_back("Preset Config Path: " + PathToUtf8(fallbackPresetPath));
    }

    // base/preset が無い環境でも、DefaultConfig だけで起動可能にして初期セットアップを妨げない。
    return true;
}
