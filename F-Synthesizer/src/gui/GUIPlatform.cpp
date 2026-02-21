#include "gui/GUIPlatform.h"

#include <array>
#include <cstring>
#include <cwchar>

#include <Windows.h>
#include <commdlg.h>

#include "io/PlatformPaths.h"

bool BrowseOpenPath(const std::string& initialPathUtf8, const wchar_t* filter, std::string& outPathUtf8)
{
    wchar_t fileBuf[2048]{};
    if (!initialPathUtf8.empty())
    {
        std::wstring initial = Utf8ToWide(initialPathUtf8);
        wcsncpy_s(fileBuf, initial.c_str(), _TRUNCATE);
    }

    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = fileBuf;
    ofn.nMaxFile = static_cast<DWORD>(std::size(fileBuf));
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (!GetOpenFileNameW(&ofn))
    {
        return false;
    }

    outPathUtf8 = WideToUtf8(fileBuf);
    return true;
}

bool BrowseSavePath(const std::string& initialPathUtf8, const wchar_t* filter, const wchar_t* defExt, std::string& outPathUtf8)
{
    wchar_t fileBuf[2048]{};
    if (!initialPathUtf8.empty())
    {
        std::wstring initial = Utf8ToWide(initialPathUtf8);
        wcsncpy_s(fileBuf, initial.c_str(), _TRUNCATE);
    }

    OPENFILENAMEW ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFile = fileBuf;
    ofn.nMaxFile = static_cast<DWORD>(std::size(fileBuf));
    ofn.lpstrFilter = filter;
    ofn.nFilterIndex = 1;
    ofn.lpstrDefExt = defExt;
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    if (!GetSaveFileNameW(&ofn))
    {
        return false;
    }

    outPathUtf8 = WideToUtf8(fileBuf);
    return true;
}

std::string CompactPathForUi(const std::string& path, size_t maxChars)
{
    if (path.size() <= maxChars)
    {
        return path;
    }
    const size_t head = maxChars / 2 - 3;
    const size_t tail = maxChars - head - 3;
    return path.substr(0, head) + "..." + path.substr(path.size() - tail);
}

void CopyPath(char* dst, size_t dstSize, const std::filesystem::path& path)
{
    if (dst == nullptr || dstSize == 0)
    {
        return;
    }
    const std::string s = PathToUtf8(path);
    strncpy_s(dst, dstSize, s.c_str(), _TRUNCATE);
}

