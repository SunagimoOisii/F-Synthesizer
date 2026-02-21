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
    int defaultWave = 2;
    int uiScaleIndex = 1;
    float logPanelHeight = 240.0f;
    int presetIndex = 0;
    bool serialSave = false;
    bool previewLoop = false;
    int selectedChannel = 0;
    int selectedDrumNote = 36;
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

bool LoadGUIStateStorageFile(const std::filesystem::path& path, GUIStateStorageData& data, std::string& err);
bool SaveGUIStateStorageFile(const std::filesystem::path& path, const GUIStateStorageData& data, std::string& err);
bool LoadPianoRollProjectStorageFile(const std::filesystem::path& path, PianoRollProjectStorageData& data, std::string& err);
bool SavePianoRollProjectStorageFile(const std::filesystem::path& path, const PianoRollProjectStorageData& data, std::string& err);
