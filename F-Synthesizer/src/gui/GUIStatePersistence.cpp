#include "gui/GUIStatePersistence.h"

#include <algorithm>
#include <fstream>

#include "config/ProjectJSON.h"
#include "gui/GUIProjectFacade.h"
#include "io/PlatformPaths.h"

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(MacroSliderState,
    brightness, roughness, movement, envelope,
    lastLayer2Roughness, lastLayer2Envelope, lastLayer2Movement)

namespace gui
{
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(PianoRollNote,
    startTick, endTick, note, channel, velocity)
}

namespace
{
using Json = nlohmann::json;

#define WORKSPACE_FIELDS(X) \
    X(UIScaleIndex) X(UIModeTab) X(UIThemeIndex) X(logPanelHeight) \
    X(serialSave) X(selectedSoundSlot) X(selectedDrumNote) X(tonePreviewNoteNumber) \
    X(chordModeEnabled) X(chordType) X(drumChannelSpecialHandling) \
    X(previewLoop) X(autoTonePreviewEnabled) X(macroSliders) \
    X(macroRandomizeStrength) X(layer1Expanded) X(layer2Expanded)

#define PIANO_VIEW_FIELDS(X) \
    X(displayChannel) X(snapIndex) X(pixelsPerQuarter) X(tickOffset) X(noteOffset) \
    X(visibleNoteCount) X(drumNameMode) X(followPreviewPlayback) X(previewStartTick) \
    X(previewRangeEnabled) X(previewRangeStartTick) X(previewRangeEndTick)

Json WorkspaceToJSON(const GUIState& state)
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
    root["workspace"] = std::move(ui);

    const auto& piano = state.pianoRoll;
    Json roll = Json::object();
#define SAVE_PIANO(name) roll[#name] = piano.name;
    PIANO_VIEW_FIELDS(SAVE_PIANO)
#undef SAVE_PIANO
    const bool loaded = !piano.loadedMidiPath.empty();
    roll["midiPath"] = PathToUtf8(loaded ? piano.loadedMidiPath : piano.projectMidiPath);
    roll["ticksPerQuarter"] = loaded ? piano.ticksPerQuarter : piano.projectTicksPerQuarter;
    roll["hasProjectData"] = piano.hasProjectData;
    roll["notes"] = loaded ? piano.notes : piano.projectNotes;
    root["pianoRoll"] = std::move(roll);
    return root;
}

void ApplyWorkspaceJSON(GUIState& state, const Json& root)
{
    const Json ui = root.value("workspace", Json::object());
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
    auto& piano = state.pianoRoll;
    const Json roll = root.value("pianoRoll", Json::object());
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
} // namespace gui
