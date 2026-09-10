#pragma once
#include <thread>

inline void CheckToneWorkspace()
{
    auto state = std::make_unique<GUIState>();
    gui::InitializeGUIState(*state, {});
    std::string error;
    // Make two real preset fixtures and protect their bytes throughout editing.
    auto presetA = state->instruments[0]; presetA.displayName = "tone A"; presetA.category = "Lead";
    auto presetB = presetA; presetB.displayName = "tone B"; presetB.sound.amp *= .6;
    std::filesystem::path pathA, pathB;
    Require(gui::SaveUserPresetFile(testRoot, presetA, "tone A", pathA, error), error);
    Require(gui::SaveUserPresetFile(testRoot, presetB, "tone B", pathB, error), error);
    const auto originalA = Bytes(pathA), originalB = Bytes(pathB);
    gui::RefreshPresetItems(*state, {});
    auto indexOf = [&](const char* name) {
        for (int i = 0; i < static_cast<int>(state->presetItems.size()); ++i) if (state->presetItems[i].displayName == name) return i;
        return -1;
    };
    gui::InitializeToneWorkspace(*state);
    gui::SelectToneChannel(*state, 0);
    Require(gui::SelectTonePreset(*state, indexOf("tone A"), error), error);
    gui::BeginToneEdit(*state); state->tones[0].draft.values[2] = .7f; gui::UpdateToneControls(*state); gui::FinishToneEdit(*state);
    const auto adjusted = state->tones[0].draft.instrument.sound;
    Require(adjusted.attackSec == presetA.sound.attackSec && adjusted.releaseSec > presetA.sound.releaseSec,
        "release changed attack or did not change release");
    gui::UndoToneEdit(*state); Require(state->tones[0].draft.values[2] == 0, "tone undo failed");
    gui::UndoToneEdit(*state, true); Require(state->tones[0].draft.values[2] == .7f, "tone redo failed");
    Require(gui::SelectTonePreset(*state, indexOf("tone B"), error), error);
    Require(gui::SelectTonePreset(*state, indexOf("tone A"), error), error);
    Require(state->instruments[0].sound == adjusted, "A/B/A lost adjustments");
    gui::SelectToneChannel(*state, 1); Require(gui::SelectTonePreset(*state, indexOf("tone B"), error), error);
    Require(gui::PendingToneCount(*state) == 2, "per-channel drafts lost");
    gui::PublishLiveRenderSettings(*state);
    Require(state->liveSettings->load()->sounds[0] == RenderSound(state->tones[0].draft.instrument), "other channel draft stopped sounding");
    state->tones[0].compare = true; gui::PublishLiveRenderSettings(*state);
    Require(state->liveSettings->load()->sounds[0] == RenderSound(state->tones[0].adopted.instrument), "compare did not restore adopted sound");
    state->channelMixStates[1].level = .42; state->channelMixStates[1].pan = -.3;
    gui::CancelTone(*state, 1);
    Require(state->channelMixStates[1].level == .42 && state->channelMixStates[1].pan == -.3, "cancel damaged mix");
    state->tones[0].compare = false;
    Require(gui::SaveGUIStateFile(*state, error), error);
    auto restored = std::make_unique<GUIState>(); gui::InitializeGUIState(*restored, {});
    Require(gui::LoadGUIStateFile(*restored, error), "draft recovery: " + error);
    Require(gui::TonePending(*restored, 0) && restored->tones[0].draft.instrument.sound == adjusted, "pending recovery failed");
    Require(restored->tones[0].cache.size() >= 3 && restored->tones[1].cache.size() >= 2, "preset cache recovery failed");
    Require(!gui::SaveSongProjectFile(*state, testRoot / "pending.fsynth", error), "named save accepted pending drafts");
    gui::AdoptAllTones(*state); Require(gui::PendingToneCount(*state) == 0, "adopt all failed");
    state->pianoRoll.ticksPerQuarter = 480; state->pianoRoll.maxTick = 3840;
    state->pianoRoll.timeSignatures = {{0, 3, 4}, {2880, 6, 8}};
    Require(gui::SongBarTicks(*state) == std::vector<int>({0, 1440, 2880, 4320}), "3/4 and 6/8 bar boundaries failed");
    Require(Bytes(pathA) == originalA && Bytes(pathB) == originalB, "editing overwrote original presets");
    std::cout << "Tone workspace: A/B/A, per-channel drafts, compare, independent mix, undo/redo, recovery, save guard OK\n";
}

