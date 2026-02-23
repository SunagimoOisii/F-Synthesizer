#include "ConfigFileInternal.h"

#include <string>

#include "io/PlatformPaths.h"
#include "config/SourceRegistry.h"

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

bool ParseLfo1Object(const std::string& text, LfoConfig& lfo, std::string& err)
{
    if (auto v = ReadJsonString(text, "wave"))
    {
        LfoWave wave{};
        if (!TryParseLfoWave(*v, wave))
        {
            err = "invalid modulation.lfo1.wave: " + *v;
            return false;
        }
        lfo.wave = wave;
    }
    if (auto v = ReadJsonDouble(text, "rateHz")) lfo.rateHz = *v;
    if (auto v = ReadJsonDouble(text, "depth")) lfo.depth = *v;
    if (auto v = ReadJsonBool(text, "bipolar")) lfo.bipolar = *v;
    return true;
}

bool ParseEnv2Object(const std::string& text, ModEnvelopeConfig& env2)
{
    if (auto v = ReadJsonDouble(text, "attackSec")) env2.attackSec = *v;
    if (auto v = ReadJsonDouble(text, "decaySec")) env2.decaySec = *v;
    if (auto v = ReadJsonDouble(text, "sustainLevel")) env2.sustainLevel = *v;
    if (auto v = ReadJsonDouble(text, "releaseSec")) env2.releaseSec = *v;
    return true;
}

bool ParseRouteObject(const std::string& text, ModRoute& route, std::string& err)
{
    if (auto v = ReadJsonString(text, "source"))
    {
        ModSource source{};
        if (!TryParseModSource(*v, source))
        {
            err = "invalid modulation.route.source: " + *v;
            return false;
        }
        route.source = source;
    }
    if (auto v = ReadJsonString(text, "destination"))
    {
        ModDestination destination{};
        if (!TryParseModDestination(*v, destination))
        {
            err = "invalid modulation.route.destination: " + *v;
            return false;
        }
        route.destination = destination;
    }
    if (auto v = ReadJsonDouble(text, "amount")) route.amount = *v;
    if (auto v = ReadJsonBool(text, "enabled")) route.enabled = *v;
    return true;
}

bool ParseModulationObject(const std::string& text, ModulationConfig& modulation, std::string& err)
{
    std::string lfo1Obj;
    bool foundLfo1 = false;
    if (!ExtractObjectForKey(text, "lfo1", lfo1Obj, foundLfo1, err))
    {
        return false;
    }
    if (foundLfo1 && !ParseLfo1Object(lfo1Obj, modulation.lfo1, err))
    {
        return false;
    }

    std::string env2Obj;
    bool foundEnv2 = false;
    if (!ExtractObjectForKey(text, "env2", env2Obj, foundEnv2, err))
    {
        return false;
    }
    if (foundEnv2 && !ParseEnv2Object(env2Obj, modulation.env2))
    {
        return false;
    }

    std::string routesObj;
    bool foundRoutes = false;
    if (!ExtractObjectForKey(text, "routes", routesObj, foundRoutes, err))
    {
        return false;
    }
    if (foundRoutes)
    {
        if (!ParseTopLevelObjectEntries(routesObj, [&](const std::string& k, const std::string& valueObj) {
            int index = -1;
            try
            {
                index = std::stoi(k);
            }
            catch (...)
            {
                err = "invalid modulation route key: " + k;
                return false;
            }
            if (index < 0 || index >= static_cast<int>(modulation.matrix.routes.size()))
            {
                err = "modulation route key out of range: " + k;
                return false;
            }
            ModRoute route = modulation.matrix.routes[static_cast<size_t>(index)];
            if (!ParseRouteObject(valueObj, route, err))
            {
                return false;
            }
            modulation.matrix.routes[static_cast<size_t>(index)] = route;
            return true;
            }, err))
        {
            return false;
        }
    }
    return true;
}

