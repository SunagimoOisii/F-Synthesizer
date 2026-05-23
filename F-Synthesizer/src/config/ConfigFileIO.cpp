#include "AppCore.h"

#include "ConfigFileInternal.h"
#include "project/ProjectModel.h"

bool LoadConfigFile(const std::filesystem::path& configPath, AppConfig& cfg, std::string& err)
{
    // 公開I/Fは薄いラッパーに保ち、実装詳細は config::internal へ閉じ込める。
    const auto overrideNoteTicks = cfg.overrideNoteTicks;
    const int overrideTicksPerQuarter = cfg.overrideTicksPerQuarter;

    ProjectModel model = ProjectModelFromAppConfig(cfg);
    if (!config::internal::LoadProjectModelFileInternal(configPath, model, err))
    {
        return false;
    }

    cfg = ToAppConfig(model);
    cfg.overrideNoteTicks = overrideNoteTicks;
    cfg.overrideTicksPerQuarter = overrideTicksPerQuarter;
    return true;
}

bool SaveConfigFile(const std::filesystem::path& configPath, const AppConfig& config, std::string& err)
{
    // 保存先ディレクトリ生成・JSON整形出力・失敗時エラー文言は internal 側へ委譲する。
    const ProjectModel model = ProjectModelFromAppConfig(config);
    return config::internal::SaveProjectModelFileInternal(configPath, model, err);
}

bool LoadProjectModelFile(const std::filesystem::path& configPath, ProjectModel& model, std::string& err)
{
    return config::internal::LoadProjectModelFileInternal(configPath, model, err);
}

bool SaveProjectModelFile(const std::filesystem::path& configPath, const ProjectModel& model, std::string& err)
{
    return config::internal::SaveProjectModelFileInternal(configPath, model, err);
}
