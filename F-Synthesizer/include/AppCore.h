#pragma once

#include <array>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "core/AudioBuffer.h"
#include "SynthEngine/LiveRenderSettings.h"
#include "midi/MIDIParser.h"
#include "SynthEngine/InstrumentSoundConfig.h"
#include "SynthEngine/EffectsConfig.h"

struct ProjectModel;

enum class RunMode
{
    Export,
    Preview
};

// Run実行時の挙動を切り替えるオプション集合。
// app層から実行コアへ渡し、Export/Previewの境界条件を統一する。
// durationSec < 0 は「末尾まで」を表す。
struct RenderOptions
{
    RunMode mode = RunMode::Export;
    double startSec = 0.0;
    double durationSec = -1.0; // < 0 means full length
    bool writeWAV = true;
    bool allowCancel = true;
};

// Project保存形式に入れない、1回の実行だけの差し替え入力。
struct RenderRuntimeOverrides
{
    std::shared_ptr<const std::vector<MIDIEventTick>> noteTicks;
    int ticksPerQuarter = 0;
    std::shared_ptr<LiveRenderMailbox> liveSettings;
};

// Run実行の進捗通知と停止要求を受け持つ観測I/F。
// GUIとCLIの双方から同じログ経路を扱えるようにするための境界型。
struct IRunObserver
{
    virtual ~IRunObserver() = default;
    virtual void OnLogLine(const std::string& line) = 0;
    virtual bool ShouldCancel() { return false; }
};

struct IPreviewStreamSink
{
    virtual ~IPreviewStreamSink() = default;
    virtual bool Begin(int sampleRate, int channels, int totalFrames, bool loop) = 0;
    virtual bool WriteFrame(double left, double right) = 0;
    virtual bool WriteFrames(const double* interleavedStereo, int frameCount)
    {
        if (interleavedStereo == nullptr && frameCount > 0)
        {
            return false;
        }
        for (int i = 0; i < frameCount; i++)
        {
            if (!WriteFrame(interleavedStereo[i * 2 + 0], interleavedStereo[i * 2 + 1]))
            {
                return false;
            }
        }
        return true;
    }
    virtual void Complete(bool canceled) = 0;
};

std::filesystem::path FindProjectRootPath();
RenderOptions DefaultRenderOptions();
RenderOptions DefaultPreviewRenderOptions();
int Run(const ProjectModel& project);
int Run(const ProjectModel& project, IRunObserver* observer);
int Run(const ProjectModel& project, const RenderOptions& options);
int Run(const ProjectModel& project, const RenderOptions& options, IRunObserver* observer);
// 戻り値は 0=成功, 1=失敗, 2=キャンセル要求受理（allowCancel=true かつ observer 経由）で固定。
// renderedSound が null でなければ、保存成否にかかわらずレンダ結果を書き戻す。
int Run(
    const ProjectModel& project,
    const RenderOptions& options,
    const RenderRuntimeOverrides& overrides,
    IRunObserver* observer,
    SoundData* renderedSound);
int RunPreviewStreaming(
    const ProjectModel& project,
    const RenderOptions& options,
    const RenderRuntimeOverrides& overrides,
    IRunObserver* observer,
    IPreviewStreamSink& streamSink,
    bool loop);
