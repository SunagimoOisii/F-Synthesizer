#pragma once
#include <thread>
#include <objbase.h>
#include "gui/GUIPlatform.h"

inline void CheckToneWorkspace()
{
    InstrumentSoundConfig steady{};
    WaveformConfig pure{}; pure.wave = WaveType::Sine; pure.filterMode = FilterMode::Bypass;
    steady.source = pure; steady.sustainLevel = 1;
    Require(!gui::ToneControlSupported(steady, 0) && !gui::ToneControlSupported(steady, 4),
        "pure held waveform exposes inactive brightness/decay controls");
    Require(gui::ToneControlSupported(steady, 2) && gui::ToneControlSupported(steady, 3),
        "pure waveform lost its attack/release controls");
    steady.sustainLevel = .7;
    Require(gui::ToneControlSupported(steady, 4), "decaying waveform lost its decay control");
    auto state = std::make_unique<GUIState>();
    gui::InitializeGUIState(*state, {});
    std::string error;
    // Make two real preset fixtures and protect their bytes throughout editing.
    auto presetA = state->tones[0].draft.instrument; presetA.displayName = "tone A"; presetA.category = "Lead";
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
    Require(state->tones[0].draft.instrument.sound == adjusted, "A/B/A lost adjustments");
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
    // Updating a preset must not be hidden by its previous trial cache. Keep
    // edits against both contents, including across workspace save/recovery.
    std::filesystem::path revisionPath;
    Require(gui::SaveUserPresetFile(testRoot, presetA, "revision fixture", revisionPath, error), error);
    gui::RefreshPresetItems(*state, {}); gui::SelectToneChannel(*state, 0);
    Require(gui::SelectTonePreset(*state, indexOf("revision fixture"), error), error);
    gui::BeginToneEdit(*state); state->tones[0].draft.values[2] = .45f;
    gui::UpdateToneControls(*state); gui::FinishToneEdit(*state);
    const auto oldVersion = state->tones[0].draft;
    const auto oldJson = nlohmann::json::parse(Bytes(revisionPath));
    auto updated = oldJson;
    updated["project"]["instruments"]["sound"]["sound"]["amp"] = presetA.sound.amp * .43;
    Require(config::WriteJSONFile(revisionPath, updated, error), error);
    gui::RefreshPresetItems(*state, {});
    Require(gui::SelectTonePreset(*state, indexOf("revision fixture"), error), error);
    const auto newVersion = state->tones[0].draft;
    Require(newVersion.instrument.sound.amp == presetA.sound.amp * .43 &&
        newVersion.presetRevision != oldVersion.presetRevision, "updated preset was hidden by a cached trial");
    Require(state->tones[0].cache.at(gui::ToneCacheKey(oldVersion)).instrument.sound == oldVersion.instrument.sound,
        "preset update discarded old adjustments");
    Require(gui::SaveGUIStateFile(*state, error), error);
    auto revisions = std::make_unique<GUIState>(); gui::InitializeGUIState(*revisions, {});
    Require(gui::LoadGUIStateFile(*revisions, error), error);
    Require(revisions->tones[0].cache.contains(gui::ToneCacheKey(oldVersion)) &&
        revisions->tones[0].cache.contains(gui::ToneCacheKey(newVersion)), "recovery merged distinct preset revisions");
    Require(config::WriteJSONFile(revisionPath, oldJson, error), error);
    gui::RefreshPresetItems(*state, {});
    Require(gui::SelectTonePreset(*state, indexOf("revision fixture"), error), error);
    Require(state->tones[0].draft.instrument.sound == oldVersion.instrument.sound, "returning to old preset lost its edits");
    gui::SelectToneChannel(*state, 1);
    const auto otherChannel = gui::AudibleInstrument(*state, 0);
    const auto beforeDetail = state->tones[1].draft;
    auto detailSound = beforeDetail.instrument.sound;
    detailSound.amp *= .63;
    gui::ApplyDetailedToneEdit(*state, detailSound); gui::FinishToneEdit(*state);
    gui::PublishLiveRenderSettings(*state);
    Require(state->tones[1].draft.instrument.sound == detailSound &&
        state->liveSettings->load()->sounds[1] == RenderSound(state->tones[1].draft.instrument),
        "detailed edit did not reach playback");
    Require(gui::AudibleInstrument(*state, 0).sound == otherChannel.sound, "detailed edit changed another channel");
    gui::UndoToneEdit(*state);
    Require(state->tones[1].draft.instrument.sound == beforeDetail.instrument.sound &&
        state->tones[1].draft.values == beforeDetail.values, "detailed undo lost the previous tone or macros");
    gui::UndoToneEdit(*state, true);
    Require(state->tones[1].draft.instrument.sound == detailSound, "detailed redo failed");
    Require(gui::SaveGUIStateFile(*state, error) && gui::LoadGUIStateFile(*restored, error), error);
    Require(restored->tones[1].draft.instrument.sound == detailSound, "detailed edit was lost on restart");
    auto sharedProject = gui::BuildProjectModelFromGUI(*state);
    auto assignments = std::make_shared<std::array<ProjectChannelAssignment, 16>>(*sharedProject.projectChannels);
    (*assignments)[1].instrumentId = (*assignments)[0].instrumentId;
    sharedProject.projectChannels = assignments;
    gui::ApplyProjectModelToGUI(*restored, sharedProject);
    Require(restored->tones[0].draft.instrument.sound == restored->tones[1].draft.instrument.sound,
        "shared project instrument ID did not resolve to both channels");
    restored->tones[1].draft.instrument.sound.amp *= .5;
    Require(restored->tones[0].draft.instrument.sound.amp == state->tones[0].draft.instrument.sound.amp,
        "imported shared instrument was not detached for channel editing");
    std::cout << "Preset updates: fresh content, separate trials, revision recovery OK\n";
    std::cout << "Tone workspace: A/B/A, per-channel drafts, compare, independent mix, undo/redo, recovery, save guard OK\n";
}

