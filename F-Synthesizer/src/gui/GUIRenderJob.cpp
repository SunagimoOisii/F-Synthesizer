#include "gui/GUIRenderJob.h"
#include "gui/GUIActions.h"
#include "gui/GUIActionsInternal.h"
#include "gui/PreviewAudio.h"
#include <chrono>
#include <future>
#include <system_error>

namespace gui::detail
{
std::string FormatRunException(const std::exception &ex)
{
    if (const auto *error = dynamic_cast<const std::system_error *>(&ex))
        return std::string(error->what()) + " code=" + std::to_string(error->code().value()) +
               " category=" + error->code().category().name();
    return ex.what();
}
std::string BuildUserErrorMessage(const std::string &summary, const std::string &detail)
{
    return detail.empty() ? summary : summary + " (" + detail + ")";
}
void ReportRunStartFailure(GUIState &state, const std::string &detail)
{
    state.running = false;
    state.hasRun = true;
    state.lastRunExitCode = 1;
    state.previewAudioReady = false;
    state.runIsPreview = false;
    AppendGUILog(state, "[GUI] Run start failed: " + detail);
    RaiseGUIError(state, BuildUserErrorMessage("再生・書き出しを開始できません。実行環境を確認してください。", detail),
                  0, true);
}
namespace
{
int ExecuteRenderJob(GUIState &state, const RenderJob &job)
{
    try
    {
        if (job.options.mode == RunMode::Preview)
        {
            // Start an existing device; never initialize COM/device ownership here.
            PreviewAudioStreamSink sink(state.playback, job.startTick, job.frameOffset);
            return RunPreviewStreaming(job.project, job.options, job.overrides, &state.observer, sink, job.loop);
        }
        return Run(job.project, job.options, job.overrides, &state.observer, nullptr);
    }
    catch (const std::exception &ex)
    {
        AppendGUILogToTab(state, job.logTab, "[GUI] Run exception: " + FormatRunException(ex));
    }
    catch (...)
    {
        AppendGUILogToTab(state, job.logTab, "[GUI] Run exception: unknown exception");
    }
    return 1;
}
} // namespace
void LaunchRenderJob(GUIState &state, RenderJob job)
{
    state.runLogTab = job.logTab;
    state.observer.logs = &LogsByTab(state, job.logTab);
    {
        std::lock_guard<std::mutex> lock(state.logMutex);
        state.observer.logs->clear();
    }
    state.lastPeak = 0.0;
    state.hasPeak = false;
    state.runIsPreview = job.options.mode == RunMode::Preview;
    state.previewAudioReady = false;
    AppendGUILog(state, state.runIsPreview ? "[GUI] Preview started" : "[GUI] Export started");
    if (job.overrides.noteTicks)
        AppendGUILog(state, "[GUI] Edited/preview note events: " + std::to_string(job.overrides.noteTicks->size()));
    AppendGUILog(state, "[GUI] secStart=" + std::to_string(job.options.startSec) + " secDuration=" +
                            std::to_string(job.options.durationSec) + " Output: " + state.lastOutputPath);
    state.hasRun = false;
    state.stopRequested.store(false, std::memory_order_relaxed);
    state.running = true;
    try
    {
        state.runFuture =
            std::async(std::launch::async, [&state, job = std::move(job)] { return ExecuteRenderJob(state, job); });
    }
    catch (const std::exception &ex)
    {
        ReportRunStartFailure(state, FormatRunException(ex));
    }
    catch (...)
    {
        ReportRunStartFailure(state, "unknown exception");
    }
}
} // namespace gui::detail

namespace gui
{
bool TryFinalizeCompletedRun(GUIState &state)
{
    if (!state.running || !state.runFuture.valid() ||
        state.runFuture.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
    {
        return false;
    }

    try
    {
        state.lastRunExitCode = state.runFuture.get();
    }
    catch (const std::exception &ex)
    {
        state.lastRunExitCode = 1;
        detail::AppendGUILogToTab(state, state.runLogTab,
                                  "[GUI] Run future exception: " + detail::FormatRunException(ex));
        RaiseGUIError(
            state,
            detail::BuildUserErrorMessage("Export/Preview 実行中に例外が発生しました。ログを確認してください。",
                                          detail::FormatRunException(ex)),
            0, true);
    }
    catch (...)
    {
        state.lastRunExitCode = 1;
        detail::AppendGUILogToTab(state, state.runLogTab, "[GUI] Run future exception: unknown exception");
        RaiseGUIError(state, "Export/Preview 実行中に不明な例外が発生しました。ログを確認してください。", 0, true);
    }
    state.hasRun = true;
    state.running = false;
    if (state.lastRunExitCode == 1 && !state.hasUIError)
        RaiseGUIError(state,
                      state.runIsPreview ? "再生を開始できませんでした。MIDIと音声出力を確認してください。"
                                         : "WAVを書き出せませんでした。MIDIと出力先を確認してください。",
                      0, true);
    detail::AppendGUILogToTab(state, state.runLogTab,
                              std::string("[GUI] Run finished: exit=") + std::to_string(state.lastRunExitCode));
    const bool finishedPreview = state.runIsPreview;
    if (state.runIsPreview)
    {
        if (state.lastRunExitCode == 0)
        {
            state.previewAudioReady = true;
            detail::AppendGUILogToTab(state, state.runLogTab, "[GUI] Preview streaming completed");
        }
        else
        {
            state.previewAudioReady = false;
        }
        state.runIsPreview = false;
    }
    if (!finishedPreview && state.lastRunExitCode == 0 && !state.lastOutputPath.empty() &&
        state.lastOutputPath != "[memory preview]")
    {
        state.recentWavPaths.insert(state.recentWavPaths.begin(), state.lastOutputPath);
        if (state.recentWavPaths.size() > 5)
        {
            state.recentWavPaths.resize(5);
        }
    }
    return true;
}

void StopGUIRunAndPreview(GUIState &state)
{
    StopPreviewAudio(state.playback);
    if (state.running)
    {
        state.stopRequested.store(true, std::memory_order_relaxed);
        AppendGUILog(state, "[GUI] Stop requested (render cancellation signal sent)");
    }
}
} // namespace gui
