#pragma once

#include <ostream>
#include <string>

#include "SynthEngine/InstrumentSoundConfig.h"
#include "SynthEngine/SourceConfig.h"

namespace gui
{
WaveType WaveFromIndex(int idx);
int WaveToIndex(WaveType w);
NoiseType NoiseFromIndex(int idx);
int NoiseToIndex(NoiseType n);
int SourceTypeIndex(const SourceConfig& src);
bool InstrumentSoundConfigEquals(const InstrumentSoundConfig& a, const InstrumentSoundConfig& b);
bool ChannelMixStateEquals(const ChannelMixState& a, const ChannelMixState& b);
void WriteJSONEscaped(std::ostream& out, const std::string& s);
} // namespace gui
