#include "gui/GUIPresetIO.h"

#include <algorithm>
#include <cstring>
#include <fstream>

#include "config/ProjectJSON.h"
#include "gui/GUIActions.h"
#include "gui/GUIMacroMapping.h"
#include "gui/GUIProjectFacade.h"
#include "io/PlatformPaths.h"

namespace
{
using Json = nlohmann::json;

std::filesystem::path PresetPath(const std::filesystem::path& root, const std::string& key)
{
    if (key.rfind("user/", 0) == 0)
        return root / "config" / "user_presets" / Utf8ToPath(key.substr(5) + ".json");
    return root / "config" / "presets" / Utf8ToPath(key + ".json");
}

GUIPresetItem ReadPresetItem(const std::filesystem::path& path, const std::string& key)
{
    std::ifstream in(path, std::ios::binary);
    const Json root = Json::parse(in);
    GUIPresetItem item{};
    item.name = key;
    item.displayName = PathToUtf8(path.stem());
    item.internalOnly = key.rfind("demo_", 0) == 0;
    const auto& project = root.at("project");
    const auto instruments = project.find("instruments");
    if (instruments == project.end() || instruments->empty()) return item;
    const auto& instrument = instruments->begin().value();
    item.displayName = instrument.value("displayName", item.displayName);
    item.category = instrument.value("category", std::string{});
    item.description = instrument.value("description", std::string{});
    item.tags = instrument.value("tags", std::vector<std::string>{});
    item.internalOnly = instrument.value("internal", item.internalOnly);
    if (instrument.contains("recommendedRange"))
    {
        const auto& range = instrument.at("recommendedRange");
        item.recommendedRange.low = range.value("low", 48);
        item.recommendedRange.high = range.value("high", 84);
        item.recommendedRange.preview = range.value("preview", 60);
        item.recommendedRange.available = true;
    }
    if (instrument.contains("macroHints"))
        for (const auto& hint : instrument.at("macroHints"))
            item.macroHints.push_back({hint.value("id", std::string{}),
                hint.value("label", std::string{}), hint.value("description", std::string{})});
    return item;
}
} // namespace

