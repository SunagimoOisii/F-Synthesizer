#include <Windows.h>
#include <iostream>
#include <exception>

#include "AppCore.h"
#include "app/AppEntry.h"
#include "app/CLI.h"
#include "app/RunInternal.h"
#include "project/ProjectModel.h"

std::filesystem::path FindProjectRootPath()
{
    return app::run::FindProjectRootInternal();
}

RenderOptions DefaultRenderOptions()
{
    return RenderOptions{};
}

RenderOptions DefaultPreviewRenderOptions()
{
    RenderOptions opt{};
    // Preview は短時間試聴向けの既定値に固定し、保存I/Oは無効化する。
    opt.mode = RunMode::Preview;
    opt.startSec = 0.0;
    opt.durationSec = 8.0;
    opt.writeWAV = false;
    opt.allowCancel = true;
    return opt;
}

int Run(
    const ProjectModel& project,
    const RenderOptions& options,
    const RenderRuntimeOverrides& overrides,
    IRunObserver* observer,
    SoundData* renderedSound)
{
    return app::run::RunMain(project, options, overrides, observer, renderedSound);
}

int RunPreviewStreaming(
    const ProjectModel& project,
    const RenderOptions& options,
    const RenderRuntimeOverrides& overrides,
    IRunObserver* observer,
    IPreviewStreamSink& streamSink,
    bool loop)
{
    return app::run::RunPreviewStreamingInternal(project, options, overrides, observer, streamSink, loop);
}

int Run(const ProjectModel& project)
{
    return Run(project, DefaultRenderOptions(), RenderRuntimeOverrides{}, nullptr, nullptr);
}

int Run(const ProjectModel& project, const RenderOptions& options)
{
    return Run(project, options, RenderRuntimeOverrides{}, nullptr, nullptr);
}

int Run(const ProjectModel& project, IRunObserver* observer)
{
    return Run(project, DefaultRenderOptions(), RenderRuntimeOverrides{}, observer, nullptr);
}

int Run(const ProjectModel& project, const RenderOptions& options, IRunObserver* observer)
{
    return Run(project, options, RenderRuntimeOverrides{}, observer, nullptr);
}

int RunGUIApp();

int main(int argc, char** argv)
{
    try
    {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    CLIOptions cli{};
    if (!ParseCLIArguments(argc, argv, cli))
    {
        return 1;
    }
    if (cli.showHelp)
    {
        return 0;
    }
    if (!cli.startCLI)
    {
        // 既定はGUI起動。CLI明示時だけ RunCLIApplication へ切り替える。
        return RunGUIApp();
    }

    return RunCLIApplication(cli);
    }
    catch (const std::exception& ex)
    {
        std::cerr << "F-Synthesizer: " << ex.what() << std::endl;
        return 1;
    }
}
