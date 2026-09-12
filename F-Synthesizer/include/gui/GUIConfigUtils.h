#pragma once
#include "SynthEngine/SourceConfig.h"

namespace gui
{
WaveType WaveFromIndex(int idx);
int WaveToIndex(WaveType w);
NoiseType NoiseFromIndex(int idx);
int NoiseToIndex(NoiseType n);
} // namespace gui
