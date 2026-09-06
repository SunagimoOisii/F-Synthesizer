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

int main()
{
    try
    {
        const auto projectRoot = std::filesystem::current_path();
        testRoot = projectRoot / "output" / "check" /
            ("persistence-" + std::to_string(GetCurrentProcessId()));
        std::filesystem::create_directories(testRoot / "config" / "presets");
        std::string err;

        auto state = std::make_unique<GUIState>();
        gui::InitializeGUIState(*state, {});
        auto& instrument = state->instruments[4];
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
        state->instruments[15] = instrument;
        state->instruments[15].displayName = "Unused sound";
        state->channelAssignments[0] = 4;
        state->channelAssignments[2] = 4;
        state->channelAssignments[15] = 0;
        state->channelMixStates[2].pan = -0.4;
        state->macroSliders[4].brightness = 0.75f;
        state->macroSliders[4].lastLayer2Roughness = 0.625f;
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
        Require(restored->instruments[4].sound.attackLayer.level == 0.23, "attack layer was lost");
        Require(restored->instruments[4].sound.pluckLayer.level == 0.21, "pluck layer was lost");
        Require(restored->instruments[4].sound.stringLayer.level == 0.22, "string layer was lost");
        Require(restored->instruments[4].sound.portamentoTimeSec == 0.125, "portamento was lost");
        Require(restored->instruments[4].sound.expressionMap.velocityToString == 0.37,
            "disabled expression settings were lost");
        Require(restored->macroSliders[4].lastLayer2Roughness == 0.625f, "macro state was lost");
        Require(restored->stepSeq.steps[3][7] && restored->stepSeq.velocity[3] == 83, "drums were lost");
        Require(restored->pianoRoll.projectNotes.size() == 1 &&
            restored->pianoRoll.projectNotes[0].channel == 2, "edited notes were lost");
        state->pianoRoll.notes.clear();
        Require(gui::SaveGUIStateFile(*state, err), err);
        Require(gui::LoadGUIStateFile(*restored, err), err);
        Require(restored->pianoRoll.hasProjectData && restored->pianoRoll.projectNotes.empty(),
            "empty edited score must remain empty");
        const auto savedWorkspace = Bytes(gui::GUIStatePath());
        state->instruments[4].sound.amp = std::numeric_limits<double>::quiet_NaN();
        Require(!gui::SaveGUIStateFile(*state, err), "invalid sound should fail to save");
        Require(Bytes(gui::GUIStatePath()) == savedWorkspace, "failed save damaged existing workspace");
        state->instruments[4] = restored->instruments[4];

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

        const auto source = projectRoot / "config" / "presets" / "sound_lead_blade.json";
        const auto builtin = testRoot / "config" / "presets" / source.filename();
        std::filesystem::copy_file(source, builtin, std::filesystem::copy_options::overwrite_existing);
        const auto builtinBytes = Bytes(builtin);
        gui::RefreshPresetItems(*state, "sound_lead_blade");
        state->selectedSoundSlot = 4;
        Require(gui::ApplySelectedPresetPaths(*state, err), err);
        const auto factorySound = state->instruments[4].sound.amp;
        state->instruments[4].sound.amp *= 0.75;
        Require(gui::SaveGUIStateFile(*state, err), err);
        Require(gui::SaveUserPresetFromState(*state, err), err);
        Require(Bytes(builtin) == builtinBytes, "factory preset was overwritten");
        gui::RefreshPresetItems(*state, "sound_lead_blade");
        Require(gui::ApplySelectedPresetPaths(*state, err), err);
        Require(state->instruments[4].sound.amp == factorySound, "factory reload retained edits");
        Require(UndoSound(*state), "preset selection must be undoable");
        Require(state->instruments[4].sound.amp == factorySound * 0.75 &&
            state->instruments[4].displayName == state->userPresetName,
            "undo must restore both edited sound and personal name");
        Require(RedoSound(*state) && state->instruments[4].sound.amp == factorySound,
            "redo must restore the selected preset");

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
