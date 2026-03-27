#include "app/CLI.h"

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
#ifdef _WIN32
struct LocalFreeGuard
{
    LPWSTR* ptr = nullptr;

    ~LocalFreeGuard()
    {
        if (ptr != nullptr)
        {
            LocalFree(ptr);
        }
    }
};
#endif

bool ParseNarrowArgs(const std::vector<std::string>& args, CLIOptions& outOptions)
{
    outOptions = CLIOptions{};
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
            outOptions.startCLI = true;
        }
        else if (arg == "--preset")
        {
            if (i + 1 >= args.size())
            {
                return false;
            }
            outOptions.presetName = args[++i];
            outOptions.startCLI = true;
        }
        else if (arg == "--cli")
        {
            outOptions.startCLI = true;
        }
        else if (arg == "--gui")
        {
            // 後に指定されたオプションを優先するため、--cli の後の --gui ではGUI起動を選ぶ。
            outOptions.startCLI = false;
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
bool ParseWideArgs(CLIOptions& outOptions)
{
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv == nullptr || argc <= 0)
    {
        return false;
    }
    LocalFreeGuard argvGuard{ argv };

    outOptions = CLIOptions{};
    // 日本語パスを壊さないため、Windowsではワイド文字引数を優先して解釈する。
    for (int i = 1; i < argc; i++)
    {
        const std::wstring arg = argv[i];
        if (arg == L"--config")
        {
            if (i + 1 >= argc)
            {
                return false;
            }
            outOptions.configPath = std::filesystem::path(argv[++i]);
            outOptions.startCLI = true;
        }
        else if (arg == L"--preset")
        {
            if (i + 1 >= argc)
            {
                return false;
            }
            outOptions.presetName = WideToUtf8(argv[++i]);
            outOptions.startCLI = true;
        }
        else if (arg == L"--cli")
        {
            outOptions.startCLI = true;
        }
        else if (arg == L"--gui")
        {
            // 後に指定されたオプションを優先するため、--cli の後の --gui ではGUI起動を選ぶ。
            outOptions.startCLI = false;
        }
        else if (arg == L"--help" || arg == L"-h")
        {
            std::cout << "Usage: F-Synthesizer.exe [--gui] [--cli] [--config path/to/config.json] [--preset name]" << std::endl;
            std::cout << "Default: start GUI when no CLI options are given." << std::endl;
            outOptions.showHelp = true;
            return true;
        }
    }

    return true;
}
#endif
} // namespace

bool ParseCLIArguments(int argc, char** argv, CLIOptions& outOptions)
{
#ifdef _WIN32
    // ワイド文字解析に成功した場合は通常引数解析へフォールバックしない。
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
