#pragma once

#include <array>
#include <atomic>
#include <deque>
#include <future>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "AppCore.h"
#include "gui/GUIMacroSliders.h"
#include "gui/GUISoundHistory.h"
#include "gui/GUIPianoRoll.h"
#include "gui/PreviewAudio.h"

// ステップシーケンサー状態 (ch10 / drumChannel 専用)
struct GUIStepSeqState
{
    static constexpr int kRows = 7;
    static constexpr int kSteps = 16;
    bool steps[kRows][kSteps]{};
    int velocity[kRows]{};
    bool viewActive = false;

    GUIStepSeqState()
    {
        for (int r = 0; r < kRows; ++r)
        {
            velocity[r] = 100;
        }
    }
};

// GUI状態のうち、保存対象またはProjectModelへ反映される値。
struct GUIPersistentState
{
    char midiPath[1024]{};
    char wavPath[1024]{};
    int targetChannel = -1;
    int sampleRate = 44100;
    int initialSeconds = 6;
    int bits = 16;
    float extraReleaseSec = 0.3f;
    MasterEffectConfig masterEffects{};
    int UIScaleIndex = 1; // 0=100%, 1=125%, 2=150%
    int UIModeTab = 0; // 0=Play, 1=Compose, 2=Export, 3=Advanced
    int UIThemeIndex = 0; // 0=Blueprint Beat Light, 1=Blueprint Beat Dark
    float logPanelHeight = 240.0f;
    int presetIndex = 0;
    bool serialSave = false;
    int selectedSoundSlot = 0;
    int selectedDrumNote = 36;
    int tonePreviewNoteNumber = 60;
    bool chordModeEnabled = false;
    int chordType = 0; // 0=Major 1=Minor 2=7th 3=Minor7th 4=Sus4
    char presetName[128]{ "custom" };
    bool presetDirty = false;
    std::vector<std::string> presetItems{};
    std::vector<std::vector<std::string>> presetItemTags{};
    std::vector<std::string> presetItemDescriptions{};
    std::vector<std::string> presetItemDisplayNames{};
    std::vector<std::string> presetItemCategories{};
    std::vector<bool> presetItemInternal{};
    std::string lastOutputPath{};
    std::string lastPresetPath{};
    std::shared_ptr<std::array<ChannelConfig, 16>> channelConfigs{};
    std::shared_ptr<std::array<ChannelMixState, 16>> channelMixStates{};
    std::array<int, 16> channelAssignments{ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    bool drumChannelSpecialHandling = true;
    bool previewLoop = false;
    bool autoTonePreviewEnabled = false;
    // Layer2 マクロスライダーの状態（チャンネル数分）
    std::array<MacroSliderState, 16> macroSliders{};
    // Layer1 タグフィルターの選択状態（タグ数分）
    std::array<bool, kMacroTagCount> macroTagFilters{};
    // Randomize 強度: 0=Subtle, 1=Medium, 2=Wild
    int macroRandomizeStrength = 1;
    bool layer1Expanded = true;
    bool layer2Expanded = true;
    gui::PianoRollState pianoRoll{};
    GUIStepSeqState stepSeq{};
};

// 画面表示中だけ意味を持つ一時状態。
struct GUITransientState
{
    bool hasUIError = false;
    bool showErrorDialog = false;
    int UIErrorAction = 0; // 0=None, 1=BrowseMIDI, 2=BrowseOutput, 3=GoSound, 4=GoMusic
    std::string UIErrorMessage{};
    double lastPeak = 0.0;
    bool hasPeak = false;
    bool soloPreviewActive = false;
    bool restorePreviewOnRunComplete = false;
    int soloPreviewChannel = 0;
    std::array<ChannelMixState, 16> soloPreviewBackup{};
    bool autoTonePreviewPending = false;
    double autoTonePreviewLastEditSec = 0.0;
    int previewRequestedStartTick = 0;
    double previewRequestedDurationSec = 0.0;
    // Randomize: 直前の MacroSliderState スナップショット（元に戻す用）
    std::array<MacroSliderState, 16> macroRandomizeSnapshot{};
    std::array<bool, 16> macroRandomizeHasSnapshot{};
    // Sound タブ Undo/Redo スタック（セッション中のみ保持）
    std::deque<SoundUndoEntry> soundUndoStack;
    std::deque<SoundUndoEntry> soundRedoStack;
    int playCategoryIndex = 0;
    bool playInspectorOpen = true;
};

// 非同期Run/Preview再生に関係する状態。
struct GUIAsyncRunState
{
    int lastRunExitCode = 0;
    bool hasRun = false;
    bool running = false;
    std::atomic<bool> stopRequested{ false };
    bool previewAudioReady = false;
    bool runIsPreview = false;
    bool autoPlayPreviewOnRunComplete = false;
    std::shared_ptr<SoundData> previewRenderedSound{};
    std::shared_ptr<SoundData> runOutputBuffer{};
    PreviewPlaybackState playback{};
    std::future<int> runFuture{};
};

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

// ログとRun観測用の状態。
struct GUILogState
{
    std::mutex logMutex{};
    std::vector<std::string> soundLogs{};
    std::vector<std::string> musicLogs{};
    std::vector<std::string> exportLogs{};
    std::vector<std::string> recentWavPaths{}; // Export 成功時に最大 5 件記録
    int runLogTab = 0;
    GUIRunObserver observer{};
};

// GUIState は互換用の集約型として残す。
// フィールド名は既存画面コード向けに維持しつつ、寿命別baseへ責務を分ける。
struct GUIState :
    GUIPersistentState,
    GUITransientState,
    GUIAsyncRunState,
    GUILogState
{
};
