#include "ConfigFileInternal.h"

#include <string>

#include "load/Internal.h"

namespace config::internal
{
bool LoadProjectModelFileInternal(const std::filesystem::path& configPath, ProjectModel& model, std::string& err)
{
    // 目的: 設定ファイル文字列を読み、LoadConfigFromText へ橋渡しする。
    // 前提: configPath は読み取り可能なJSONファイルを指す。
    // 副作用: model を更新し、失敗時は err へ診断文字列を書き込む。
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
    if (!load::LoadConfigFromText(text, baseDir, model, err))
    {
        return false;
    }

    return true;
}
} // namespace config::internal
