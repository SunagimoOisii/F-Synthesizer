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
// path を可能なら絶対パス化し、失敗時も lexically_normal で表記ゆれを減らす。
std::filesystem::path NormalizePath(const std::filesystem::path& path);
// value が相対パスなら baseDir 基準で解決し、絶対パスならそのまま正規化する。
std::filesystem::path ResolvePathFromBase(const std::filesystem::path& baseDir, const std::string& value);
// filePath の親ディレクトリを作成する。失敗時は err に診断文字列を返す。
bool EnsureDirectoryForFile(const std::filesystem::path& filePath, std::string& err);
// GUI/CLIで共通利用するI/O診断文字列を組み立てる。
std::string FormatPathDiagnostic(
    const std::string& operation,
    const std::filesystem::path& path,
    const std::string& cause,
    const std::string& hint);
