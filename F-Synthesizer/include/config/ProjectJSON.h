#pragma once

#include "project/ProjectModel.h"
#include "third_party/nlohmann/json.hpp"

namespace config
{
nlohmann::json ProjectToJSON(const ProjectModel& project);
bool ProjectFromJSON(const nlohmann::json& json, const std::filesystem::path& baseDir,
    ProjectModel& project, std::string& err);
bool WriteJSONFile(const std::filesystem::path& path, const nlohmann::json& json,
    std::string& err, bool overwrite = true);
} // namespace config
