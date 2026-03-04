#include <Windows.h>

#include "AppCore.h"
#include "app/AppEntry.h"
#include "app/Cli.h"
#include "app/RunInternal.h"

std::filesystem::path FindProjectRootPath()
{
    return app::run::FindProjectRootInternal();
}

AppConfig DefaultConfig()
{
    return app::run::BuildDefaultConfig();
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
    opt.writeWav = false;
    opt.allowCancel = true;
    return opt;
}

int Run(const AppConfig& config, const RenderOptions& options, IRunObserver* observer, SoundData* renderedSound)
{
    return app::run::RunMain(config, options, observer, renderedSound);
}

int Run(const AppConfig& config)
{
    return Run(config, DefaultRenderOptions(), nullptr, nullptr);
}

int Run(const AppConfig& config, const RenderOptions& options)
{
    return Run(config, options, nullptr, nullptr);
}

int Run(const AppConfig& config, IRunObserver* observer)
{
    return Run(config, DefaultRenderOptions(), observer, nullptr);
}

int Run(const AppConfig& config, const RenderOptions& options, IRunObserver* observer)
{
    return Run(config, options, observer, nullptr);
}

int RunGUIApp();

int main(int argc, char** argv)
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    CliOptions cli{};
    if (!ParseCliArguments(argc, argv, cli))
    {
        return 1;
    }
    if (cli.showHelp)
    {
        return 0;
    }
    if (!cli.startCli)
    {
        // 既定はGUI起動。CLI明示時だけ RunCliApplication へ切り替える。
        return RunGUIApp();
    }

    return RunCliApplication(cli);
}
