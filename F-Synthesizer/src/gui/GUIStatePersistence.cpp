#include "gui/GUIStatePersistence.h"

#include <algorithm>
#include <fstream>

#include "config/ProjectJSON.h"
#include "gui/GUIProjectFacade.h"
#include "io/PlatformPaths.h"
#include "midi/MIDIReader.h"


namespace gui
{
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PianoRollNote,
    startTick, endTick, note, channel, velocity)
}

namespace
{
using Json = nlohmann::json;

// A session owns its starting sounds. Preset files can change or disappear
// without changing recovered trials. Write each shared starting sound once.
class ToneWriter
{
public:
    Json Write(const gui::ToneSnapshot& tone)
    {
        if (!tone.base) throw std::runtime_error("保存する音色の出発点がありません。");
        const auto [entry, inserted] = ids_.try_emplace(tone.base.get(), std::to_string(ids_.size()));
        if (inserted) bases_->emplace(entry->second, *tone.base);
        return {{"base", entry->second}, {"key", tone.key}, {"presetRevision", tone.presetRevision},
            {"values", tone.values}, {"customizedBase", tone.customizedBase}};
    }

    Json Bases() const
    {
        auto model = DefaultProjectModel();
        model.instruments = bases_;
        model.projectChannels.reset();
        return config::ProjectToJSON(model);
    }
private:
    std::map<const InstrumentConfig*, std::string> ids_;
    std::shared_ptr<std::map<std::string, InstrumentConfig>> bases_ =
        std::make_shared<std::map<std::string, InstrumentConfig>>();
};

class ToneReader
{
public:
    explicit ToneReader(const Json& workspace)
    {
        if (workspace.contains("toneBases"))
        {
            const auto model = ReadModel(workspace.at("toneBases"));
            for (const auto& [id, base] : *model.instruments)
                bases_.emplace(id, std::make_shared<const InstrumentConfig>(base));
        }
    }

