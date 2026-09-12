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
#include "gui/GUIPianoRoll.h"
#include "gui/PreviewAudio.h"
#include "gui/GUIToneWorkspace.h"

// ステップシーケンサー状態 (ch10 / drumChannel 専用)
struct GUIStepSeqState
{
    int startTick = 0;
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
    double comparisonGain = 1;
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
    std::string revision{};
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
    bool toneWorkspaceReady = false;
    std::array<gui::ChannelToneWorkspace, 16> tones;
    float auditionLengthSec = 0.8f;
    bool toneExtraOpen = false;
    bool toneNotesOpen = false;
    bool scopeWholeMix = false;
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
    int presetIndex = 0;
    bool serialSave = false;
    int selectedDrumNote = 36;
    int tonePreviewNoteNumber = 60;
    char presetName[128]{ "custom" };
    std::string lastOutputPath{};
    std::string lastPresetPath{};
    std::array<ChannelMixState, 16> channelMixStates{};
    bool drumChannelSpecialHandling = true;
    bool previewLoop = false;
    gui::PianoRollState pianoRoll{};
    GUIStepSeqState stepSeq{};
};

// 画面表示中だけ意味を持つ一時状態。project/config 保存対象にはしない。
namespace gui { enum class TransportAction { None, Song, Audition }; }

struct GUITransientState
{
    std::unique_ptr<gui::ToneVersion> toneEditBefore;
    int toneEditChannel = -1;
    gui::TransportAction transportAction = gui::TransportAction::None;
    bool toneAuditionActive = false;
    bool resumeAfterAudition = false;
    int resumeSongTick = 0;
    int songCursorTick = 0;
    uint64_t previewFrameOffset = 0;
    std::shared_ptr<AudioScope> audioScope = std::make_shared<AudioScope>();
    std::string pendingOpenPath{};
    bool pendingOpenIsSong = false;
    uint64_t observedNotesVersion = 0;
    bool hasUIError = false;
    bool showErrorDialog = false;
    int UIErrorAction = 0; // 0=None, 1=BrowseMIDI, 2=BrowseOutput, 3=GoSound, 4=GoMusic
    std::string UIErrorMessage{};
    double lastPeak = 0.0;
    bool hasPeak = false;
    int previewRequestedStartTick = 0;
    double previewRequestedDurationSec = 0.0;
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
