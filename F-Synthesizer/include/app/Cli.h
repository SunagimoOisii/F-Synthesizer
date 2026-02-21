#pragma once

#include <filesystem>
#include <string>

struct CliOptions
{
    std::filesystem::path configPath;
    std::string presetName;
    bool startCli = false;
    bool showHelp = false;
};

bool ParseCliArguments(int argc, char** argv, CliOptions& outOptions);
