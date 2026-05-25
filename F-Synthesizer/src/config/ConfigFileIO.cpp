#include "ConfigFileInternal.h"
#include "project/ProjectModel.h"

bool LoadProjectModelFile(const std::filesystem::path& configPath, ProjectModel& model, std::string& err)
{
    return config::internal::LoadProjectModelFileInternal(configPath, model, err);
}

bool SaveProjectModelFile(const std::filesystem::path& configPath, const ProjectModel& model, std::string& err)
{
    return config::internal::SaveProjectModelFileInternal(configPath, model, err);
}
