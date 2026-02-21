#include "ConfigFileInternal.h"

#include "io/PlatformPaths.h"

namespace config::internal
{
namespace
{
bool ParseDrumConfigObject(const std::string& text, DrumConfig& drum, std::string& err)
{
    if (auto t = ReadJsonString(text, "drumType"))
    {
        DrumType dt{};
        if (!TryParseDrumType(*t, dt))
        {
            err = "invalid drumType: " + *t;
            return false;
        }
        drum.type = dt;
    }
    if (auto v = ReadJsonDouble(text, "gain")) drum.gain = *v;
    if (auto v = ReadJsonDouble(text, "baseFreq")) drum.baseFreq = *v;
    if (auto v = ReadJsonDouble(text, "pitchDrop")) drum.pitchDrop = *v;
    if (auto v = ReadJsonDouble(text, "pitchDecaySec")) drum.pitchDecaySec = *v;
    if (auto v = ReadJsonDouble(text, "toneFreq")) drum.toneFreq = *v;
    if (auto v = ReadJsonDouble(text, "toneLevel")) drum.toneLevel = *v;
    if (auto v = ReadJsonDouble(text, "noiseLevel")) drum.noiseLevel = *v;
    if (auto v = ReadJsonDouble(text, "hpCut")) drum.hpCut = *v;
    if (auto v = ReadJsonDouble(text, "lpCut")) drum.lpCut = *v;
    if (auto v = ReadJsonString(text, "toneWave"))
    {
        WaveType w{};
        if (!TryParseWaveType(*v, w))
        {
            err = "invalid toneWave: " + *v;
            return false;
        }
        drum.toneWave = (int)w;
    }
    if (auto v = ReadJsonString(text, "noiseType"))
    {
        NoiseType n{};
        if (!TryParseNoiseType(*v, n))
        {
            err = "invalid noiseType: " + *v;
            return false;
        }
        drum.noiseType = (int)n;
    }
    return true;
}

bool ParseSourceObject(const std::string& sourceObjText, SourceConfig& outSource, std::string& err)
{
    const auto type = ReadJsonString(sourceObjText, "type");
    if (!type)
    {
        err = "source.type is required";
        return false;
    }

    if (*type == "waveform")
    {
        auto wave = ReadJsonString(sourceObjText, "wave");
        if (!wave)
        {
            err = "waveform source requires 'wave'";
            return false;
        }
        WaveType w{};
        if (!TryParseWaveType(*wave, w))
        {
            err = "invalid wave: " + *wave;
            return false;
        }
        outSource = WaveformConfig{ w };
        return true;
    }
    if (*type == "noise")
    {
        auto noise = ReadJsonString(sourceObjText, "noise");
        if (!noise)
        {
            err = "noise source requires 'noise'";
            return false;
        }
        NoiseType n{};
        if (!TryParseNoiseType(*noise, n))
        {
            err = "invalid noise: " + *noise;
            return false;
        }
        outSource = NoiseConfig{ n };
        return true;
    }
    if (*type == "fm")
    {
        auto carrier = ReadJsonString(sourceObjText, "carrierWave");
        auto mod = ReadJsonString(sourceObjText, "modWave");
        auto carrierRatio = ReadJsonDouble(sourceObjText, "carrierRatio");
        auto modRatio = ReadJsonDouble(sourceObjText, "modRatio");
        auto index = ReadJsonDouble(sourceObjText, "index");
        auto outLevel = ReadJsonDouble(sourceObjText, "outLevel");
        if (!carrier || !mod || !carrierRatio || !modRatio || !index || !outLevel)
        {
            err = "fm source requires carrierWave/modWave/carrierRatio/modRatio/index/outLevel";
            return false;
        }
        WaveType cw{}, mw{};
        if (!TryParseWaveType(*carrier, cw) || !TryParseWaveType(*mod, mw))
        {
            err = "invalid fm wave type";
            return false;
        }
        outSource = FmConfig{ cw, mw, *carrierRatio, *modRatio, *index, *outLevel };
        return true;
    }
    if (*type == "drum")
    {
        DrumConfig drum{};
        if (!ParseDrumConfigObject(sourceObjText, drum, err))
        {
            return false;
        }
        outSource = drum;
        return true;
    }
    if (*type == "drumkit")
    {
        DrumKitConfig kit{};
        for (auto& d : kit.map)
        {
            d.type = DrumType::None;
        }
        std::string mapObj;
        bool mapFound = false;
        if (!ExtractObjectForKey(sourceObjText, "map", mapObj, mapFound, err))
        {
            return false;
        }
        if (mapFound)
        {
            if (!ParseTopLevelObjectEntries(mapObj, [&](const std::string& k, const std::string& valueObj) {
                int note = -1;
                try
                {
                    note = std::stoi(k);
                }
                catch (...)
                {
                    err = "invalid drumkit note key: " + k;
                    return false;
                }
                if (note < 0 || note > 127)
                {
                    err = "drumkit note out of range: " + k;
                    return false;
                }
                DrumConfig d{};
                if (!ParseDrumConfigObject(valueObj, d, err))
                {
                    return false;
                }
                kit.map[note] = d;
                return true;
                }, err))
            {
                return false;
            }
        }
        outSource = kit;
        return true;
    }

    err = "unknown source.type: " + *type;
    return false;
}

bool ParseChannelObject(const std::string& channelObjText, ChannelConfig& cfg, std::string& err)
{
    if (auto v = ReadJsonDouble(channelObjText, "amp")) cfg.amp = *v;
    if (auto v = ReadJsonDouble(channelObjText, "attackSec")) cfg.attackSec = *v;
    if (auto v = ReadJsonDouble(channelObjText, "decaySec")) cfg.decaySec = *v;
    if (auto v = ReadJsonDouble(channelObjText, "sustainLevel")) cfg.sustainLevel = *v;
    if (auto v = ReadJsonDouble(channelObjText, "releaseSec")) cfg.releaseSec = *v;

    std::string sourceObj;
    bool found = false;
    if (!ExtractObjectForKey(channelObjText, "source", sourceObj, found, err))
    {
        return false;
    }
    if (found)
    {
        SourceConfig s = cfg.source;
        if (!ParseSourceObject(sourceObj, s, err))
        {
            return false;
        }
        cfg.source = s;
    }
    return true;
}

bool LoadChannelsDiff(const std::string& text, AppConfig& cfg, std::string& err)
{
    std::string channelsObj;
    bool found = false;
    if (!ExtractObjectForKey(text, "channels", channelsObj, found, err))
    {
        return false;
    }
    if (!found)
    {
        return true;
    }

    auto table = MakeMutableChannelConfigs(cfg);
    if (!ParseTopLevelObjectEntries(channelsObj, [&](const std::string& k, const std::string& valueObj) {
        int ch = -1;
        try
        {
            ch = std::stoi(k);
        }
        catch (...)
        {
            err = "invalid channel key: " + k;
            return false;
        }
        if (ch < 0 || ch > 15)
        {
            err = "channel key out of range: " + k;
            return false;
        }
        ChannelConfig chCfg = (*table)[ch];
        if (!ParseChannelObject(valueObj, chCfg, err))
        {
            err = "channel " + k + ": " + err;
            return false;
        }
        (*table)[ch] = chCfg;
        return true;
        }, err))
    {
        return false;
    }

    cfg.channelConfigs = table;
    return true;
}

bool ParseChannelMixObject(const std::string& mixObjText, ChannelMixState& mix, std::string& err)
{
    if (auto v = ReadJsonBool(mixObjText, "mute")) mix.mute = *v;
    if (auto v = ReadJsonBool(mixObjText, "solo")) mix.solo = *v;
    if (auto v = ReadJsonDouble(mixObjText, "level")) mix.level = *v;
    if (auto v = ReadJsonDouble(mixObjText, "pan")) mix.pan = *v;
    if (auto v = ReadJsonDouble(mixObjText, "gain")) mix.gain = *v;

    if (mix.level < 0.0 || mix.level > 2.0)
    {
        err = "level must be in range 0.0..2.0";
        return false;
    }
    if (mix.pan < -1.0 || mix.pan > 1.0)
    {
        err = "pan must be in range -1.0..1.0";
        return false;
    }
    if (mix.gain < 0.0 || mix.gain > 4.0)
    {
        err = "gain must be in range 0.0..4.0";
        return false;
    }
    return true;
}

bool LoadChannelMixDiff(const std::string& text, AppConfig& cfg, std::string& err)
{
    std::string mixObj;
    bool found = false;
    if (!ExtractObjectForKey(text, "channelMix", mixObj, found, err))
    {
        return false;
    }
    if (!found)
    {
        return true;
    }

    auto table = MakeMutableChannelMixStates(cfg);
    if (!ParseTopLevelObjectEntries(mixObj, [&](const std::string& k, const std::string& valueObj) {
        int ch = -1;
        try
        {
            ch = std::stoi(k);
        }
        catch (...)
        {
            err = "invalid channelMix key: " + k;
            return false;
        }
        if (ch < 0 || ch > 15)
        {
            err = "channelMix key out of range: " + k;
            return false;
        }
        ChannelMixState mix = (*table)[ch];
        if (!ParseChannelMixObject(valueObj, mix, err))
        {
            err = "channelMix " + k + ": " + err;
            return false;
        }
        (*table)[ch] = mix;
        return true;
        }, err))
    {
        return false;
    }

    cfg.channelMixStates = table;
    return true;
}
} // namespace

bool LoadConfigFileInternal(const std::filesystem::path& configPath, AppConfig& cfg, std::string& err)
{
    const std::string text = ReadTextFile(configPath);
    if (text.empty())
    {
        err = "failed to read config file";
        return false;
    }

    const std::filesystem::path baseDir = configPath.has_parent_path()
        ? configPath.parent_path()
        : std::filesystem::current_path();

    if (auto v = ReadJsonString(text, "midiPath"))
    {
        cfg.midiPath = ResolvePathFromBase(baseDir, *v);
    }
    if (auto v = ReadJsonString(text, "wavPath"))
    {
        cfg.wavPath = ResolvePathFromBase(baseDir, *v);
    }
    if (auto v = ReadJsonInt(text, "targetChannel"))
    {
        cfg.targetChannel = *v;
    }
    if (auto v = ReadJsonInt(text, "initialSeconds"))
    {
        cfg.initialSeconds = *v;
    }
    if (auto v = ReadJsonInt(text, "bits"))
    {
        cfg.bits = *v;
    }
    if (auto v = ReadJsonInt(text, "sampleRate"))
    {
        cfg.sampleRate = *v;
    }
    if (auto v = ReadJsonDouble(text, "extraReleaseSec"))
    {
        cfg.extraReleaseSec = *v;
    }
    if (auto v = ReadJsonString(text, "defaultWave"))
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
} // namespace config::internal
