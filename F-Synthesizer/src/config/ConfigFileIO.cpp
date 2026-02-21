#include "AppCore.h"

#include "ConfigFileInternal.h"

bool LoadConfigFile(const std::filesystem::path& configPath, AppConfig& cfg, std::string& err)
{
    return config::internal::LoadConfigFileInternal(configPath, cfg, err);
}

bool SaveConfigFile(const std::filesystem::path& configPath, const AppConfig& config, std::string& err)
{
    return config::internal::SaveConfigFileInternal(configPath, config, err);
}