    gui::ToneSnapshot Read(const Json& json)
    {
        gui::ToneSnapshot tone;
        if (json.contains("base"))
        {
            const auto base = bases_.find(json.at("base").get<std::string>());
            if (base == bases_.end()) throw std::runtime_error("試聴音色の出発点が見つかりません。");
            tone.base = base->second;
        }
        else
        {
            // Recover the user's existing workspace before the compact format.
            // Identical embedded sounds are parsed only once and then shared.
            const auto& original = json.at("project").at("instruments").at("tone");
            auto base = embeddedBases_.find(original);
            if (base == embeddedBases_.end())
            {
                const auto model = ReadModel(json);
                base = embeddedBases_.emplace(original,
                    std::make_shared<const InstrumentConfig>(model.instruments->at("tone"))).first;
            }
            tone.base = base->second;
        }
        tone.key = json.at("key").get<std::string>();
        tone.presetRevision = json.value("presetRevision", std::string{});
        tone.customizedBase = json.value("customizedBase", false);
        tone.values = json.at("values").get<std::array<float, 6>>();
        for (auto& value : tone.values)
        {
            if (!std::isfinite(value)) throw std::runtime_error("invalid tone control");
            value = std::clamp(value, -1.f, 1.f);
        }
        return gui::RememberTone(gui::RestoreTone(std::move(tone)));
    }
private:
    static ProjectModel ReadModel(const Json& json)
    {
        auto model = DefaultProjectModel();
        model.instruments.reset(); model.projectChannels.reset();
        std::string error;
        if (!config::ProjectFromJSON(json, {}, model, error) || !model.instruments)
            throw std::runtime_error("保存した試聴音色を読み込めません。" + error);
        return model;
    }
    std::map<std::string, std::shared_ptr<const InstrumentConfig>> bases_;
    std::map<Json, std::shared_ptr<const InstrumentConfig>> embeddedBases_;
};

#define WORKSPACE_FIELDS(X) \
    X(activeProjectPath) X(songMidiName) \
    X(serialSave) X(selectedDrumNote) X(tonePreviewNoteNumber) \
    X(drumChannelSpecialHandling) X(previewLoop) \
    X(auditionLengthSec) X(toneExtraOpen) X(toneNotesOpen) X(scopeWholeMix)

#define PIANO_VIEW_FIELDS(X) \
    X(displayChannel) X(snapIndex) X(pixelsPerQuarter) X(tickOffset) X(noteOffset) \
    X(visibleNoteCount) X(drumNameMode) X(followPreviewPlayback) X(previewStartTick) \
    X(previewRangeEnabled) X(previewRangeStartTick) X(previewRangeEndTick)

enum class SaveContent { Workspace, Song };

Json WorkspaceToJSON(const GUIState& state, SaveContent content = SaveContent::Workspace)
{
    Json root = config::ProjectToJSON(gui::BuildProjectModelFromGUI(state));
    Json ui = Json::object();
#define SAVE_FIELD(name) ui[#name] = state.name;
    WORKSPACE_FIELDS(SAVE_FIELD)
#undef SAVE_FIELD
    ui["presetName"] = state.presetName;
    ui["userPresetName"] = state.userPresetName;
    ui["lastPresetPath"] = state.lastPresetPath;
    ui["stepSeq"] = Json::array();
    for (int row = 0; row < GUIStepSeqState::kRows; row++)
    {
        std::array<bool, GUIStepSeqState::kSteps> steps{};
        std::copy_n(state.stepSeq.steps[row], GUIStepSeqState::kSteps, steps.begin());
        ui["stepSeq"].push_back(Json{{"steps", steps}, {"velocity", state.stepSeq.velocity[row]}});
    }
    ui["stepSeqViewActive"] = state.stepSeq.viewActive;
    ui["stepSeqStartTick"] = state.stepSeq.startTick;
    if (state.toneWorkspaceReady && content == SaveContent::Workspace)
    {
        ToneWriter writer;
        ui["tones"] = Json::array();
        for (const auto& part : state.tones)
        {
            Json cached = Json::array();
            for (const auto& [key, tone] : part.cache) cached.push_back(writer.Write(tone));
            ui["tones"].push_back({{"adopted", writer.Write(part.adopted)}, {"draft", writer.Write(part.draft)},
                {"cache", std::move(cached)}, {"category", part.category}, {"auditionNote", part.auditionNote}});
        }
        ui["toneBases"] = writer.Bases();
    }
    root["workspace"] = std::move(ui);

    const auto& piano = state.pianoRoll;
    Json roll = Json::object();
#define SAVE_PIANO(name) roll[#name] = piano.name;
    PIANO_VIEW_FIELDS(SAVE_PIANO)
#undef SAVE_PIANO
    const bool loaded = !piano.loadedMidiPath.empty();
    roll["midiPath"] = PathToUtf8(loaded ? piano.loadedMidiPath : piano.projectMidiPath);
    roll["ticksPerQuarter"] = loaded ? piano.ticksPerQuarter : piano.projectTicksPerQuarter;
    roll["hasProjectData"] = loaded || piano.hasProjectData;
    roll["notes"] = loaded ? piano.notes : piano.projectNotes;
    root["pianoRoll"] = std::move(roll);
    return root;
}

void ApplyWorkspaceJSON(GUIState& state, const Json& root)
{
    const Json empty = Json::object();
    const Json& ui = root.contains("workspace") ? root.at("workspace") : empty;
#define LOAD_FIELD(name) if (ui.contains(#name)) ui.at(#name).get_to(state.name);
    WORKSPACE_FIELDS(LOAD_FIELD)
#undef LOAD_FIELD
    const std::string presetName = ui.value("presetName", std::string(state.presetName));
    strncpy_s(state.presetName, sizeof(state.presetName), presetName.c_str(), _TRUNCATE);
    const std::string userName = ui.value("userPresetName", std::string("My Sound"));
    strncpy_s(state.userPresetName, sizeof(state.userPresetName), userName.c_str(), _TRUNCATE);
    state.lastPresetPath = ui.value("lastPresetPath", std::string{});
    if (ui.contains("stepSeq"))
    {
        const auto& rows = ui.at("stepSeq");
        if (!rows.is_array() || rows.size() != GUIStepSeqState::kRows)
            throw std::runtime_error("invalid step sequencer rows");
        for (int row = 0; row < GUIStepSeqState::kRows; row++)
        {
            const auto steps = rows.at(row).at("steps").get<std::array<bool, GUIStepSeqState::kSteps>>();
            std::copy(steps.begin(), steps.end(), state.stepSeq.steps[row]);
            state.stepSeq.velocity[row] = std::clamp(rows.at(row).value("velocity", 100), 1, 127);
        }
    }
    state.stepSeq.viewActive = ui.value("stepSeqViewActive", false);
    state.stepSeq.startTick = std::max(0, ui.value("stepSeqStartTick", 0));
    state.auditionLengthSec = std::clamp(state.auditionLengthSec, .2f, 3.f);
    if (ui.contains("tones"))
    {
        ToneReader reader(ui);
        if (!ui.at("tones").is_array() || ui.at("tones").size() != 16) throw std::runtime_error("invalid part count");
        for (int ch = 0; ch < 16; ++ch)
        {
            auto& part = state.tones[ch];
            const auto& json = ui.at("tones").at(ch);
            part.adopted = gui::RestoreTone(reader.Read(json.at("adopted")));
            part.draft = gui::RestoreTone(reader.Read(json.at("draft")));
            part.category = json.value("category", std::string("Keys"));
            part.auditionNote = std::clamp(json.value("auditionNote", -1), -1, 127);
            for (const auto& cached : json.at("cache"))
            {
                auto tone = reader.Read(cached);
                const auto cacheKey = gui::ToneCacheKey(tone);
                part.cache.emplace(cacheKey, std::move(tone));
            }
        }
        state.toneWorkspaceReady = true;
    }
    auto& piano = state.pianoRoll;
    const Json& roll = root.contains("pianoRoll") ? root.at("pianoRoll") : empty;
#define LOAD_PIANO(name) if (roll.contains(#name)) roll.at(#name).get_to(piano.name);
    PIANO_VIEW_FIELDS(LOAD_PIANO)
#undef LOAD_PIANO
    piano.projectMidiPath = Utf8ToPath(roll.value("midiPath", std::string{}));
    piano.projectTicksPerQuarter = (std::max)(1, roll.value("ticksPerQuarter", 480));
    piano.hasProjectData = roll.value("hasProjectData", false);
    piano.projectNotes = roll.value("notes", std::vector<gui::PianoRollNote>{});
    for (const auto& note : piano.projectNotes)
    {
        if (note.startTick < 0 || note.endTick <= note.startTick ||
            note.note < 0 || note.note > 127 || note.channel < 0 || note.channel > 15 ||
            note.velocity < 1 || note.velocity > 127)
            throw std::runtime_error("invalid saved MIDI note");
    }
}
#undef WORKSPACE_FIELDS
#undef PIANO_VIEW_FIELDS
} // namespace

