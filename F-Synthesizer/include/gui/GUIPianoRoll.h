#pragma once

#include <filesystem>
#include <functional>
#include <cstdint>
#include <string>
#include <vector>

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

// ピアノロール表示/編集のUI状態。
// Phase2では Note の選択・移動・長さ変更までを扱う。
struct PianoRollState
{
    int displayChannel = 0;
    int snapIndex = 3; // 0=OFF, 1=1/4, 2=1/8, 3=1/16, 4=1/32
    float pixelsPerQuarter = 72.0f;
    int tickOffset = 0;
    int noteOffset = 36;
    int visibleNoteCount = 48;
    bool drumNameMode = false;

    std::filesystem::path loadedMidiPath{};
    std::filesystem::file_time_type loadedWriteTime{};
    int ticksPerQuarter = 480;
    int maxTick = 0;
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
    int dragTargetIndex = -1;
    int dragStartMouseTick = 0;
    int dragStartMouseNote = 60;
    std::vector<PianoRollNote> dragSnapshot{};

    bool hasLoadError = false;
    std::string lastError{};
};

void DrawPianoRollPanel(
    PianoRollState& state,
    const char* midiPathUtf8,
    const std::function<void(const std::string&)>& appendLog);
} // namespace gui
