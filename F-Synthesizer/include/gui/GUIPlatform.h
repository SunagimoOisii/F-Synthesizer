#pragma once

#include <cstddef>
#include <filesystem>
#include <string>

bool BrowseOpenPath(const std::string& initialPathUtf8, const wchar_t* filter, std::string& outPathUtf8);
bool BrowseSavePath(const std::string& initialPathUtf8, const wchar_t* filter, const wchar_t* defExt, std::string& outPathUtf8);
std::string CompactPathForUi(const std::string& path, size_t maxChars = 72);
void CopyPath(char* dst, size_t dstSize, const std::filesystem::path& path);
