#pragma once
#include "midi/MIDIPipeline.h"
#include "gui/GUIMacroMapping.h"

inline void CheckSongIntegration()
{
    const auto midiPath = testRoot / L"元の曲.mid";
    std::vector<uint8_t> bytes{'M','T','h','d',0,0,0,6,0,1,0,2,1,0xe0};
    auto track = [&](std::vector<uint8_t> data)
    {
        const auto size = static_cast<uint32_t>(data.size());
        bytes.insert(bytes.end(), {'M','T','r','k',static_cast<uint8_t>(size >> 24),static_cast<uint8_t>(size >> 16),static_cast<uint8_t>(size >> 8),static_cast<uint8_t>(size)});
        bytes.insert(bytes.end(), data.begin(), data.end());
    };
    track({0,0xff,0x51,3,7,0xa1,0x20,0x83,0x60,0xff,0x51,3,0x0f,0x42,0x40,0x83,0x60,0xff,0x2f,0});
    track({0,0xc0,80,0,0xb0,64,127,0,0xe0,0,64,0,0x90,69,100,
        0x81,0x70,69,110,0x81,0x70,0x80,69,0,0x81,0x70,69,0,0x81,0x70,0xb0,64,0,0,0xff,0x2f,0});
    { std::ofstream file(midiPath, std::ios::binary); file.write(reinterpret_cast<const char*>(bytes.data()), bytes.size()); }
    const auto raw = ParseSMFFile(midiPath, -1);
    Require(raw.ok && raw.stats.numTracks == 2 && raw.tempoEvents.size() == 2, "midifile multi-track import failed");
    std::vector<MIDIEventTick> notes;
    for (const auto& event : raw.rawEvents) if (event.type == MIDIEventType::Note) notes.push_back(event);
    Require(notes.size() == 4 && notes[0].noteInstanceID == notes[2].noteInstanceID && notes[1].noteInstanceID == notes[3].noteInstanceID,
        "overlapping notes / running status were not paired FIFO");
    std::vector<MIDIEvent> samples;
    BuildSampleEvents(notes, raw.tempoEvents, raw.ticksPerQuarter, 44100, samples);
    Require(samples[1].sample == 11025 && samples[2].sample == 22050 && samples[3].sample == 44100, "tempo-map conversion failed");
    auto noTempoNotes = notes;
    noTempoNotes[0].tick = 0; noTempoNotes[1].tick = 24;
    noTempoNotes[2].tick = 48; noTempoNotes[3].tick = 96;
    BuildSampleEvents(noTempoNotes, {}, 96, 22050, samples);
    Require(samples[1].sample == 2756 && samples[2].sample == 5512 && samples[3].sample == 11025,
        "no-tempo MIDI tick interpolation failed");
    MIDIBuildOutput windowed;
    std::string windowError;
    Require(BuildMIDIPipeline(midiPath, -1, 44100, 0.6, 0.4, nullptr, 0, windowed, windowError), "range preview failed");
    const auto sustained = std::count_if(windowed.events.begin(), windowed.events.end(), [](const auto& event) {
        return event.sample == 0 && event.type == MIDIEventType::Note && event.isNoteOn;
    });
    Require(sustained == 2, "range preview dropped held/sustained notes");
    std::vector<MIDIEventTick> empty;
    MIDIBuildOutput emptied;
    std::string error;
    Require(!BuildMIDIPipeline(midiPath, -1, 44100, 0, -1, &empty, 480, emptied, error) && error == "no note events found",
        "deleting all notes must not restore notes from the MIDI file");

    auto state = std::make_unique<GUIState>(); gui::InitializeGUIState(*state, {});
    Require(gui::SaveSongProjectFile(*state, testRoot / "no-midi.fsynth", error) &&
        gui::LoadSongProjectFile(*state, testRoot / "no-midi.fsynth", error) && state->midiPath[0] == '\0',
        "sound-only song could not be reopened");
    const std::string midiUtf8 = PathToUtf8(midiPath);
    strncpy_s(state->midiPath, sizeof(state->midiPath), midiUtf8.c_str(), _TRUNCATE);
    Require(gui::LoadPianoRollMIDI(state->pianoRoll, midiPath), "piano roll import failed");
    state->pianoRoll.notes[0].note = 60;
    state->instruments[2].displayName = "自分のベース";
    state->channelAssignments[0] = 2; state->channelMixStates[0].pan = -0.25;
    const auto song = testRoot / L"曲_日本語.fsynth";
    Require(gui::SaveSongProjectFile(*state, song, error), "named song save: " + error);
    const auto originalSongBytes = Bytes(song);
    std::filesystem::remove(midiPath); // Only the fixture created above.
    auto restored = std::make_unique<GUIState>(); gui::InitializeGUIState(*restored, {});
    Require(gui::LoadSongProjectFile(*restored, song, error), "portable song load: " + error);
    Require(restored->pianoRoll.notes.size() == 2 && restored->pianoRoll.notes[0].note == 60, "song lost edited notes");
    Require(restored->pianoRoll.tempoEvents.size() == 2 && ParseSMFFile(Utf8ToPath(restored->midiPath), -1).ok, "song lost its embedded MIDI / tempo");
    Require(restored->channelAssignments[0] == 2 && restored->instruments[2].displayName == "自分のベース" && restored->channelMixStates[0].pan == -0.25,
        "song lost sounds or channel mix");
    Require(gui::SaveGUIStateFile(*restored, error), error);
    auto resumed = std::make_unique<GUIState>(); gui::InitializeGUIState(*resumed, {});
    Require(gui::LoadGUIStateFile(*resumed, error) && resumed->pianoRoll.loadedMidiPath.empty(), "workspace restart failed");
    const std::string wav = PathToUtf8(testRoot / L"再開した曲.wav");
    strncpy_s(resumed->wavPath, sizeof(resumed->wavPath), wav.c_str(), _TRUNCATE);
    resumed->initialSeconds = 1;
    resumed->UIModeTab = 2;
    gui::StartGUIRun(*resumed, false);
    if (!resumed->runFuture.valid()) for (const auto& log : resumed->exportLogs) std::cerr << log << '\n';
    Require(resumed->runFuture.valid(), "export before opening piano roll failed to start");
    resumed->runFuture.wait();
    Require(gui::TryFinalizeCompletedRun(*resumed) && resumed->lastRunExitCode == 0 &&
        resumed->pianoRoll.notes[0].note == 60 && std::filesystem::file_size(Utf8ToPath(wav)) > 44,
        "export after restart did not apply saved note edits");
    restored->pianoRoll.notes.clear();
    Require(gui::SaveSongProjectFile(*restored, song, error), error);
    Require(gui::LoadSongProjectFile(*state, song, error) && state->pianoRoll.notes.empty(), "saved empty score was not restored");
    const auto validBytes = Bytes(song);
    state->instruments[0].sound.amp = std::numeric_limits<double>::quiet_NaN();
    Require(!gui::SaveSongProjectFile(*state, song, error) && Bytes(song) == validBytes, "failed save damaged named song");
    state->instruments[0].sound.amp = 0.2;
    const auto before = config::ProjectToJSON(gui::BuildProjectModelFromGUI(*state));
    { std::ofstream broken(testRoot / "broken.fsynth"); broken << "{}"; }
    Require(!gui::LoadSongProjectFile(*state, testRoot / "broken.fsynth", error) && config::ProjectToJSON(gui::BuildProjectModelFromGUI(*state)) == before,
        "failed song load changed the current work");
    auto sound = state->instruments[0].sound;
    auto previous = ReadMacroSliders(sound, {}); auto edited = previous; edited.brightness = 0.9f;
    const double attack = sound.attackSec, release = sound.releaseSec;
    ApplyMacroSliders(sound, edited, &previous);
    Require(sound.attackSec == attack && sound.releaseSec == release, "brightness changed unrelated envelope controls");
    struct LoopSink : IPreviewStreamSink
    {
        std::shared_ptr<LiveRenderMailbox> mailbox;
        int length = 0, frames = 0, begins = 0;
        double energy[2]{};
        bool completed = false;
        bool Begin(int, int, int total, bool loop) override { length = total; ++begins; return loop; }
        bool WriteFrame(double, double) override { return false; }
        bool WriteFrames(const double* data, int count) override
        {
            const int lap = frames / length;
            for (int i = 0; i < count; ++i) energy[(std::min)(lap, 1)] += data[i * 2] * data[i * 2];
            frames += count;
            if (frames == length)
            {
                auto next = std::make_shared<LiveRenderSettings>(*mailbox->load());
                next->sounds[0].amp *= 0.1; mailbox->store(next);
            }
            return frames < length * 2;
        }
        void Complete(bool canceled) override { completed = canceled; }
    } sink;
    ProjectModel loopProject = DefaultProjectModel();
    loopProject.midiPath.clear();
    auto live = std::make_shared<LiveRenderSettings>();
    live->sounds[0] = sound;
    sink.mailbox = std::make_shared<LiveRenderMailbox>(); sink.mailbox->store(live);
    RenderRuntimeOverrides overrides;
    overrides.noteTicks = std::make_shared<const std::vector<MIDIEventTick>>(notes);
    overrides.ticksPerQuarter = 480; overrides.liveSettings = sink.mailbox;
    RenderOptions options = DefaultPreviewRenderOptions(); options.durationSec = 0.25;
    Require(RunPreviewStreaming(loopProject, options, overrides, nullptr, sink, true) == 2 && sink.completed && sink.begins == 1,
        "loop stream did not stop cleanly");
    Require(sink.length == 11025 && sink.energy[0] > 0 && sink.energy[1] < sink.energy[0] * 0.03,
        "loop duration or sound updates on the second lap failed");
    std::cout << "Loop duration / second-lap live edits OK\n";
    std::cout << "Song / MIDI checks: portable file, edited and empty notes, save failure, tempo changes, FIFO notes, macro isolation OK\n";
}
