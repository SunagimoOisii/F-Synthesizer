#pragma once
#include "AppCore.h"
#include "project/ProjectModel.h"
#include <exception>

struct GUIState;
namespace gui::detail
{
struct RenderJob
{
    ProjectModel project;
    RenderOptions options;
    RenderRuntimeOverrides overrides;
    int logTab = 0;
    int startTick = 0;
    uint64_t frameOffset = 0;
    bool loop = false;
};
std::string FormatRunException(const std::exception &ex);
std::string BuildUserErrorMessage(const std::string &summary, const std::string &detail);
void ReportRunStartFailure(GUIState &state, const std::string &detail);
// GUI thread only. Preview devices must already be prepared on this thread.
// Owns worker startup and status/log initialization for export and both previews.
void LaunchRenderJob(GUIState &state, RenderJob job);
} // namespace gui::detail
