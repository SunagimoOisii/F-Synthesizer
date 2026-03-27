#include "Internal.h"

#include "../ConfigFileInternal.h"

#include "config/SourceRegistry.h"

namespace config::internal::load
{
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
    if (!ValidateLifecycleContract(sourceObjText, sourceKind, err))
    {
        return false;
    }
    if (!ValidateSmoothingSupport(sourceObjText, sourceKind, err))
    {
        return false;
    }

    switch (sourceKind)
    {
    case SourceKind::Waveform: return ParseWaveformSource(sourceObjText, outSource, err);
    case SourceKind::Analog: return ParseAnalogSource(sourceObjText, outSource, err);
    case SourceKind::Noise: return ParseNoiseSource(sourceObjText, outSource, err);
    case SourceKind::Fm: return ParseFmSource(sourceObjText, outSource, err);
    case SourceKind::Drum: return ParseDrumSource(sourceObjText, outSource, err);
    case SourceKind::DrumKit: return ParseDrumKitSource(sourceObjText, outSource, err);
    case SourceKind::Psg: return ParsePsgSource(sourceObjText, outSource, err);
    case SourceKind::Count:
        break;
    }

    err = "unknown source.type: " + *type;
    return false;
}
} // namespace config::internal::load