namespace gui
{
std::filesystem::path GUIStatePath()
{
    return FindProjectRootPath() / "config" / "workspace.json";
}

bool LoadGUIStateFile(GUIState& state, std::string& err)
{
    try
    {
        const auto path = GUIStatePath();
        if (!std::filesystem::exists(path)) return true;
        std::ifstream in(path, std::ios::binary);
        if (!in) throw std::runtime_error("failed to open workspace");
        const Json root = Json::parse(in);
        ProjectModel project = DefaultProjectModel();
        if (!config::ProjectFromJSON(root, path.parent_path(), project, err)) return false;
        auto candidate = std::make_unique<GUIState>();
        ApplyProjectModelToGUI(*candidate, project);
        ApplyWorkspaceJSON(*candidate, root);
        static_cast<GUIPersistentState&>(state) =
            std::move(static_cast<GUIPersistentState&>(*candidate));
        strncpy_s(state.userPresetName, sizeof(state.userPresetName),
            candidate->userPresetName, _TRUNCATE);
        return true;
    }
    catch (const std::exception& ex)
    {
        err = ex.what();
        return false;
    }
}

bool SaveGUIStateFile(const GUIState& state, std::string& err)
{
    try
    {
        return config::WriteJSONFile(GUIStatePath(), WorkspaceToJSON(state), err);
    }
    catch (const std::exception& ex)
    {
        err = ex.what();
        return false;
    }
}
bool SaveSongProjectFile(GUIState& state, const std::filesystem::path& path, std::string& err)
{
    try
    {
        if (path.extension() != ".fsynth") throw std::runtime_error("曲ファイルの拡張子は .fsynth です。");
        if (PendingToneCount(state)) throw std::runtime_error("音色の変更を採用してから保存してください。");
        // Exploration belongs to recovery; do not construct it for a named song.
        Json root = WorkspaceToJSON(state, SaveContent::Song);
        const auto midiPath = Utf8ToPath(state.midiPath);
        if (!midiPath.empty())
        {
            std::ifstream midi(midiPath, std::ios::binary);
            if (!midi) throw std::runtime_error("保存する元の MIDI を読み込めません。");
            std::vector<uint8_t> bytes{std::istreambuf_iterator<char>(midi), std::istreambuf_iterator<char>()};
            if (!ParseSMFFile(midiPath, -1).ok) throw std::runtime_error("保存する MIDI が不正です。");
            root["midi"] = {{"name", state.songMidiName.empty() ? PathToUtf8(midiPath.filename()) : state.songMidiName}, {"bytes", bytes}};
            if (!state.pianoRoll.loadedMidiPath.empty() && state.pianoRoll.loadedMidiPath != midiPath)
            {
                root["pianoRoll"]["notes"] = Json::array();
                root["pianoRoll"]["hasProjectData"] = false;
            }
        }
        root["songVersion"] = 1;
        root["workspace"]["activeProjectPath"] = "";
        if (!config::WriteJSONFile(path, root, err)) return false;
        state.activeProjectPath = PathToUtf8(std::filesystem::absolute(path));
        state.presetDirty = false;
        state.skipWorkspaceAutosave = false;
        return true;
    }
    catch (const std::exception& ex) { err = ex.what(); return false; }
}

bool LoadSongProjectFile(GUIState& state, const std::filesystem::path& path, std::string& err)
{
    try
    {
        if (state.running) throw std::runtime_error("再生・書き出しを停止してから曲を開いてください。");
        std::ifstream input(path, std::ios::binary);
        if (!input) throw std::runtime_error("曲ファイルを開けません。");
        Json root = Json::parse(input);
        if (root.value("songVersion", 0) != 1) throw std::runtime_error("対応していない曲ファイルです。");
        ProjectModel project = DefaultProjectModel();
        if (!config::ProjectFromJSON(root, path.parent_path(), project, err)) return false;
        auto candidate = std::make_unique<GUIState>();
        ApplyProjectModelToGUI(*candidate, project);
        ApplyWorkspaceJSON(*candidate, root);
        if (root.contains("midi"))
        {
            const auto bytes = root.at("midi").at("bytes").get<std::vector<uint8_t>>();
            if (bytes.size() < 14) throw std::runtime_error("曲に含まれる MIDI が不正です。");
            uint64_t hash = 14695981039346656037ull;
            for (uint8_t value : bytes) { hash ^= value; hash *= 1099511628211ull; }
            const auto directory = FindProjectRootPath() / "config" / "song_cache";
            std::filesystem::create_directories(directory);
            const auto cached = directory / (std::to_string(hash) + ".mid");
            std::ofstream midi(cached, std::ios::binary | std::ios::trunc);
            midi.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
            midi.close();
            if (!midi || !ParseSMFFile(cached, -1).ok) throw std::runtime_error("曲の MIDI を復元できません。");
            const std::string cachedPath = PathToUtf8(cached);
            strncpy_s(candidate->midiPath, sizeof(candidate->midiPath), cachedPath.c_str(), _TRUNCATE);
            candidate->pianoRoll.projectMidiPath = cached;
            candidate->songMidiName = root.at("midi").value("name", std::string("song.mid"));
        }
        if (candidate->midiPath[0] != '\0' && !LoadPianoRollMIDI(candidate->pianoRoll, Utf8ToPath(candidate->midiPath)))
            throw std::runtime_error(candidate->pianoRoll.lastError);
        candidate->activeProjectPath = PathToUtf8(std::filesystem::absolute(path));
        StopPreviewAudio(state.playback);
        static_cast<GUIPersistentState&>(state) = std::move(static_cast<GUIPersistentState&>(*candidate));
        strncpy_s(state.userPresetName, sizeof(state.userPresetName), candidate->userPresetName, _TRUNCATE);
        state.presetDirty = false; state.skipWorkspaceAutosave = false;
        state.playEditingChannel = state.pianoRoll.displayChannel;
        return true;
    }
    catch (const std::exception& ex) { err = ex.what(); return false; }
}
} // namespace gui
