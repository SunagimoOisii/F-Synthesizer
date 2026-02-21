#pragma once

#include <filesystem>
#include <functional>
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

// ピアノロール表示のUI状態と表示用ノートキャッシュ。
// 編集機能は次Phaseで拡張し、Phase1では表示と操作基盤に限定する。
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

    bool hasLoadError = false;
    std::string lastError{};
};

void DrawPianoRollPanel(
    PianoRollState& state,
    const char* midiPathUtf8,
    const std::function<void(const std::string&)>& appendLog);
} // namespace gui
