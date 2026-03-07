#include "Internal.h"

#include "../ConfigFileInternal.h"

namespace config::internal::load
{
namespace
{
bool ParseLfo1Object(const std::string& text, LfoConfig& lfo, std::string& err)
{
    if (auto v = ReadJSONString(text, "wave"))
    {
        LfoWave wave{};
        if (!TryParseLfoWave(*v, wave))
        {
            err = "invalid modulation.lfo1.wave: " + *v;
            return false;
        }
        lfo.wave = wave;
    }
    if (auto v = ReadJSONDouble(text, "rateHz")) lfo.rateHz = *v;
    if (auto v = ReadJSONDouble(text, "depth")) lfo.depth = *v;
    if (auto v = ReadJSONBool(text, "bipolar")) lfo.bipolar = *v;
    return true;
}

bool ParseEnv2Object(const std::string& text, ModEnvelopeConfig& env2)
{
    if (auto v = ReadJSONDouble(text, "attackSec")) env2.attackSec = *v;
    if (auto v = ReadJSONDouble(text, "decaySec")) env2.decaySec = *v;
    if (auto v = ReadJSONDouble(text, "sustainLevel")) env2.sustainLevel = *v;
    if (auto v = ReadJSONDouble(text, "releaseSec")) env2.releaseSec = *v;
    return true;
}

bool ParseRouteObject(const std::string& text, ModRoute& route, std::string& err)
{
    if (auto v = ReadJSONString(text, "source"))
    {
        ModSource source{};
        if (!TryParseModSource(*v, source))
        {
            err = "invalid modulation.route.source: " + *v;
            return false;
        }
        route.source = source;
    }
    if (auto v = ReadJSONString(text, "destination"))
    {
        ModDestination destination{};
        if (!TryParseModDestination(*v, destination))
        {
            err = "invalid modulation.route.destination: " + *v;
            return false;
        }
        route.destination = destination;
    }
    if (auto v = ReadJSONDouble(text, "amount")) route.amount = *v;
    if (auto v = ReadJSONBool(text, "enabled")) route.enabled = *v;
    return true;
}
} // namespace

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

bool ParseWaveformSmoothingObject(const std::string& text, WaveformConfig::SmoothingConfig& smoothing)
{
    if (auto v = ReadJSONBool(text, "enabled")) smoothing.enabled = *v;
    if (auto v = ReadJSONBool(text, "pitchEnabled")) smoothing.pitchEnabled = *v;
    if (auto v = ReadJSONDouble(text, "ampTimeMs")) smoothing.ampTimeMs = *v;
    if (auto v = ReadJSONDouble(text, "pitchTimeMs")) smoothing.pitchTimeMs = *v;
    if (auto v = ReadJSONDouble(text, "filterCutoffTimeMs")) smoothing.filterCutoffTimeMs = *v;
    return true;
}

bool ValidateModulation(
    const ModulationConfig& modulation,
    bool allowFmIndexDestination,
    const char* contextPrefix,
    std::string& err)
{
    const std::string prefix = (contextPrefix != nullptr && contextPrefix[0] != '\0')
        ? std::string(contextPrefix)
        : std::string("modulation");
    if (modulation.lfo1.rateHz < 0.0 || modulation.lfo1.rateHz > 100.0)
    {
        err = prefix + ".lfo1.rateHz must be in range 0.0..100.0";
        return false;
    }
    if (modulation.lfo1.depth < 0.0 || modulation.lfo1.depth > 1.0)
    {
        err = prefix + ".lfo1.depth must be in range 0.0..1.0";
        return false;
    }
    if (modulation.env2.attackSec < 0.0 || modulation.env2.decaySec < 0.0 || modulation.env2.releaseSec < 0.0)
    {
        err = prefix + ".env2 attack/decay/release must be >= 0.0";
        return false;
    }
    if (modulation.env2.sustainLevel < 0.0 || modulation.env2.sustainLevel > 1.0)
    {
        err = prefix + ".env2.sustainLevel must be in range 0.0..1.0";
        return false;
    }
    for (size_t i = 0; i < modulation.matrix.routes.size(); i++)
    {
        const ModRoute& route = modulation.matrix.routes[i];
        if (route.destination == ModDestination::FmIndex && !allowFmIndexDestination)
        {
            err = prefix + ".routes[" + std::to_string(i) + "].destination 'fm.index' is not allowed for this source type";
            return false;
        }
        if (route.amount < -1.0 || route.amount > 1.0)
        {
            err = prefix + ".routes[" + std::to_string(i) + "].amount must be in range -1.0..1.0";
            return false;
        }
    }
    return true;
}

bool ValidateWaveformSmoothing(const WaveformConfig::SmoothingConfig& smoothing, std::string& err)
{
    if (smoothing.ampTimeMs < 0.0 || smoothing.ampTimeMs > 1000.0)
    {
        err = "waveform.smoothing.ampTimeMs must be in range 0.0..1000.0";
        return false;
    }
    if (smoothing.pitchTimeMs < 0.0 || smoothing.pitchTimeMs > 1000.0)
    {
        err = "waveform.smoothing.pitchTimeMs must be in range 0.0..1000.0";
        return false;
    }
    if (smoothing.filterCutoffTimeMs < 0.0 || smoothing.filterCutoffTimeMs > 1000.0)
    {
        err = "waveform.smoothing.filterCutoffTimeMs must be in range 0.0..1000.0";
        return false;
    }
    return true;
}
} // namespace config::internal::load
