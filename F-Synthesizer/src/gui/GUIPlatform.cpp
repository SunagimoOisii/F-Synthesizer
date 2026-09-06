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

    // Win32ファイルダイアログはワイド文字APIを使い、UTF-8とはここで相互変換する。
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

std::string CompactPathForUI(const std::string& path, size_t maxChars)
{
    if (path.size() <= maxChars)
    {
        return path;
    }
    // 先頭と末尾を残して省略し、ファイル名側の視認性を優先する。
    if (maxChars <= 3) return std::string(maxChars, '.');
    size_t head = (maxChars - 3) / 2;
    size_t tailStart = path.size() - (maxChars - head - 3);
    // UTF-8 の途中で切らない。
    while (head > 0 && (static_cast<unsigned char>(path[head]) & 0xc0) == 0x80) --head;
    while (tailStart < path.size() && (static_cast<unsigned char>(path[tailStart]) & 0xc0) == 0x80) ++tailStart;
    return path.substr(0, head) + "..." + path.substr(tailStart);
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

