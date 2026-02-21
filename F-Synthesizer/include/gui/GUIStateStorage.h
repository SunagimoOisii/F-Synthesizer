#pragma once

#include <array>
#include <filesystem>
#include <string>

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
    std::array<ChannelMixState, 16> channelMixStates{};
};

bool LoadGUIStateStorageFile(const std::filesystem::path& path, GUIStateStorageData& data, std::string& err);
bool SaveGUIStateStorageFile(const std::filesystem::path& path, const GUIStateStorageData& data, std::string& err);
