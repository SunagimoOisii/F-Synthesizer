#include "app/AppEntry.h"

#include <iostream>
#include <string>

#include "AppCore.h"
#include "config/ConfigResolver.h"

int RunCLIApplication(const CLIOptions& options)
{
    ResolvedRuntimeConfig resolved{};
    std::string err;
    // 設定解決は ConfigResolver に委譲し、入口は「表示 + Run呼び出し」に限定する。
    if (!ResolveRuntimeConfig(options, resolved, err))
    {
        std::cout << err << std::endl;
        return 1;
    }
    for (const std::string& line : resolved.infoLines)
    {
        std::cout << line << std::endl;
    }
    return Run(resolved.config);
}
