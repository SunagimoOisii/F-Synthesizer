#include "Internal.h"

#include "../ConfigFileInternal.h"

#include "io/PlatformPaths.h"

namespace config::internal::load
{
bool LoadConfigFromText(
    const std::string& text,
    const std::filesystem::path& baseDir,
    AppConfig& cfg,
    std::string& err)
{
    if (auto v = ReadJSONString(text, "midiPath"))
    {
        cfg.midiPath = ResolvePathFromBase(baseDir, *v);
    }
    if (auto v = ReadJSONString(text, "wavPath"))
    {
        cfg.wavPath = ResolvePathFromBase(baseDir, *v);
    }
    if (auto v = ReadJSONInt(text, "targetChannel"))
    {
        cfg.targetChannel = *v;
    }
    if (auto v = ReadJSONInt(text, "initialSeconds"))
    {
        cfg.initialSeconds = *v;
    }
    if (auto v = ReadJSONInt(text, "bits"))
    {
        cfg.bits = *v;
    }
    if (auto v = ReadJSONInt(text, "sampleRate"))
    {
        cfg.sampleRate = *v;
    }
    if (auto v = ReadJSONDouble(text, "extraReleaseSec"))
    {
        cfg.extraReleaseSec = *v;
    }
    if (auto v = ReadJSONString(text, "defaultWave"))
    {
        WaveType w{};
        if (!TryParseWaveType(*v, w))
        {
            err = "invalid defaultWave: " + *v;
            return false;
        }
        cfg.defaultWave = w;
    }

    if (cfg.targetChannel < -1 || cfg.targetChannel > 15)
    {
        err = "targetChannel must be -1 or 0..15";
        return false;
    }
    if (cfg.sampleRate <= 0)
    {
        err = "sampleRate must be positive";
        return false;
    }
    if (cfg.initialSeconds <= 0)
    {
        err = "initialSeconds must be positive";
        return false;
    }
    if (cfg.bits != 16)
    {
        // Writer実装の現行制約に合わせて早期に失敗させる。
        err = "bits must be 16";
        return false;
    }
    if (!LoadChannelsDiff(text, cfg, err))
    {
        return false;
    }
    if (!LoadChannelMixDiff(text, cfg, err))
    {
        return false;
    }

    return true;
}
} // namespace config::internal::load
