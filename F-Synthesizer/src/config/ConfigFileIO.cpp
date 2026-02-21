#include "AppCore.h"

#include "ConfigFileInternal.h"

bool LoadConfigFile(const std::filesystem::path& configPath, AppConfig& cfg, std::string& err)
{
    // 公開I/Fは薄いラッパーに保ち、実装詳細は config::internal へ閉じ込める。
    return config::internal::LoadConfigFileInternal(configPath, cfg, err);
}

bool SaveConfigFile(const std::filesystem::path& configPath, const AppConfig& config, std::string& err)
{
    return config::internal::SaveConfigFileInternal(configPath, config, err);
}
