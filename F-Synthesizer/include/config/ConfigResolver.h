#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "AppCore.h"
#include "app/Cli.h"

struct ResolvedRuntimeConfig
{
    AppConfig config;
    std::filesystem::path selectedConfigPath;
    std::vector<std::string> infoLines;
};

bool ResolveRuntimeConfig(const CliOptions& options, ResolvedRuntimeConfig& outResolved, std::string& err);
