#include "app/Cli.h"

#include <iostream>

bool ParseCliArguments(int argc, char** argv, CliOptions& outOptions)
{
    outOptions = CliOptions{};
    for (int i = 1; i < argc; i++)
    {
        const std::string arg = argv[i];
        if (arg == "--config")
        {
            if (i + 1 >= argc)
            {
                return false;
            }
            outOptions.configPath = std::filesystem::path(argv[++i]);
            outOptions.startCli = true;
        }
        else if (arg == "--preset")
        {
            if (i + 1 >= argc)
            {
                return false;
            }
            outOptions.presetName = argv[++i];
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
