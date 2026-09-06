#pragma once

#include "project/ProjectModel.h"

namespace gui
{
// Always creates a new file in config/user_presets; existing sounds are never overwritten.
bool SaveUserPresetFile(const std::filesystem::path& projectRoot, const InstrumentConfig& sound,
    const std::string& name, std::filesystem::path& savedPath, std::string& err);
} // namespace gui
