#include "RunInternal.h"

#include <cstdlib>
#include <vector>

#include <Windows.h>

namespace app::run
{
namespace
{
std::filesystem::path GetExecutableDirectory()
{
    // MAX_PATH 固定バッファを避けるため、十分な長さになるまで段階的に拡張する。
    std::vector<wchar_t> modulePath(512, L'\0');
    while (true)
    {
        DWORD len = GetModuleFileNameW(nullptr, modulePath.data(), static_cast<DWORD>(modulePath.size()));
        if (len == 0)
        {
            return std::filesystem::path(".");
        }
        if (len < modulePath.size() - 1)
        {
            std::filesystem::path p(modulePath.data());
            if (p.has_parent_path())
            {
                return p.parent_path();
            }
            return std::filesystem::path(".");
        }
        if (modulePath.size() >= 32768)
        {
            return std::filesystem::path(".");
        }
        modulePath.resize(modulePath.size() * 2, L'\0');
    }
}

std::filesystem::path GetProjectRootFromEnv()
{
    wchar_t* envRoot = nullptr;
    size_t envLen = 0;
    if (_wdupenv_s(&envRoot, &envLen, L"FSYNTH_ROOT") != 0)
    {
        return std::filesystem::path();
    }
    if (envRoot == nullptr || envLen == 0 || envRoot[0] == L'\0')
    {
        if (envRoot != nullptr)
        {
            free(envRoot);
        }
        return std::filesystem::path();
    }
    std::filesystem::path root(envRoot);
    free(envRoot);
    return root;
}
} // namespace

std::filesystem::path FindProjectRootInternal()
{
    {
        const std::filesystem::path envRoot = GetProjectRootFromEnv();
        if (!envRoot.empty())
        {
            return envRoot;
        }
    }

    std::error_code ec;
    std::filesystem::path cur = std::filesystem::current_path(ec);
    if (ec || cur.empty())
    {
        // 実行環境で current_path が取得できない場合は実行ファイルの場所を基点に探索する。
        cur = GetExecutableDirectory();
    }

    auto hasProjectMarker = [](const std::filesystem::path& dir)
    {
        std::error_code existsEc;
        return std::filesystem::exists(dir / "config" / "default.json", existsEc) && !existsEc;
    };

    for (int depth = 0; depth < 8; depth++)
    {
        if (hasProjectMarker(cur))
        {
            return cur;
        }
        if (!cur.has_parent_path())
        {
            break;
        }
        cur = cur.parent_path();
    }

    cur = GetExecutableDirectory();
    // current_path 起点で見つからない場合のみ、実行ファイルの場所から再探索する。
    for (int depth = 0; depth < 8; depth++)
    {
        if (hasProjectMarker(cur))
        {
            return cur;
        }
        if (!cur.has_parent_path())
        {
            break;
        }
        cur = cur.parent_path();
    }

    return GetExecutableDirectory();
}

} // namespace app::run
