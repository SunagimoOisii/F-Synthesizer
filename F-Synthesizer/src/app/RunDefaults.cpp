#include "RunInternal.h"

#include <Windows.h>

namespace app::run
{
namespace
{
std::filesystem::path GetExecutableDirectory()
{
    wchar_t modulePath[MAX_PATH] = {};
    DWORD len = GetModuleFileNameW(nullptr, modulePath, MAX_PATH);
    if (len == 0 || len >= MAX_PATH)
    {
        return std::filesystem::path(".");
    }
    std::filesystem::path p(modulePath);
    if (p.has_parent_path())
    {
        return p.parent_path();
    }
    return std::filesystem::path(".");
}
} // namespace

std::filesystem::path FindProjectRootInternal()
{
    std::error_code ec;
    std::filesystem::path cur = std::filesystem::current_path(ec);
    if (ec || cur.empty())
    {
        cur = GetExecutableDirectory();
    }

    auto hasProjectMarker = [](const std::filesystem::path& dir)
    {
        std::error_code existsEc;
        return std::filesystem::exists(dir / "F-Synthesizer.vcxproj", existsEc) && !existsEc;
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

std::shared_ptr<const std::array<ChannelConfig, 16>> BuildDefaultChannelConfigs()
{
    // 既定設定は不変なので static 化して、毎回の再構築を避ける。
    static const std::shared_ptr<const std::array<ChannelConfig, 16>> table = []()
    {
        auto makeWave = [](WaveType wave,
            double amp, double atk, double dec, double sus, double rel)
        {
            return ChannelConfig{ WaveformConfig{ wave },
                amp, atk, dec, sus, rel };
        };
        auto makeDrumKitDetail = [](const DrumKitConfig& kit,
            double amp, double atk, double dec, double sus, double rel)
        {
            return ChannelConfig{ kit, amp, atk, dec, sus, rel };
        };
        auto makeGmDrumKit = []()
        {
            auto kit = std::make_unique<DrumKitConfig>();
            for (auto& d : kit->map)
            {
                d.type = DrumType::None;
            }

            DrumConfig kick{ DrumType::Kick };
            kick.gain = 0.6;
            kick.baseFreq = 60.0;
            kick.pitchDrop = 3.0;
            kick.pitchDecaySec = 0.06;

            DrumConfig snare{ DrumType::Snare };
            snare.gain = 0.6;
            snare.toneFreq = 220.0;
            snare.toneLevel = 0.55;
            snare.noiseLevel = 0.35;
            snare.hpCut = 700.0;
            snare.lpCut = 6000.0;
            snare.toneWave = (int)WaveType::Triangle;
            snare.noiseType = (int)NoiseType::White;

            DrumConfig hat{ DrumType::Hat };
            hat.gain = 0.15;
            hat.toneFreq = 8000.0;
            hat.toneLevel = 0.2;
            hat.noiseLevel = 0.2;
            hat.hpCut = 4000.0;
            hat.lpCut = 6000.0;
            hat.toneWave = (int)WaveType::Sine;
            hat.noiseType = (int)NoiseType::White;

            kit->map[36] = kick;
            kit->map[38] = snare;
            kit->map[40] = snare;
            kit->map[42] = hat;
            kit->map[44] = hat;
            kit->map[46] = hat;
            kit->map[49] = hat;

            return *kit;
        };

        auto tmp = std::make_shared<std::array<ChannelConfig, 16>>();
        (*tmp)[0] = makeWave(WaveType::Saw, 0.25, 0.01, 0.15, 0.75, 0.20);
        (*tmp)[1] = makeWave(WaveType::Saw, 0.25, 0.01, 0.15, 0.75, 0.20);
        (*tmp)[2] = makeWave(WaveType::Saw, 0.25, 0.01, 0.15, 0.75, 0.20);
        (*tmp)[3] = makeWave(WaveType::Saw, 0.25, 0.01, 0.15, 0.75, 0.20);
        (*tmp)[4] = makeWave(WaveType::Saw, 0.25, 0.01, 0.15, 0.75, 0.20);
        (*tmp)[5] = makeWave(WaveType::Saw, 0.25, 0.01, 0.15, 0.75, 0.20);
        (*tmp)[6] = makeWave(WaveType::Saw, 0.25, 0.01, 0.15, 0.75, 0.20);
        (*tmp)[7] = makeWave(WaveType::Saw, 0.25, 0.01, 0.15, 0.75, 0.20);
        (*tmp)[8] = makeWave(WaveType::Saw, 0.25, 0.01, 0.15, 0.75, 0.20);
        (*tmp)[9] = makeDrumKitDetail(makeGmDrumKit(), 10.0, 0.001, 0.15, 0.1, 0.3);
        (*tmp)[10] = makeWave(WaveType::Saw, 0.25, 0.01, 0.15, 0.75, 0.20);
        (*tmp)[11] = makeWave(WaveType::Saw, 0.25, 0.01, 0.15, 0.75, 0.20);
        (*tmp)[12] = makeWave(WaveType::Saw, 0.25, 0.01, 0.15, 0.75, 0.20);
        (*tmp)[13] = makeWave(WaveType::Saw, 0.25, 0.01, 0.15, 0.75, 0.20);
        (*tmp)[14] = makeWave(WaveType::Saw, 0.25, 0.01, 0.15, 0.75, 0.20);
        (*tmp)[15] = makeWave(WaveType::Saw, 0.25, 0.01, 0.15, 0.75, 0.20);
        return std::static_pointer_cast<const std::array<ChannelConfig, 16>>(tmp);
    }();
    return table;
}

std::shared_ptr<const std::array<ChannelMixState, 16>> BuildDefaultChannelMixStates()
{
    // 既定Mixテーブルも同様に再利用する。
    static const std::shared_ptr<const std::array<ChannelMixState, 16>> table = []()
    {
        auto tmp = std::make_shared<std::array<ChannelMixState, 16>>();
        for (int ch = 0; ch < 16; ch++)
        {
            (*tmp)[ch] = ChannelMixState{};
        }
        return std::static_pointer_cast<const std::array<ChannelMixState, 16>>(tmp);
    }();
    return table;
}

AppConfig BuildDefaultConfig()
{
    const std::filesystem::path projectRoot = FindProjectRootInternal();

    auto config = std::make_unique<AppConfig>();
    config->midiPath = projectRoot / "assets" / "midi" / "solstice_intro.mid";
    config->wavPath = projectRoot / "output" / "test.wav";
    config->targetChannel = -1;
    config->defaultWave = WaveType::Saw;
    config->initialSeconds = 6;
    config->bits = 16;
    config->sampleRate = 44100;
    config->extraReleaseSec = 0.3;
    config->channelConfigs = BuildDefaultChannelConfigs();
    config->channelMixStates = BuildDefaultChannelMixStates();
    return *config;
}
} // namespace app::run
