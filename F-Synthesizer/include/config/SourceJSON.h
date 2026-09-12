#pragma once
#include "SynthEngine/InstrumentSoundConfig.h"
#include "third_party/nlohmann/json.hpp"

namespace config
{
// Build parsed JSON values directly; reject non-finite sound parameters before saving.
nlohmann::json SoundToJSON(const InstrumentSoundConfig& sound);
}