inline bool fileDialogSeen = false;
inline bool cancelFileDialog = false;

inline void CALLBACK FinishTestFileDialog(HWND window, UINT, UINT_PTR timer, DWORD)
{
    KillTimer(window, timer);
    PostMessageW(window, WM_COMMAND, cancelFileDialog ? IDCANCEL : IDOK, 0);
}
inline LRESULT CALLBACK TestFileDialogHook(int code, WPARAM wp, LPARAM lp)
{
    if (code == HCBT_ACTIVATE)
    {
        const auto window = reinterpret_cast<HWND>(wp);
        wchar_t name[32]{}; GetClassNameW(window, name, 32);
        if (wcscmp(name, L"#32770") == 0)
        {
            fileDialogSeen = true;
            ShowWindow(window, SW_HIDE);
            SetTimer(window, 1, 250, FinishTestFileDialog);
        }
    }
    return CallNextHookEx(nullptr, code, wp, lp);
}

inline void CheckFileDialogAfterAudio(const std::filesystem::path& path, bool save, bool cancel)
{
    // Exercise the real Shell dialog: the regression hangs inside its folder
    // initialization after a render thread exits. A PCM-only test misses it.
    std::jthread watchdog([](std::stop_token stop) {
        for (int i = 0; i < 200 && !stop.stop_requested(); ++i) Sleep(100);
        if (!stop.stop_requested())
        {
            std::cerr << "File dialog did not return after audio playback\n" << std::flush;
            ExitProcess(1);
        }
    });
    fileDialogSeen = false; cancelFileDialog = cancel;
    const auto hook = SetWindowsHookExW(WH_CBT, TestFileDialogHook, nullptr, GetCurrentThreadId());
    Require(hook != nullptr, "file dialog test hook failed");
    std::string selected = "unchanged";
    const bool accepted = save
        ? BrowseSavePath(PathToUtf8(path), L"F-Synthesizer Song (*.fsynth)\0*.fsynth\0", L"fsynth", selected)
        : BrowseOpenPath(PathToUtf8(path), L"MIDI (*.mid;*.midi)\0*.mid;*.midi\0", selected);
    UnhookWindowsHookEx(hook);
    watchdog.request_stop();
    Require(fileDialogSeen && accepted == !cancel, "file dialog did not open/accept/cancel");
    Require(cancel ? selected == "unchanged" : Utf8ToPath(selected) == path, "file dialog changed selected path");
    APTTYPE type{}; APTTYPEQUALIFIER qualifier{};
    Require(SUCCEEDED(CoGetApartmentType(&type, &qualifier)) && (type == APTTYPE_STA || type == APTTYPE_MAINSTA),
        "audio or file dialog unbalanced the GUI's COM apartment");
}

inline void CheckTransportDevice()
{
    struct COMScope
    {
        HRESULT result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        ~COMScope() { if (SUCCEEDED(result)) CoUninitialize(); }
    } com;
    Require(SUCCEEDED(com.result), "transport test requires the GUI's STA apartment");
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
        Require(pumpUntil([&] { return state->transportAction == gui::TransportAction::None && state->playback.playing.load() && state->songCursorTick >= 1200; }), "seek failed");
        gui::PauseSongPlayback(*state);
        Require(pumpUntil([&] { return !state->running && !state->playback.playing.load(); }), "pause failed");
        const auto replacement = testRoot / L"replacement 日本語.mid";
        std::filesystem::copy_file(midi, replacement, std::filesystem::copy_options::overwrite_existing);
        CheckFileDialogAfterAudio(replacement, false, true);
        CheckFileDialogAfterAudio(replacement, false, false);
        Require(gui::LoadPianoRollMIDI(state->pianoRoll, replacement), "MIDI replacement after audio failed");
        strncpy_s(state->midiPath, PathToUtf8(replacement).c_str(), _TRUNCATE);
        const auto songPath = testRoot / L"dialog-only 日本語.fsynth";
        CheckFileDialogAfterAudio(songPath, true, true);
        CheckFileDialogAfterAudio(songPath, true, false);
        Require(!std::filesystem::exists(songPath), "file picker unexpectedly wrote a song");
        // Recreate the device, audition another note, then open again. Both
        // preview entry points and device teardown must keep the same owner.
        state->sampleRate = 48000;
        gui::RequestToneAudition(*state);
        Require(pumpUntil([&] { return state->toneAuditionActive && state->playback.playing.load(); }), "48000 Hz audition failed");
        Require(pumpUntil([&] { return !state->toneAuditionActive && !state->running && !state->playback.playing.load(); }), "audition did not finish");
        CheckFileDialogAfterAudio(replacement, false, false);
        ShutdownPreviewAudio(state->playback);
    }
    catch (...)
    {
        gui::PauseSongPlayback(*state);
        if (state->runFuture.valid()) state->runFuture.wait();
        ShutdownPreviewAudio(state->playback); throw;
    }
    std::cout << "Audio device: loop seek, pause -> one note -> resume, preserved position/range, live waveform OK\n";
    std::cout << "File dialogs after audio: MIDI replace, open/save cancellation, Japanese paths, 48000 Hz recreation OK\n";
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
