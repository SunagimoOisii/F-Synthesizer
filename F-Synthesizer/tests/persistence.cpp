#define NOMINMAX
#include <fstream>
#include <iostream>
#include <limits>
#include <stdexcept>

#include "config/ProjectJSON.h"
#include "gui/GUIActions.h"
#include "gui/GUIPresetIO.h"
#include "gui/GUIProjectFacade.h"
#include "gui/GUIStateModel.h"
#include "gui/GUIStatePersistence.h"
#include "io/PlatformPaths.h"

// Reuse the production Run entry points; isolate all user-data paths for this executable.
#define main ApplicationEntryForChecks
#define FindProjectRootPath OriginalProjectRootPath
#include "../src/SoundGenerate.cpp"
#undef FindProjectRootPath
#undef main

namespace
{
std::filesystem::path testRoot;
void Require(bool passed, const std::string& message)
{
    if (!passed) throw std::runtime_error(message);
}
std::string Bytes(const std::filesystem::path& path)
{
    std::ifstream in(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}
}

std::filesystem::path FindProjectRootPath() { return testRoot; }

#include "audio_checks.h"
#include "song_checks.h"
#include "tone_checks.h"

int main(int argc, char** argv)
{
    try
    {
        if (argc > 1 && std::string(argv[1]) == "--calibrate-presets")
        { testRoot = std::filesystem::current_path(); CalibratePresetLevels(testRoot); return 0; }
        if (argc > 1 && std::string(argv[1]) == "--check-transport")
        { testRoot = std::filesystem::current_path() / "output" / "check" / "transport"; CheckTransportDevice(); return 0; }
        CheckAudioIntegration();
        const auto projectRoot = std::filesystem::current_path();
        testRoot = projectRoot / "output" / "check" /
            ("persistence-" + std::to_string(GetCurrentProcessId()));
        std::filesystem::create_directories(testRoot / "config" / "presets");
        CheckSongIntegration();
        CheckToneWorkspace();
        std::string err;

        auto state = std::make_unique<GUIState>();
        gui::InitializeGUIState(*state, {});
        auto& instrument = state->tones[4].draft.instrument;
        instrument.displayName = "Lead \"A\" 日本語";
        instrument.category = "Lead";
        instrument.description = "line 1\nline 2";
        instrument.tags = {"fm", "lead"};
        instrument.recommendedRange = {40, 88, 67};
        instrument.macroHints = {{"brightness", "明るさ", "微調整"}};
        auto& sound = instrument.sound;
        sound.amp = 0.123456789123456;
        sound.portamentoTimeSec = 0.125;
        sound.attackLayer.enabled = true; sound.attackLayer.level = 0.23;
        sound.pluckLayer.enabled = true; sound.pluckLayer.level = 0.21;
        sound.stringLayer.enabled = true; sound.stringLayer.level = 0.22;
        sound.bodyLayer.enabled = true; sound.bodyLayer.mix = 0.24;
        sound.harmonicLayer.enabled = true; sound.harmonicLayer.harmonicLevels[3] = 0.31;
        sound.powerChordLayer.enabled = true; sound.powerChordLayer.level = 0.25;
        sound.chugLayer.enabled = true; sound.chugLayer.level = 0.26;
        sound.ampCabLayer.enabled = true; sound.ampCabLayer.drive = 0.27;
        sound.expressionMap.enabled = false;
        sound.expressionMap.velocityToString = 0.37;
        state->tones[15].draft.instrument = instrument;
        state->tones[15].draft.instrument.displayName = "Unused sound";
        state->tones[0].draft.instrument = instrument;
        state->tones[2].draft.instrument = instrument;
        state->channelMixStates[2].pan = -0.4;
        state->stepSeq.steps[3][7] = true;
        state->stepSeq.velocity[3] = 83;
        state->pianoRoll.hasProjectData = true;
        state->pianoRoll.loadedMidiPath = testRoot / "song.mid";
        state->pianoRoll.notes = {{0, 240, 64, 2, 99}};
        state->pianoRoll.ticksPerQuarter = 480;
        const auto expected = config::ProjectToJSON(gui::BuildProjectModelFromGUI(*state));
        Require(gui::SaveGUIStateFile(*state, err), "workspace save: " + err);
        auto restored = std::make_unique<GUIState>();
        gui::InitializeGUIState(*restored, {});
        Require(gui::LoadGUIStateFile(*restored, err), "workspace load: " + err);
        const auto actual = config::ProjectToJSON(gui::BuildProjectModelFromGUI(*restored));
        Require(expected == actual, "project metadata / sounds / assignments changed after restore");
        Require(restored->tones[4].draft.instrument.sound.attackLayer.level == 0.23, "attack layer was lost");
        Require(restored->tones[4].draft.instrument.sound.pluckLayer.level == 0.21, "pluck layer was lost");
        Require(restored->tones[4].draft.instrument.sound.stringLayer.level == 0.22, "string layer was lost");
        Require(restored->tones[4].draft.instrument.sound.portamentoTimeSec == 0.125, "portamento was lost");
        Require(restored->tones[4].draft.instrument.sound.expressionMap.velocityToString == 0.37,
            "disabled expression settings were lost");
        Require(restored->stepSeq.steps[3][7] && restored->stepSeq.velocity[3] == 83, "drums were lost");
        Require(restored->pianoRoll.projectNotes.size() == 1 &&
            restored->pianoRoll.projectNotes[0].channel == 2, "edited notes were lost");
        state->pianoRoll.notes.clear();
        Require(gui::SaveGUIStateFile(*state, err), err);
        Require(gui::LoadGUIStateFile(*restored, err), err);
        Require(restored->pianoRoll.hasProjectData && restored->pianoRoll.projectNotes.empty(),
            "empty edited score must remain empty");
        const auto savedWorkspace = Bytes(gui::GUIStatePath());
        state->tones[4].draft.instrument.sound.amp = std::numeric_limits<double>::quiet_NaN();
        Require(!gui::SaveGUIStateFile(*state, err), "invalid sound should fail to save");
        Require(Bytes(gui::GUIStatePath()) == savedWorkspace, "failed save damaged existing workspace");
        state->tones[4].draft.instrument = restored->tones[4].draft.instrument;

        std::filesystem::path first, second;
        Require(gui::SaveUserPresetFile(testRoot, instrument, "My Lead", first, err), err);
        const auto firstBytes = Bytes(first);
        Require(gui::SaveUserPresetFile(testRoot, instrument, "My Lead", second, err), err);
        Require(first != second && first.parent_path().filename() == "user_presets",
            "personal preset must be a new file in its own directory");
        Require(Bytes(first) == firstBytes, "existing personal preset was overwritten");
        Require(!gui::SaveUserPresetFile(testRoot, instrument, "../escape", second, err),
            "preset names must not escape the personal preset directory");
        ProjectModel personal = DefaultProjectModel();
        Require(LoadProjectModelFile(first, personal, err), err);
        const auto& savedInstrument = personal.instruments->at("sound");
        Require(savedInstrument.displayName == "My Lead" &&
            savedInstrument.category == "Lead" && savedInstrument.macroHints.size() == 1,
            "personal preset metadata was lost");
        Require(savedInstrument.sound.pluckLayer.level == 0.21, "personal preset layers were lost");

        const auto source = projectRoot / "config" / "presets" / "sound_lead_razor.json";
        const auto builtin = testRoot / "config" / "presets" / source.filename();
        std::filesystem::copy_file(source, builtin, std::filesystem::copy_options::overwrite_existing);
        const auto builtinBytes = Bytes(builtin);
        gui::RefreshPresetItems(*state, "sound_lead_razor");
        gui::InitializeToneWorkspace(*state);
        gui::SelectToneChannel(*state, 4);
        Require(gui::SelectTonePreset(*state, state->presetIndex, err), err);
        auto edited = state->tones[4].draft.instrument.sound;
        edited.amp *= .75;
        gui::ApplyDetailedToneEdit(*state, edited); gui::FinishToneEdit(*state);
        const auto adjusted = state->tones[4].draft;
        Require(gui::SaveUserPresetFile(testRoot, adjusted.instrument, "Edited Razor", first, err), err);
        Require(Bytes(builtin) == builtinBytes, "factory preset was overwritten");
        gui::RefreshPresetItems(*state, "user/" + PathToUtf8(first.stem()));
        Require(gui::SelectTonePreset(*state, state->presetIndex, err), err);
        gui::UndoToneEdit(*state);
        Require(state->tones[4].draft.instrument.sound == adjusted.instrument.sound &&
            state->tones[4].draft.instrument.displayName == adjusted.instrument.displayName,
            "undo must restore edited sound and its name");
        gui::UndoToneEdit(*state, true);
        Require(state->tones[4].draft.instrument.displayName == "Edited Razor", "redo must restore selected personal copy");

        const std::string personalKey = "user/" + PathToUtf8(first.stem());
        const auto beforeRename = nlohmann::json::parse(Bytes(first));
        const auto revision = state->presetItems[state->presetIndex].revision;
        Require(gui::RenameUserPreset(testRoot, personalKey, "Renamed 日本語", err), err);
        auto expectedRename = beforeRename;
        expectedRename["project"]["instruments"].begin().value()["displayName"] = "Renamed 日本語";
        Require(nlohmann::json::parse(Bytes(first)) == expectedRename, "rename changed the sound or other metadata");
        gui::RefreshPresetItems(*state, personalKey);
        Require(state->presetItems[state->presetIndex].revision != revision, "rename did not refresh preset identity");
        InstrumentConfig loaded;
        Require(gui::LoadPresetInstrument(testRoot, state->presetItems[state->presetIndex], loaded, err) &&
            loaded.displayName == "Renamed 日本語" && loaded.sound == adjusted.instrument.sound,
            "preset repository did not return the renamed sound");
        Require(!gui::RenameUserPreset(testRoot, "sound_lead_razor", "Changed", err) && Bytes(builtin) == builtinBytes,
            "rename must protect factory presets");
        Require(!gui::RenameUserPreset(testRoot, "user/../presets/sound_lead_razor", "Changed", err) && Bytes(builtin) == builtinBytes,
            "personal preset key escaped its directory");

        const auto beforeFailure = config::ProjectToJSON(gui::BuildProjectModelFromGUI(*state));
        { std::ofstream out(gui::GUIStatePath()); out << "{"; }
        Require(!gui::LoadGUIStateFile(*state, err), "broken workspace should fail to load");
        Require(config::ProjectToJSON(gui::BuildProjectModelFromGUI(*state)) == beforeFailure,
            "failed load changed the current project");

        int checked = 0;
        for (const auto& entry : std::filesystem::directory_iterator(projectRoot / "config" / "presets"))
        {
            if (entry.path().extension() != ".json") continue;
            ProjectModel original = DefaultProjectModel();
            Require(LoadProjectModelFile(entry.path(), original, err), entry.path().string() + ": " + err);
            const auto json = config::ProjectToJSON(original);
            ProjectModel reloaded = DefaultProjectModel();
            Require(config::ProjectFromJSON(json, testRoot, reloaded, err), err);
            Require(config::ProjectToJSON(reloaded) == json, entry.path().string() + ": round trip changed");
            // Only the factory instrument; ProjectModel also contains default slots.
            const auto factory = reloaded.instruments->find("sound");
            if (factory != reloaded.instruments->end())
                CheckPercussionCoverage(factory->second.sound, entry.path().filename().string());
            checked++;
        }
        std::cout << "Persistence checks: OK (" << checked
            << " preset round trips, workspace, notes, personal copies, factory protection, undo, failed IO)\n";
        return 0;
    }
    catch (const std::exception& ex)
    {
        std::cerr << "Persistence check failed: " << ex.what() << '\n';
        return 1;
    }
}
