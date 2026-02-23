#include "ConfigFileInternal.h"

#include <string>

#include "load/Internal.h"

namespace config::internal
{
bool LoadConfigFileInternal(const std::filesystem::path& configPath, AppConfig& cfg, std::string& err)
{
    const std::string text = ReadTextFile(configPath);
    if (text.empty())
    {
        err = "failed to read config file";
        return false;
    }

    // 相対パスは設定ファイル配置ディレクトリ基準で解決する。
    const std::filesystem::path baseDir = configPath.has_parent_path()
        ? configPath.parent_path()
        : std::filesystem::current_path();
    return load::LoadConfigFromText(text, baseDir, cfg, err);
}
} // namespace config::internal
