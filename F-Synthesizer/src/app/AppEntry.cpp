#include "app/AppEntry.h"

#include <iostream>
#include <string>

#include "AppCore.h"
#include "config/ConfigResolver.h"

int RunCliApplication(const CliOptions& options)
{
    ResolvedRuntimeConfig resolved{};
    std::string err;
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
