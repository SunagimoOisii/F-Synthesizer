#include "Internal.h"

#include <algorithm>

#include "../ConfigFileInternal.h"

namespace config::internal::load
{
bool ParseWaveformSource(const std::string& sourceObjText, SourceConfig& outSource, std::string& err)
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
    if (!ParseWaveformCommonFields(sourceObjText, wf, err)) { return false; }
    if (!ValidateWaveformBySchema(wf, err))
    {
        return false;
    }
    if (!ValidateModulation(wf.modulation, false, "waveform.modulation", err))
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

bool ParseAnalogSource(const std::string& sourceObjText, SourceConfig& outSource, std::string& err)
{
    auto wave = ReadJSONString(sourceObjText, "wave");
    if (!wave)
    {
        err = "analog source requires 'wave'";
        return false;
    }
    WaveType w{};
    if (!TryParseWaveType(*wave, w))
    {
        err = "invalid wave: " + *wave;
        return false;
    }
    AnalogConfig analog{};
    analog.wave = w;
    if (!ParseAnalogCommonFields(sourceObjText, analog, err)) { return false; }
    if (auto v = ReadJSONDouble(sourceObjText, "driftDepthCents"))
    {
        analog.driftDepthCents = *v;
    }
    if (auto v = ReadJSONDouble(sourceObjText, "driftRateHz"))
    {
        analog.driftRateHz = *v;
    }
    if (!ValidateAnalogBySchema(analog, err))
    {
        return false;
    }
    if (!ValidateModulation(analog.modulation, false, "analog.modulation", err))
    {
        return false;
    }
    if (!ValidateWaveformSmoothing(analog.smoothing, err))
    {
        return false;
    }
    outSource = analog;
    return true;
}
} // namespace config::internal::load
