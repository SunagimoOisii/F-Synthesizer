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
            kick.bodyFreq = 58.0;
            kick.pitchStart = 4.2;
            kick.pitchDecaySec = 0.06;
            kick.clickLevel = 0.22;
            kick.clickDecaySec = 0.008;
            kick.bodyLevel = 0.9;
            kick.drive = 0.28;

            DrumConfig snare{ DrumType::Snare };
            snare.gain = 0.6;
            snare.bodyFreq = 220.0;
            snare.bodyLevel = 0.48;
            snare.snapLevel = 0.72;
            snare.snapDecaySec = 0.055;
            snare.hpCut = 700.0;
            snare.lpCut = 6000.0;
            snare.drive = 0.22;
            snare.noiseColor = (int)NoiseType::Pink;

            DrumConfig hat{ DrumType::Hat };
            hat.gain = 0.15;
            hat.metalLevel = 0.46;
            hat.airLevel = 0.28;
            hat.decaySec = 0.045;
            hat.hpCut = 4000.0;
            hat.lpCut = 6000.0;
            hat.drive = 0.16;
            hat.noiseColor = (int)NoiseType::Pink;

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
    config->initialSeconds = 6;
    config->bits = 16;
    config->sampleRate = 44100;
    config->extraReleaseSec = 0.3;
    config->channelConfigs = BuildDefaultChannelConfigs();
    config->channelMixStates = BuildDefaultChannelMixStates();
    return *config;
}
} // namespace app::run
