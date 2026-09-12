#include "gui/GUIPresetIO.h"

#include <algorithm>
#include <cstring>
#include <cstdint>
#include <fstream>

#include "config/ProjectJSON.h"
#include "gui/GUIActions.h"
#include "io/PlatformPaths.h"

namespace
{
using Json = nlohmann::json;

std::filesystem::path PresetPath(const std::filesystem::path& root, const std::string& key)
{
    const bool user = key.starts_with("user/");
    const auto stem = user ? key.substr(5) : key;
    if (stem.empty() || stem == "." || stem == ".." || stem.find_first_of("/\\:") != std::string::npos)
        throw std::runtime_error("音色の識別名が不正です。");
    return root / "config" / (user ? "user_presets" : "presets") / Utf8ToPath(stem + ".json");
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
    // Stable content identity; formatting changes do not invalidate a trial.
    uint64_t revision = 14695981039346656037ull;
    for (unsigned char byte : instrument.dump()) { revision ^= byte; revision *= 1099511628211ull; }
    item.revision = std::to_string(revision);
    item.displayName = instrument.value("displayName", item.displayName);
    item.comparisonGain = instrument.value("comparisonGain", 1.0);
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
bool LoadPresetInstrument(const std::filesystem::path& projectRoot, const GUIPresetItem& item,
    InstrumentConfig& instrument, std::string& err)
{
    try
    {
        const auto path = PresetPath(projectRoot, item.name);
        std::ifstream input(path, std::ios::binary);
        ProjectModel model = DefaultProjectModel();
        model.instruments.reset(); model.projectChannels.reset();
        if (!config::ProjectFromJSON(Json::parse(input), path.parent_path(), model, err)) return false;
        if (!model.instruments || model.instruments->empty()) { err = "音色が含まれていません。"; return false; }
        instrument = model.instruments->begin()->second;
        instrument.comparisonGain = item.comparisonGain;
        return true;
    }
    catch (const std::exception& ex) { err = ex.what(); return false; }
}

bool RenameUserPreset(const std::filesystem::path& projectRoot, const std::string& key,
    const std::string& name, std::string& err)
{
    try
    {
        if (!key.starts_with("user/")) { err = "付属音色の名前は変更できません。"; return false; }
        if (name.empty()) { err = "音色名を入力してください。"; return false; }
        const auto path = PresetPath(projectRoot, key);
        std::ifstream input(path, std::ios::binary);
        auto json = Json::parse(input); input.close();
        auto& instruments = json.at("project").at("instruments");
        if (!instruments.is_object() || instruments.empty()) { err = "音色が含まれていません。"; return false; }
        instruments.begin().value()["displayName"] = name;
        return config::WriteJSONFile(path, json, err);
    }
    catch (const std::exception& ex) { err = ex.what(); return false; }
}

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
    Json levels;
    try { std::ifstream input(root / "assets" / "ui" / "preset-levels.json"); if (input) levels = Json::parse(input); }
    catch (const std::exception&) { }
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
                if (!user && levels.contains(key)) item.comparisonGain = std::clamp(levels.at(key).value("gain", 1.0), .1, 4.0);
                if (item.internalOnly) continue;
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

} // namespace gui
