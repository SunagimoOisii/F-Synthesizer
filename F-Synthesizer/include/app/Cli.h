#pragma once

#include <filesystem>
#include <string>

// コマンドライン引数を解析した結果を保持する型。
// mainの分岐判定（GUI/CLI/Help）と設定解決入力に使う。
struct CliOptions
{
    std::filesystem::path configPath;
    std::string presetName;
    bool startCli = false;
    bool showHelp = false;
};

bool ParseCliArguments(int argc, char** argv, CliOptions& outOptions);
