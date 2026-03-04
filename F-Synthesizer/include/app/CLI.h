#pragma once

#include <filesystem>
#include <string>

// コマンドライン引数を解析した結果を保持する型。
// mainの分岐判定（GUI/CLI/Help）と設定解決入力に使う。
struct CLIOptions
{
    std::filesystem::path configPath;
    std::string presetName;
    bool startCLI = false;
    bool showHelp = false;
};

bool ParseCLIArguments(int argc, char** argv, CLIOptions& outOptions);
