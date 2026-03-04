#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "AppCore.h"
#include "app/CLI.h"

// 実行時設定の解決結果。
// 解決済みAppConfigと、採用された設定ファイル情報/通知行をまとめて返す。
struct ResolvedRuntimeConfig
{
    AppConfig config;
    std::filesystem::path selectedConfigPath;
    std::vector<std::string> infoLines;
};

// 目的: CLIオプションから実行設定を解決し、採用元情報を返す。
// 前提: outResolved は上書きされる。--config と --preset は ResolveRuntimeConfig 側で優先順位を判定する。
// 副作用: 読み込んだ設定の説明行を infoLines に追記する。失敗時は false と err を返す。
bool ResolveRuntimeConfig(const CLIOptions& options, ResolvedRuntimeConfig& outResolved, std::string& err);
