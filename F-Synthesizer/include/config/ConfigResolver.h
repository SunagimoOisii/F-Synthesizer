#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "AppCore.h"
#include "app/Cli.h"

// 実行時設定の解決結果。
// 解決済みAppConfigと、採用された設定ファイル情報/通知行をまとめて返す。
struct ResolvedRuntimeConfig
{
    AppConfig config;
    std::filesystem::path selectedConfigPath;
    std::vector<std::string> infoLines;
};

bool ResolveRuntimeConfig(const CliOptions& options, ResolvedRuntimeConfig& outResolved, std::string& err);