bool ValidateModulation(const ModulationConfig& modulation, std::string& err)
{
    if (modulation.lfo1.rateHz < 0.0 || modulation.lfo1.rateHz > 100.0)
    {
        err = "waveform.modulation.lfo1.rateHz must be in range 0.0..100.0";
        return false;
    }
    if (modulation.lfo1.depth < 0.0 || modulation.lfo1.depth > 1.0)
    {
        err = "waveform.modulation.lfo1.depth must be in range 0.0..1.0";
        return false;
    }
    if (modulation.env2.attackSec < 0.0 || modulation.env2.decaySec < 0.0 || modulation.env2.releaseSec < 0.0)
    {
        err = "waveform.modulation.env2 attack/decay/release must be >= 0.0";
        return false;
    }
    if (modulation.env2.sustainLevel < 0.0 || modulation.env2.sustainLevel > 1.0)
    {
        err = "waveform.modulation.env2.sustainLevel must be in range 0.0..1.0";
        return false;
    }
    for (size_t i = 0; i < modulation.matrix.routes.size(); i++)
    {
        const ModRoute& route = modulation.matrix.routes[i];
        if (route.amount < -1.0 || route.amount > 1.0)
        {
            err = "waveform.modulation.routes[" + std::to_string(i) + "].amount must be in range -1.0..1.0";
            return false;
        }
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

    SourceKind sourceKind{};
    if (!TryParseSourceKind(*type, sourceKind))
    {
        err = "unknown source.type: " + *type;
        return false;
    }

    switch (sourceKind)
    {
    case SourceKind::Waveform:
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
        WaveformConfig wf{};
        wf.wave = w;
        if (auto v = ReadJsonInt(sourceObjText, "unisonVoices"))
        {
            wf.unisonVoices = *v;
        }
        if (auto v = ReadJsonDouble(sourceObjText, "unisonDetuneCents"))
        {
            wf.unisonDetuneCents = *v;
        }
        if (auto v = ReadJsonDouble(sourceObjText, "unisonSpread"))
        {
            wf.unisonSpread = *v;
        }
        if (auto v = ReadJsonDouble(sourceObjText, "subOscLevel"))
        {
            wf.subOscLevel = *v;
        }
        if (auto v = ReadJsonString(sourceObjText, "filterMode"))
        {
            FilterMode mode{};
            if (!TryParseFilterMode(*v, mode))
            {
                err = "invalid waveform.filterMode: " + *v;
                return false;
            }
            wf.filterMode = mode;
        }
        if (auto v = ReadJsonDouble(sourceObjText, "filterCutoffHz"))
        {
            wf.filterCutoffHz = *v;
        }
        if (auto v = ReadJsonDouble(sourceObjText, "filterResonance"))
        {
            wf.filterResonance = *v;
        }
        std::string modulationObj;
        bool foundModulation = false;
        if (!ExtractObjectForKey(sourceObjText, "modulation", modulationObj, foundModulation, err))
        {
            return false;
        }
        if (foundModulation)
        {
            if (!ParseModulationObject(modulationObj, wf.modulation, err))
            {
                return false;
            }
        }
        if (wf.unisonVoices < 1 || wf.unisonVoices > 8)
        {
            err = "waveform.unisonVoices must be in range 1..8";
            return false;
        }
        if (wf.unisonDetuneCents < 0.0 || wf.unisonDetuneCents > 120.0)
        {
            err = "waveform.unisonDetuneCents must be in range 0.0..120.0";
            return false;
        }
        if (wf.unisonSpread < 0.0 || wf.unisonSpread > 1.0)
        {
            err = "waveform.unisonSpread must be in range 0.0..1.0";
            return false;
        }
        if (wf.subOscLevel < 0.0 || wf.subOscLevel > 2.0)
        {
            err = "waveform.subOscLevel must be in range 0.0..2.0";
            return false;
        }
        if (wf.filterCutoffHz < 10.0 || wf.filterCutoffHz > 20000.0)
        {
            err = "waveform.filterCutoffHz must be in range 10.0..20000.0";
            return false;
        }
        if (wf.filterResonance < 0.1 || wf.filterResonance > 18.0)
        {
            err = "waveform.filterResonance must be in range 0.1..18.0";
            return false;
        }
        if (!ValidateModulation(wf.modulation, err))
        {
            return false;
        }
        outSource = wf;
        return true;
    }
    case SourceKind::Noise:
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
    case SourceKind::Fm:
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
    case SourceKind::Drum:
    {
        DrumConfig drum{};
        if (!ParseDrumConfigObject(sourceObjText, drum, err))
        {
            return false;
        }
        outSource = drum;
        return true;
    }
    case SourceKind::DrumKit:
    {
        // drumkit は差分上書き前提のため、未指定noteは None 初期値を維持する。
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
    case SourceKind::Count:
        break;
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

    // 既定値を基底に差分だけを適用し、preset互換を維持する。
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

    // channelMix も channels と同じく「差分マージ」を採用する。
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

    // 相対パスは設定ファイル配置ディレクトリ基準で解決する。
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
} // namespace config::internal
