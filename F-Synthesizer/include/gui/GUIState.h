#pragma once

#include <array>
#include <atomic>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "AppCore.h"
#include "gui/GUIPianoRoll.h"
#include "gui/PreviewAudio.h"

// GUI画面の編集値・実行状態・ログ状態を集約した永続/実行モデル。
// 1フレーム内で参照するUI状態と、非同期Run連携状態を同居させる。
struct GUIState
{
    struct GUIRunObserver : IRunObserver
    {
        std::mutex* logMutex = nullptr;
        std::vector<std::string>* logs = nullptr;
        std::atomic<bool>* cancelRequested = nullptr;

        void OnLogLine(const std::string& line) override
        {
            if (logMutex == nullptr || logs == nullptr)
            {
                return;
            }
            std::lock_guard<std::mutex> lock(*logMutex);
            logs->push_back(line);
        }

        bool ShouldCancel() override
        {
            return cancelRequested != nullptr && cancelRequested->load(std::memory_order_relaxed);
        }
    };

    char midiPath[1024]{};
    char wavPath[1024]{};
    int targetChannel = -1;
    int sampleRate = 44100;
    int initialSeconds = 6;
    int bits = 16;
    float extraReleaseSec = 0.3f;
    int defaultWave = 2; // saw
    int uiScaleIndex = 1; // 0=100%, 1=125%, 2=150%
    int uiModeTab = 0; // 0=Sound, 1=Music
    float logPanelHeight = 240.0f;
    int presetIndex = 0;
    int lastRunExitCode = 0;
    bool hasRun = false;
    bool running = false;
    std::atomic<bool> stopRequested{ false };
    bool serialSave = false;
    int selectedChannel = 0;
    int selectedDrumNote = 36;
    char presetName[128]{ "custom" };
    bool presetDirty = false;
    std::vector<std::string> presetItems{};
    std::string lastOutputPath{};
    std::string lastPresetPath{};
    std::shared_ptr<std::array<ChannelConfig, 16>> channelConfigs{};
    std::shared_ptr<std::array<ChannelMixState, 16>> channelMixStates{};
    double lastPeak = 0.0;
    bool hasPeak = false;
    bool soloPreviewActive = false;
    bool restorePreviewOnRunComplete = false;
    int soloPreviewChannel = 0;
    std::array<ChannelMixState, 16> soloPreviewBackup{};
    bool previewLoop = false;
    bool previewAudioReady = false;
    bool runIsPreview = false;
    bool autoPlayPreviewOnRunComplete = false;
    std::shared_ptr<SoundData> previewRenderedSound{};
    std::shared_ptr<SoundData> runOutputBuffer{};
    PreviewPlaybackState playback{};
    std::future<int> runFuture{};
    std::mutex logMutex{};
    std::vector<std::string> soundLogs{};
    std::vector<std::string> musicLogs{};
    int runLogTab = 0;
    GUIRunObserver observer{};
    gui::PianoRollState pianoRoll{};
};
