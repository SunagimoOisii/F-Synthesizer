#pragma once

#include "project/ProjectModel.h"

struct GUIPresetItem;

namespace gui
{
// Load a detached sound copy; factory comparison levels come from the listed item.
bool LoadPresetInstrument(const std::filesystem::path& projectRoot, const GUIPresetItem& item,
    InstrumentConfig& instrument, std::string& err);
// Changes only the label of an existing personal copy, never a factory preset.
bool RenameUserPreset(const std::filesystem::path& projectRoot, const std::string& key,
    const std::string& name, std::string& err);
// Always creates a new file in config/user_presets; existing sounds are never overwritten.
bool SaveUserPresetFile(const std::filesystem::path& projectRoot, const InstrumentConfig& sound,
    const std::string& name, std::filesystem::path& savedPath, std::string& err);
} // namespace gui