namespace gui
{
bool SaveUserPresetFile(const std::filesystem::path& projectRoot, const InstrumentConfig& sound,
    const std::string& name, std::filesystem::path& savedPath, std::string& err)
{
    try
    {
        if (name.empty() || name.find_first_of("<>:\"/\\|?*") != std::string::npos ||
            name.back() == '.' || name.back() == ' ' ||
            std::any_of(name.begin(), name.end(), [](unsigned char c) { return c < 32; }))
        {
            err = "音色名に使えない文字が含まれています。";
            return false;
        }
        InstrumentConfig instrument = sound;
        instrument.displayName = name;
        instrument.internal = false;
        ProjectModel project = DefaultProjectModel();
        project.instruments = std::make_shared<const std::map<std::string, InstrumentConfig>>(
            std::map<std::string, InstrumentConfig>{{"sound", instrument}});
        auto channels = std::make_shared<std::array<ProjectChannelAssignment, 16>>();
        (*channels)[0].enabled = true;
        (*channels)[0].instrumentId = "sound";
        project.projectChannels = channels;
        Json root = config::ProjectToJSON(project);
        // A reusable sound carries no song paths, mix or master effects.
        for (const char* key : {"midiPath", "wavPath", "targetChannel", "initialSeconds",
            "bits", "sampleRate", "extraReleaseSec", "effects"})
            root["project"].erase(key);
        root["project"]["channels"]["0"].erase("mix");
        const auto directory = projectRoot / "config" / "user_presets";
        for (int copy = 1; copy <= 10000; copy++)
        {
            const std::string suffix = copy == 1 ? "" : "_" + std::to_string(copy);
            const auto path = directory / Utf8ToPath("user_" + name + suffix + ".json");
            if (std::filesystem::exists(path)) continue;
            if (!config::WriteJSONFile(path, root, err, false)) return false;
            savedPath = path;
            return true;
        }
        err = "同名の音色が多すぎます。別の名前を指定してください。";
        return false;
    }
    catch (const std::exception& ex)
    {
        err = ex.what();
        return false;
    }
}

void RefreshPresetItems(GUIState& state, const std::string& preferName)
{
    state.presetItems.clear();
    const auto root = FindProjectRootPath();
    for (const bool user : {false, true})
    {
        const auto directory = root / "config" / (user ? "user_presets" : "presets");
        std::error_code ec;
        if (!std::filesystem::is_directory(directory, ec)) continue;
        std::filesystem::directory_iterator it(directory, ec), end;
        for (; it != end && !ec; it.increment(ec))
        {
            if (!it->is_regular_file(ec) || it->path().extension() != ".json") continue;
            try
            {
                const std::string key = (user ? "user/" : "") + PathToUtf8(it->path().stem());
                auto item = ReadPresetItem(it->path(), key);
                if (state.UIModeTab != 3 && item.internalOnly) continue;
                state.presetItems.push_back(std::move(item));
            }
            catch (const std::exception&)
            {
                // A malformed personal preset must not prevent the app from opening.
            }
        }
    }
    std::sort(state.presetItems.begin(), state.presetItems.end(),
        [](const auto& a, const auto& b) { return a.name < b.name; });
    state.presetIndex = state.presetItems.empty() ? -1 : 0;
    for (int i = 0; i < static_cast<int>(state.presetItems.size()); i++)
        if (state.presetItems[i].name == preferName) { state.presetIndex = i; break; }
}

bool ApplySelectedPresetPaths(GUIState& state, std::string& err)
{
    if (state.presetIndex < 0 || state.presetIndex >= static_cast<int>(state.presetItems.size()))
    {
        err = "音色を選択してください。";
        return false;
    }
    const GUIPresetItem item = state.presetItems[state.presetIndex];
    try
    {
        const auto path = PresetPath(FindProjectRootPath(), item.name);
        std::ifstream in(path, std::ios::binary);
        const Json root = Json::parse(in);
        ProjectModel project = DefaultProjectModel();
        project.instruments.reset();
        project.projectChannels.reset();
        if (!config::ProjectFromJSON(root, path.parent_path(), project, err)) return false;
        if (project.instruments && !project.instruments->empty())
        {
            const int slot = std::clamp(state.selectedSoundSlot, 0, 15);
            PushSoundHistoryEntry(state, slot, ReadSoundSlot(state, slot), state.macroSliders[slot]);
            // Presets are templates: copy the sound, never retain a writable link to its file.
            state.instruments[slot] = project.instruments->begin()->second;
            const auto& range = state.instruments[slot].recommendedRange;
            state.tonePreviewNoteNumber = std::clamp(range.preview, 0, 127);
            state.selectedDrumNote = state.tonePreviewNoteNumber;
            state.macroSliders[slot] = ::ReadMacroSliders(state.instruments[slot].sound, MacroSliderState{});
        }
        if (root.at("project").contains("effects")) state.masterEffects = project.masterEffects;
        strncpy_s(state.presetName, sizeof(state.presetName), item.name.c_str(), _TRUNCATE);
        state.presetDirty = true;
        return true;
    }
    catch (const std::exception& ex)
    {
        err = ex.what();
        return false;
    }
}

bool SaveUserPresetFromState(GUIState& state, std::string& err)
{
    const int slot = std::clamp(state.selectedSoundSlot, 0, 15);
    std::filesystem::path path;
    if (!SaveUserPresetFile(FindProjectRootPath(), state.instruments[slot],
        state.userPresetName, path, err)) return false;
    state.instruments[slot].displayName = state.userPresetName;
    state.instruments[slot].internal = false;
    state.lastPresetPath = PathToUtf8(path);
    const std::string key = "user/" + PathToUtf8(path.stem());
    strncpy_s(state.presetName, sizeof(state.presetName), key.c_str(), _TRUNCATE);
    RefreshPresetItems(state, key);
    state.presetDirty = true;
    return true;
}
} // namespace gui
