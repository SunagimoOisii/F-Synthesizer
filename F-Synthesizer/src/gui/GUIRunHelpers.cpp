#include "gui/GUIRunHelpers.h"

#include <algorithm>
#include <chrono>
#include <ctime>

#include "io/PlatformPaths.h"

namespace gui
{
bool ValidateRunSettings(
    const std::string& midiPathUtf8,
    const std::string& wavPathUtf8,
    int targetChannel,
    int sampleRate,
    int initialSeconds,
    int bits,
    std::string& err)
{
    // GUI入力の最終ガード。Run呼び出し前に即時失敗できる条件をここで弾く。
    if (midiPathUtf8.empty())
    {
        err = "MIDI Path is empty.";
        return false;
    }
    if (wavPathUtf8.empty())
    {
        err = "Output Path is empty.";
        return false;
    }

    const std::filesystem::path wavPath = Utf8ToPath(wavPathUtf8);
    if (wavPath.has_filename() && !wavPath.extension().empty())
    {
        std::error_code isDirEc;
        if (std::filesystem::is_directory(wavPath, isDirEc))
        {
            err = "Output Path points to a directory, not a .wav file.";
            return false;
        }
        if (isDirEc)
        {
            err = "Output Path cannot be inspected: " + isDirEc.message();
            return false;
        }
    }
    else
    {
        err = "Output Path must include a .wav filename.";
        return false;
    }

    std::error_code existsEc;
    if (!std::filesystem::exists(Utf8ToPath(midiPathUtf8), existsEc))
    {
        if (existsEc)
        {
            err = "MIDI file cannot be inspected: " + existsEc.message();
            return false;
        }
        err = "MIDI file not found: " + midiPathUtf8;
        return false;
    }
    if (targetChannel < -1 || targetChannel > 15)
    {
        err = "Target Channel must be -1 or 0..15.";
        return false;
    }
    if (sampleRate <= 0)
    {
        err = "Sample Rate must be positive.";
        return false;
    }
    if (initialSeconds <= 0)
    {
        err = "Initial Seconds must be positive.";
        return false;
    }
    if (bits != 16)
    {
        err = "Bits must be 16 in current implementation.";
        return false;
    }
    return true;
}

std::filesystem::path BuildSerialWAVPath(const std::filesystem::path& basePath)
{
    std::error_code ec;
    std::filesystem::create_directories(basePath.parent_path(), ec);

    // 連番保存は timestamp を基準にし、同じ秒で衝突したときだけ suffix を付ける。
    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm tmLocal{};
    localtime_s(&tmLocal, &tt);

    char stamp[32]{};
    std::strftime(stamp, sizeof(stamp), "%Y%m%d_%H%M%S", &tmLocal);

    const std::string stem = basePath.stem().string();
    const std::string ext = basePath.extension().string().empty() ? ".wav" : basePath.extension().string();
    std::filesystem::path candidate = basePath.parent_path() / (stem + "_" + stamp + ext);
    for (int i = 1; i <= 99; i++)
    {
        std::error_code existsEc;
        if (!std::filesystem::exists(candidate, existsEc) || existsEc)
        {
            break;
        }
        candidate = basePath.parent_path() / (stem + "_" + stamp + "_" + std::to_string(i) + ext);
    }
    return candidate;
}

std::filesystem::path BuildPreviewWAVPath(const std::filesystem::path& basePath, int channel)
{
    const std::string stem = basePath.stem().string();
    const std::string ext = basePath.extension().string().empty() ? ".wav" : basePath.extension().string();
    return basePath.parent_path() / (stem + "_preview_ch" + std::to_string(channel) + ext);
}
} // namespace gui
