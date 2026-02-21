#pragma once

#include <filesystem>
#include <functional>
#include <cstdint>
#include <string>
#include <vector>

#include "MIDIParser.h"
#include "gui/PreviewAudio.h"

namespace gui
{
struct PianoRollNote
{
    int startTick = 0;
    int endTick = 0;
    int note = 60;
    int channel = 0;
    int velocity = 0;
};

struct PianoRollEditCommand
{
    std::vector<PianoRollNote> before{};
    std::vector<PianoRollNote> after{};
};

struct PianoRollVisibleCacheKey
{
    int displayChannel = 0;
    int noteLow = 0;
    int noteHigh = 127;
    int startTick = 0;
    int endTick = 0;
};

// ピアノロール表示/編集のUI状態。
// Phase4では選択/編集に加えて Undo/Redo と永続化用の中間状態も保持する。
struct PianoRollState
{
    int displayChannel = 0;
    int snapIndex = 3; // 0=OFF, 1=1/4, 2=1/8, 3=1/16, 4=1/32
    int lastSnapIndex = 3;
    float pixelsPerQuarter = 72.0f;
    int tickOffset = 0;
    int noteOffset = 36;
    int visibleNoteCount = 48;
    bool drumNameMode = false;
    bool followPreviewPlayback = true;
    int previewStartTick = 0;

    std::filesystem::path loadedMidiPath{};
    std::filesystem::file_time_type loadedWriteTime{};
    int ticksPerQuarter = 480;
    int maxTick = 0;
    std::vector<TempoEvent> tempoEvents{};
    std::vector<PianoRollNote> notes{};
    std::vector<uint8_t> selected{};
    int primarySelectedIndex = -1;

    bool isRangeSelecting = false;
    float rangeStartX = 0.0f;
    float rangeStartY = 0.0f;
    float rangeEndX = 0.0f;
    float rangeEndY = 0.0f;

    bool isDraggingMove = false;
    bool isDraggingResize = false;
    bool isCreatingNote = false;
    int dragTargetIndex = -1;
    int dragStartMouseTick = 0;
    int dragStartMouseNote = 60;
    int createStartTick = 0;
    int createCurrentTick = 0;
    int createNote = 60;
    std::vector<PianoRollNote> dragSnapshot{};

    std::vector<int> pendingSelectedIndices{};
    std::vector<PianoRollEditCommand> undoStack{};
    std::vector<PianoRollEditCommand> redoStack{};
    int maxUndoCommands = 64;
    uint64_t notesVersion = 1;
    uint64_t cacheNotesVersion = 0;
    bool visibleNoteIndexCacheValid = false;
    PianoRollVisibleCacheKey visibleCacheKey{};
    std::vector<int> visibleNoteIndexCache{};

    std::filesystem::path projectMidiPath{};
    int projectTicksPerQuarter = 0;
    std::vector<PianoRollNote> projectNotes{};
    bool hasProjectData = false;

    bool hasLoadError = false;
    std::string lastError{};
};

void DrawPianoRollPanel(
    PianoRollState& state,
    const char* midiPathUtf8,
    const PreviewPlaybackState* playback,
    const std::function<void(const std::string&)>& appendLog,
    const std::function<void()>& requestPreviewPlay,
    const std::function<void()>& requestPreviewStop);
} // namespace gui
