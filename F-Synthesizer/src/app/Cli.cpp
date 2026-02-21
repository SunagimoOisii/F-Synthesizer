#include "app/Cli.h"

#include <iostream>
#include <vector>

#include "io/PlatformPaths.h"

#ifdef _WIN32
#include <Windows.h>
#include <shellapi.h>
#pragma comment(lib, "shell32.lib")
#endif

namespace
{
bool ParseNarrowArgs(const std::vector<std::string>& args, CliOptions& outOptions)
{
    outOptions = CliOptions{};
    // 既存CLI互換を維持するため、--config/--preset/--cli は明示的にCLI起動へ倒す。
    for (size_t i = 1; i < args.size(); i++)
    {
        const std::string& arg = args[i];
        if (arg == "--config")
        {
            if (i + 1 >= args.size())
            {
                return false;
            }
            outOptions.configPath = std::filesystem::path(args[++i]);
            outOptions.startCli = true;
        }
        else if (arg == "--preset")
        {
            if (i + 1 >= args.size())
            {
                return false;
            }
            outOptions.presetName = args[++i];
            outOptions.startCli = true;
        }
        else if (arg == "--cli")
        {
            outOptions.startCli = true;
        }
        else if (arg == "--gui")
        {
            outOptions.startCli = false;
        }
        else if (arg == "--help" || arg == "-h")
        {
            std::cout << "Usage: F-Synthesizer.exe [--gui] [--cli] [--config path/to/config.json] [--preset name]" << std::endl;
            std::cout << "Default: start GUI when no CLI options are given." << std::endl;
            outOptions.showHelp = true;
            return true;
        }
    }
    return true;
}

#ifdef _WIN32
bool ParseWideArgs(CliOptions& outOptions)
{
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv == nullptr || argc <= 0)
    {
        return false;
    }

    outOptions = CliOptions{};
    // 日本語パスを壊さないため、Windowsではwide argvを優先して解釈する。
    for (int i = 1; i < argc; i++)
    {
        const std::wstring arg = argv[i];
        if (arg == L"--config")
        {
            if (i + 1 >= argc)
            {
                LocalFree(argv);
                return false;
            }
            outOptions.configPath = std::filesystem::path(argv[++i]);
            outOptions.startCli = true;
        }
        else if (arg == L"--preset")
        {
            if (i + 1 >= argc)
            {
                LocalFree(argv);
                return false;
            }
            outOptions.presetName = WideToUtf8(argv[++i]);
            outOptions.startCli = true;
        }
        else if (arg == L"--cli")
        {
            outOptions.startCli = true;
        }
        else if (arg == L"--gui")
        {
            outOptions.startCli = false;
        }
        else if (arg == L"--help" || arg == L"-h")
        {
            std::cout << "Usage: F-Synthesizer.exe [--gui] [--cli] [--config path/to/config.json] [--preset name]" << std::endl;
            std::cout << "Default: start GUI when no CLI options are given." << std::endl;
            outOptions.showHelp = true;
            LocalFree(argv);
            return true;
        }
    }

    LocalFree(argv);
    return true;
}
#endif
} // namespace

bool ParseCliArguments(int argc, char** argv, CliOptions& outOptions)
{
#ifdef _WIN32
    // wide解析に成功した場合はnarrow側へフォールバックしない。
    if (ParseWideArgs(outOptions))
    {
        return true;
    }
#endif

    std::vector<std::string> args;
    args.reserve(static_cast<size_t>(argc));
    for (int i = 0; i < argc; i++)
    {
        args.emplace_back(argv[i]);
    }
    return ParseNarrowArgs(args, outOptions);
}