inline void CheckTransportDevice()
{
    std::filesystem::create_directories(testRoot);
    const auto midi = testRoot / "transport.mid";
    const unsigned char bytes[] = {'M','T','h','d',0,0,0,6,0,0,0,1,1,0xe0,
        'M','T','r','k',0,0,0,13,0,0x90,69,100,0xad,0,0x80,69,0,0,0xff,0x2f,0};
    { std::ofstream output(midi, std::ios::binary); output.write(reinterpret_cast<const char*>(bytes), sizeof(bytes)); }
    auto state = std::make_unique<GUIState>(); gui::InitializeGUIState(*state, {});
    strncpy_s(state->midiPath, PathToUtf8(midi).c_str(), _TRUNCATE);
    Require(gui::LoadPianoRollMIDI(state->pianoRoll, midi), "transport MIDI fixture failed");
    gui::InitializeToneWorkspace(*state); gui::SelectToneChannel(*state, 0);
    auto pumpUntil = [&](const auto& predicate) {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(8);
        do {
            gui::TryFinalizeCompletedRun(*state); gui::UpdateGUITransport(*state);
            if (predicate()) return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } while (std::chrono::steady_clock::now() < deadline);
        return false;
    };
    try
    {
        state->pianoRoll.previewRangeStartTick = 480; state->pianoRoll.previewRangeEndTick = 1920;
        state->pianoRoll.previewRangeEnabled = state->previewLoop = true;
        state->songCursorTick = 720; state->auditionLengthSec = .2f;
        gui::RequestSongPlayback(*state);
        Require(pumpUntil([&] { return state->playback.playing.load() && state->songCursorTick > 730; }), "song did not start");
        gui::RequestToneAudition(*state); const int pausedAt = state->resumeSongTick;
        Require(pumpUntil([&] { return state->toneAuditionActive && state->playback.playing.load(); }), "single note did not start");
        Require(state->songCursorTick == pausedAt, "song moved during single note");
        Require(pumpUntil([&] { return !state->toneAuditionActive && state->playback.playing.load(); }), "song did not resume");
        Require(std::abs(state->songCursorTick - pausedAt) < 70, "song resumed from wrong position");
        Require(state->previewLoop && state->pianoRoll.previewRangeStartTick == 480 && state->pianoRoll.previewRangeEndTick == 1920,
            "single note damaged loop range");
        Require(state->audioScope->cursor.load() > 1000, "audio scope received no render samples");
        gui::SeekSong(*state, 1200);
        Require(pumpUntil([&] { return state->transportAction == 0 && state->playback.playing.load() && state->songCursorTick >= 1200; }), "seek failed");
        gui::PauseSongPlayback(*state);
        Require(pumpUntil([&] { return !state->running && !state->playback.playing.load(); }), "pause failed");
        ShutdownPreviewAudio(state->playback);
    }
    catch (...)
    {
        gui::PauseSongPlayback(*state);
        if (state->runFuture.valid()) state->runFuture.wait();
        ShutdownPreviewAudio(state->playback); throw;
    }
    std::cout << "Audio device: loop seek, pause -> one note -> resume, preserved position/range, live waveform OK\n";
}

inline void CalibratePresetLevels(const std::filesystem::path& root)
{
    struct Quiet : IRunObserver { void OnLogLine(const std::string&) override {} } quiet;
    nlohmann::json levels = nlohmann::json::object();
    for (const auto& entry : std::filesystem::directory_iterator(root / "config" / "presets"))
    {
        if (entry.path().extension() != ".json") continue;
        ProjectModel model = DefaultProjectModel(); std::string error;
        model.instruments.reset(); model.projectChannels.reset();
        Require(LoadProjectModelFile(entry.path(), model, error), error);
        Require(model.instruments && model.instruments->size() == 1, "calibration must load exactly one preset");
        const auto instrument = model.instruments->begin()->second;
        if (instrument.internal) continue;
        const bool drums = std::holds_alternative<DrumKitConfig>(instrument.sound.source);
        auto instruments = std::make_shared<std::map<std::string, InstrumentConfig>>(); (*instruments)["tone"] = instrument;
        (*instruments)["tone"].comparisonGain = 1;
        auto channels = std::make_shared<std::array<ProjectChannelAssignment, 16>>(); (*channels)[0].enabled = true; (*channels)[0].instrumentId = "tone";
        model.instruments = instruments; model.projectChannels = channels;
        model.midiPath.clear(); model.masterEffects = {}; model.targetChannel = 0; model.sampleRate = 44100; model.initialSeconds = 1; model.extraReleaseSec = .2;
        auto notes = std::make_shared<std::vector<MIDIEventTick>>();
        const int pitches[] = {instrument.recommendedRange.preview, std::clamp(instrument.recommendedRange.preview + 7, 0, 127), instrument.recommendedRange.preview};
        const int drumPitches[] = {36, 38, 42};
        for (int i = 0; i < 3; ++i)
        {
            MIDIEventTick on{}, off{}; on.type = off.type = MIDIEventType::Note;
            on.tick = i * 720; off.tick = on.tick + (drums ? 100 : 576);
            on.noteNumber = off.noteNumber = drums ? drumPitches[i] : pitches[i];
            on.velocity = 100; on.isNoteOn = true; on.noteInstanceID = off.noteInstanceID = i + 1;
            on.order = i * 2; off.order = i * 2 + 1; notes->push_back(on); notes->push_back(off);
        }
        RenderRuntimeOverrides overrides; overrides.noteTicks = notes; overrides.ticksPerQuarter = 480;
        auto options = DefaultPreviewRenderOptions(); options.writeWAV = false; options.durationSec = 2.25;
        SoundData sound; Require(Run(model, options, overrides, &quiet, &sound) == 0, "level calibration failed");
        double energy = 0, peak = 0;
        for (int i = 0; i < sound.length; ++i)
        {
            const double l = sound.SampleL(i), r = sound.SampleR(i); energy += (l * l + r * r) * .5;
            peak = std::max({peak, std::abs(l), std::abs(r)});
        }
        const double rms = std::sqrt(energy / std::max(1, sound.length));
        Require(rms > .00001, "silent preset calibration");
        const double gain = std::clamp(std::min(.02 / rms, .6 / std::max(.001, peak)), .1, 4.0);
        levels[PathToUtf8(entry.path().stem())] = {{"gain", gain}, {"rms", rms}, {"peak", peak}};
    }
    std::string error;
    Require(config::WriteJSONFile(root / "assets" / "ui" / "preset-levels.json", levels, error), error);
    std::cout << "Calibrated " << levels.size() << " presets (velocity 100, RMS target 0.02, peak cap 0.6)\n";
}
