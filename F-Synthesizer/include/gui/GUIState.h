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
#include "project/ProjectModel.h"
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

struct GUIPresetItem
{
    struct RecommendedRange
    {
        int low = 48;
        int high = 84;
        int preview = 60;
        bool available = false;
    };

    struct MacroHint
    {
        std::string id{};
        std::string label{};
        std::string description{};
    };

    std::string name{};
    std::vector<std::string> tags{};
    std::string description{};
    std::string displayName{};
    std::string category{};
    RecommendedRange recommendedRange{};
    std::vector<MacroHint> macroHints{};
    bool internalOnly = false;
};

// GUI state storage に保存する値、またはProjectModelへ反映される値。
// projectModel の保存形式そのものではなく、GUI の復元に必要な永続状態を表す。
struct GUIPersistentState
{
    std::string activeProjectPath{};
    std::string songMidiName{};
    char midiPath[1024]{};
    char wavPath[1024]{};
    int targetChannel = -1;
    int sampleRate = 44100;
    int initialSeconds = 6;
    int bits = 16;
    float extraReleaseSec = 0.3f;
    MasterEffectConfig masterEffects{};
    int UIScaleIndex = 1; // 0=100%, 1=125%, 2=150%
    int UIModeTab = 1; // 0=Play, 1=Compose, 2=Export, 3=Advanced
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
    std::string lastOutputPath{};
    std::string lastPresetPath{};
    std::array<InstrumentConfig, 16> instruments{};
    std::array<ChannelMixState, 16> channelMixStates{};
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

// 画面表示中だけ意味を持つ一時状態。project/config 保存対象にはしない。
struct GUITransientState
{
    std::string pendingOpenPath{};
    bool pendingOpenIsSong = false;
    uint64_t observedNotesVersion = 0;
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
    int playEditingChannel = -1;
    bool playInspectorOpen = true;
    bool presetDirty = false;
    bool skipWorkspaceAutosave = false;
    std::vector<GUIPresetItem> presetItems{};
    char userPresetName[128]{ "My Sound" };
};

// 非同期Run/Preview再生に関係する状態。project/config 保存対象にはしない。
struct GUIAsyncRunState
{
    std::shared_ptr<LiveRenderMailbox> liveSettings = std::make_shared<LiveRenderMailbox>();
    int livePreviewChannel = -1;
    int livePreviewSlot = -1;
    int lastRunExitCode = 0;
    bool hasRun = false;
    bool running = false;
    std::atomic<bool> stopRequested{ false };
    bool previewAudioReady = false;
    bool runIsPreview = false;
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

// ログとRun観測用の状態。GUI session の運用情報であり、project/config 保存対象にはしない。
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
