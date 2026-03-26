#pragma once

#include <array>
#include <filesystem>
#include <string>
#include <vector>

#include "SynthEngine/SynthEngine.h"

struct GUIStateStorageData
{
    std::string midiPath;
    std::string wavPath;
    int targetChannel = -1;
    int sampleRate = 44100;
    int initialSeconds = 6;
    int bits = 16;
    float extraReleaseSec = 0.3f;
    int fxBitCrusherBits = 16;
    float fxSampleRateReducerRatio = 1.0f;
    bool fxChorusEnabled = false;
    float fxChorusMix = 0.0f;
    float fxChorusBaseDelayMs = 14.0f;
    float fxChorusDepthMs = 6.0f;
    float fxChorusRateHz = 0.35f;
    float fxChorusFeedback = 0.08f;
    bool fxFlangerEnabled = false;
    float fxFlangerMix = 0.0f;
    float fxFlangerBaseDelayMs = 1.5f;
    float fxFlangerDepthMs = 1.0f;
    float fxFlangerRateHz = 0.25f;
    float fxFlangerFeedback = 0.15f;
    bool fxDelayEnabled = false;
    float fxDelayMix = 0.0f;
    float fxDelayTimeSec = 0.25f;
    float fxDelayFeedback = 0.25f;
    bool fxDelayTempoSync = false;
    float fxDelaySyncBeats = 1.0f;
    bool fxReverbEnabled = false;
    float fxReverbMix = 0.0f;
    float fxReverbRoomSize = 0.45f;
    float fxReverbDamping = 0.30f;
    int UIScaleIndex = 1;
    int UIModeTab = 0;
    float logPanelHeight = 240.0f;
    int presetIndex = 0;
    bool serialSave = false;
    bool previewLoop = false;
    bool autoTonePreviewEnabled = false;
    int selectedSoundSlot = 0;
    int selectedDrumNote = 36;
    int tonePreviewNoteNumber = 60;
    std::string presetName = "custom";
    std::string lastPresetPath;
    int prDisplayChannel = 0;
    int prSnapIndex = 3;
    float prPixelsPerQuarter = 72.0f;
    int prTickOffset = 0;
    int prNoteOffset = 36;
    int prVisibleNoteCount = 48;
    bool prDrumNameMode = false;
    bool prFollowPreviewPlayback = true;
    int prPreviewStartTick = 0;
    std::vector<int> prSelectedIndices{};
    std::array<ChannelMixState, 16> channelMixStates{};
    std::array<int, 16> channelAssignments{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    bool drumChannelSpecialHandling = true;
};

struct PianoRollProjectStorageNote
{
    int startTick = 0;
    int endTick = 0;
    int note = 60;
    int channel = 0;
    int velocity = 100;
};

struct PianoRollProjectStorageData
{
    std::string midiPath{};
    int ticksPerQuarter = 480;
    std::vector<PianoRollProjectStorageNote> notes{};
};

// path が無い場合は初回起動として true を返し、data は呼び出し前値を維持する。
bool LoadGUIStateStorageFile(const std::filesystem::path& path, GUIStateStorageData& data, std::string& err);
bool SaveGUIStateStorageFile(const std::filesystem::path& path, const GUIStateStorageData& data, std::string& err);
// path が無い場合は true を返し、PianoRollプロジェクトは未保存として継続する。
bool LoadPianoRollProjectStorageFile(const std::filesystem::path& path, PianoRollProjectStorageData& data, std::string& err);
bool SavePianoRollProjectStorageFile(const std::filesystem::path& path, const PianoRollProjectStorageData& data, std::string& err);
