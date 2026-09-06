#include "ConfigFileInternal.h"
#include "project/ProjectModel.h"
#include "config/ProjectJSON.h"
#include "load/Internal.h"

bool config::ProjectFromJSON(const nlohmann::json& json, const std::filesystem::path& baseDir,
    ProjectModel& project, std::string& err)
{
    ProjectModel candidate = project;
    if (!internal::load::LoadConfigFromText(json.dump(), baseDir, candidate, err)) return false;
    project = std::move(candidate);
    return true;
}

bool LoadProjectModelFile(const std::filesystem::path& configPath, ProjectModel& model, std::string& err)
{
    return config::internal::LoadProjectModelFileInternal(configPath, model, err);
}

bool SaveProjectModelFile(const std::filesystem::path& configPath, const ProjectModel& model, std::string& err)
{
    return config::internal::SaveProjectModelFileInternal(configPath, model, err);
}
