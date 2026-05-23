#pragma once

#include <variant>

#include "SynthEngine/AnalogConfig.h"
#include "SynthEngine/DrumConfig.h"
#include "SynthEngine/DrumKitConfig.h"
#include "SynthEngine/FmConfig.h"
#include "SynthEngine/NoiseConfig.h"
#include "SynthEngine/PsgConfig.h"
#include "SynthEngine/WaveformConfig.h"

using SourceConfig = std::variant<WaveformConfig, AnalogConfig, NoiseConfig, FmConfig, DrumConfig, DrumKitConfig, PsgConfig>;
