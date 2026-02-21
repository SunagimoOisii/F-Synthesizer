#pragma once

#include <filesystem>
#include <string>
#include <string_view>

// path と文字列を相互変換する共通入口。
// WindowsのUTF-16と、設定/ログのUTF-8表現を往復させる。
std::string PathToUtf8(const std::filesystem::path& path);
std::filesystem::path Utf8ToPath(std::string_view utf8);
std::wstring Utf8ToWide(std::string_view utf8);
std::string WideToUtf8(std::wstring_view wide);
std::filesystem::path NormalizePath(const std::filesystem::path& path);
std::filesystem::path ResolvePathFromBase(const std::filesystem::path& baseDir, const std::string& value);
bool EnsureDirectoryForFile(const std::filesystem::path& filePath, std::string& err);
std::string FormatPathDiagnostic(
    const std::string& operation,
    const std::filesystem::path& path,
    const std::string& cause,
    const std::string& hint);
