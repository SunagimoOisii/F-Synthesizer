#include "io/PlatformPaths.h"

#include <sstream>
#include <system_error>

std::string PathToUtf8(const std::filesystem::path& path)
{
    const auto u8 = path.u8string();
    return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
}

std::filesystem::path Utf8ToPath(std::string_view utf8)
{
    std::u8string u8;
    u8.assign(
        reinterpret_cast<const char8_t*>(utf8.data()),
        reinterpret_cast<const char8_t*>(utf8.data() + utf8.size()));
    return std::filesystem::path(u8);
}

std::wstring Utf8ToWide(std::string_view utf8)
{
    return Utf8ToPath(utf8).wstring();
}

std::string WideToUtf8(std::wstring_view wide)
{
    return PathToUtf8(std::filesystem::path(wide));
}

std::filesystem::path NormalizePath(const std::filesystem::path& path)
{
    if (path.empty())
    {
        return path;
    }

    std::error_code ec;
    const std::filesystem::path abs = std::filesystem::absolute(path, ec);
    if (!ec)
    {
        return abs.lexically_normal();
    }
    return path.lexically_normal();
}

std::filesystem::path ResolvePathFromBase(const std::filesystem::path& baseDir, const std::string& value)
{
    const std::filesystem::path path = Utf8ToPath(value);
    if (path.is_absolute())
    {
        return NormalizePath(path);
    }
    return NormalizePath(baseDir / path);
}

bool EnsureDirectoryForFile(const std::filesystem::path& filePath, std::string& err)
{
    if (!filePath.has_parent_path())
    {
        return true;
    }

    std::error_code ec;
    std::filesystem::create_directories(filePath.parent_path(), ec);
    if (ec)
    {
        err = FormatPathDiagnostic(
            "create output directory",
            filePath.parent_path(),
            ec.message(),
            "check write permission and path spelling");
        return false;
    }
    return true;
}

std::string FormatPathDiagnostic(
    const std::string& operation,
    const std::filesystem::path& path,
    const std::string& cause,
    const std::string& hint)
{
    std::ostringstream oss;
    oss << "[IO] op=" << operation
        << " path=\"" << PathToUtf8(path) << "\""
        << " cause=\"" << cause << "\"";
    if (!hint.empty())
    {
        oss << " hint=\"" << hint << "\"";
    }
    return oss.str();
}
