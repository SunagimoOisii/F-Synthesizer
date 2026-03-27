#include "Internal.h"

#include "../ConfigFileInternal.h"

namespace config::internal::load
{
bool ParseNoiseSource(const std::string& sourceObjText, SourceConfig& outSource, std::string& err)
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
    NoiseConfig nz{};
    nz.noise = n;
    if (auto v = ReadJSONString(sourceObjText, "filterMode"))
    {
        FilterMode mode{};
        if (!TryParseFilterMode(*v, mode))
        {
            err = "invalid noise.filterMode: " + *v;
            return false;
        }
        nz.filterMode = mode;
    }
    if (auto v = ReadJSONDouble(sourceObjText, "filterCutoffHz"))
    {
        nz.filterCutoffHz = *v;
    }
    if (auto v = ReadJSONDouble(sourceObjText, "filterResonance"))
    {
        nz.filterResonance = *v;
    }
    if (!ValidateNoiseBySchema(nz, err))
    {
        return false;
    }
    outSource = nz;
    return true;
}

bool ParsePsgSource(const std::string& sourceObjText, SourceConfig& outSource, std::string& err)
{
    PsgConfig psg{};

    const auto waveStr = ReadJSONString(sourceObjText, "wave");
    if (!waveStr)
    {
        err = "psg source requires 'wave'";
        return false;
    }
    if (*waveStr == "square") psg.wave = PsgWaveType::Square;
    else if (*waveStr == "pulse") psg.wave = PsgWaveType::Pulse;
    else if (*waveStr == "triangle") psg.wave = PsgWaveType::Triangle;
    else if (*waveStr == "noise") psg.wave = PsgWaveType::Noise;
    else
    {
        err = "invalid psg wave: " + *waveStr + " (square/pulse/triangle/noise)";
        return false;
    }

    if (auto v = ReadJSONInt(sourceObjText, "duty"))
    {
        if (*v < 0 || *v > 7) { err = "psg.duty must be 0..7"; return false; }
        psg.duty = *v;
    }
    if (auto v = ReadJSONInt(sourceObjText, "volumeSteps"))
    {
        if (*v < 0 || *v > 15) { err = "psg.volumeSteps must be 0..15"; return false; }
        psg.volumeSteps = *v;
    }
    if (auto v = ReadJSONInt(sourceObjText, "maxVoices"))
    {
        if (*v < 1 || *v > 8) { err = "psg.maxVoices must be 1..8"; return false; }
        psg.maxVoices = *v;
    }

    outSource = psg;
    return true;
}
} // namespace config::internal::load
