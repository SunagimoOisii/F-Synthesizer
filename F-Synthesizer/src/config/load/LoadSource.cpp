#include "Internal.h"

#include "../ConfigFileInternal.h"

#include "config/SourceRegistry.h"

namespace config::internal::load
{
namespace
{
bool ParseDrumConfigObject(const std::string& text, DrumConfig& drum, std::string& err)
{
    if (auto t = ReadJSONString(text, "drumType"))
    {
        DrumType dt{};
        if (!TryParseDrumType(*t, dt))
        {
            err = "invalid drumType: " + *t;
            return false;
        }
        drum.type = dt;
    }
    if (auto v = ReadJSONDouble(text, "gain")) drum.gain = *v;
    if (auto v = ReadJSONDouble(text, "baseFreq")) drum.baseFreq = *v;
    if (auto v = ReadJSONDouble(text, "pitchDrop")) drum.pitchDrop = *v;
    if (auto v = ReadJSONDouble(text, "pitchDecaySec")) drum.pitchDecaySec = *v;
    if (auto v = ReadJSONDouble(text, "toneFreq")) drum.toneFreq = *v;
    if (auto v = ReadJSONDouble(text, "toneLevel")) drum.toneLevel = *v;
    if (auto v = ReadJSONDouble(text, "noiseLevel")) drum.noiseLevel = *v;
    if (auto v = ReadJSONDouble(text, "hpCut")) drum.hpCut = *v;
    if (auto v = ReadJSONDouble(text, "lpCut")) drum.lpCut = *v;
    if (auto v = ReadJSONString(text, "toneWave"))
    {
        WaveType w{};
        if (!TryParseWaveType(*v, w))
        {
            err = "invalid toneWave: " + *v;
            return false;
        }
        drum.toneWave = (int)w;
    }
    if (auto v = ReadJSONString(text, "noiseType"))
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
} // namespace

bool ParseSourceObject(const std::string& sourceObjText, SourceConfig& outSource, std::string& err)
{
    const auto type = ReadJSONString(sourceObjText, "type");
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
        auto wave = ReadJSONString(sourceObjText, "wave");
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
        if (auto v = ReadJSONInt(sourceObjText, "unisonVoices"))
        {
            wf.unisonVoices = *v;
        }
        if (auto v = ReadJSONDouble(sourceObjText, "unisonDetuneCents"))
        {
            wf.unisonDetuneCents = *v;
        }
        if (auto v = ReadJSONDouble(sourceObjText, "unisonSpread"))
        {
            wf.unisonSpread = *v;
        }
        if (auto v = ReadJSONDouble(sourceObjText, "subOscLevel"))
        {
            wf.subOscLevel = *v;
        }
        if (auto v = ReadJSONString(sourceObjText, "filterMode"))
        {
            FilterMode mode{};
            if (!TryParseFilterMode(*v, mode))
            {
                err = "invalid waveform.filterMode: " + *v;
                return false;
            }
            wf.filterMode = mode;
        }
        if (auto v = ReadJSONDouble(sourceObjText, "filterCutoffHz"))
        {
            wf.filterCutoffHz = *v;
        }
        if (auto v = ReadJSONDouble(sourceObjText, "filterResonance"))
        {
            wf.filterResonance = *v;
        }
        std::string smoothingObj;
        bool foundSmoothing = false;
        if (!ExtractObjectForKey(sourceObjText, "smoothing", smoothingObj, foundSmoothing, err))
        {
            return false;
        }
        if (foundSmoothing)
        {
            if (!ParseWaveformSmoothingObject(smoothingObj, wf.smoothing))
            {
                err = "invalid waveform.smoothing object";
                return false;
            }
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
        if (!ValidateWaveformSmoothing(wf.smoothing, err))
        {
            return false;
        }
        outSource = wf;
        return true;
    }
    case SourceKind::Noise:
    {
        auto noise = ReadJSONString(sourceObjText, "noise");
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
        auto carrier = ReadJSONString(sourceObjText, "carrierWave");
        auto mod = ReadJSONString(sourceObjText, "modWave");
        auto carrierRatio = ReadJSONDouble(sourceObjText, "carrierRatio");
        auto modRatio = ReadJSONDouble(sourceObjText, "modRatio");
        auto index = ReadJSONDouble(sourceObjText, "index");
        auto outLevel = ReadJSONDouble(sourceObjText, "outLevel");
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
} // namespace config::internal::load
