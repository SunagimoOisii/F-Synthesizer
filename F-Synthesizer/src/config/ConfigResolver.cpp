#include "config/ConfigResolver.h"

#include <filesystem>
#include <string>

bool ResolveRuntimeConfig(const CliOptions& options, ResolvedRuntimeConfig& outResolved, std::string& err)
{
    outResolved = ResolvedRuntimeConfig{};
    outResolved.config = DefaultConfig();

    const std::filesystem::path projectRoot = FindProjectRootPath();
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
            err = "Failed to load config: " + outResolved.selectedConfigPath.string() + " (" + loadErr + ")";
            return false;
        }
        outResolved.infoLines.push_back("Config Path: " + outResolved.selectedConfigPath.string());
        return true;
    }

    if (!options.presetName.empty())
    {
        const std::filesystem::path basePath = projectRoot / "config" / "base.json";
        const std::filesystem::path presetPath = projectRoot / "config" / "presets" / (options.presetName + ".json");
        if (!std::filesystem::exists(basePath))
        {
            err = "Base config not found: " + basePath.string();
            return false;
        }
        if (!std::filesystem::exists(presetPath))
        {
            err = "Preset config not found: " + presetPath.string();
            return false;
        }

        std::string loadErr;
        if (!LoadConfigFile(basePath, outResolved.config, loadErr))
        {
            err = "Failed to load base config: " + basePath.string() + " (" + loadErr + ")";
            return false;
        }
        if (!LoadConfigFile(presetPath, outResolved.config, loadErr))
        {
            err = "Failed to load preset config: " + presetPath.string() + " (" + loadErr + ")";
            return false;
        }

        outResolved.infoLines.push_back("Preset: " + options.presetName);
        outResolved.infoLines.push_back("Base Config Path: " + basePath.string());
        outResolved.infoLines.push_back("Preset Config Path: " + presetPath.string());
        return true;
    }

    const std::filesystem::path basePath = projectRoot / "config" / "base.json";
    const std::filesystem::path fallbackPresetPath = projectRoot / "config" / "presets" / "basic_wave.json";
    if (std::filesystem::exists(basePath) && std::filesystem::exists(fallbackPresetPath))
    {
        std::string loadErr;
        if (!LoadConfigFile(basePath, outResolved.config, loadErr))
        {
            err = "Failed to load base config: " + basePath.string() + " (" + loadErr + ")";
            return false;
        }
        if (!LoadConfigFile(fallbackPresetPath, outResolved.config, loadErr))
        {
            err = "Failed to load preset config: " + fallbackPresetPath.string() + " (" + loadErr + ")";
            return false;
        }
        outResolved.infoLines.push_back("Preset: basic_wave (auto)");
        outResolved.infoLines.push_back("Base Config Path: " + basePath.string());
        outResolved.infoLines.push_back("Preset Config Path: " + fallbackPresetPath.string());
    }

    return true;
}
