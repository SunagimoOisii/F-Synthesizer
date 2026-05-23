#pragma once

#include <ostream>
#include <string>

#include "SynthEngine/ChannelConfig.h"
#include "SynthEngine/SourceConfig.h"

namespace gui
{
WaveType WaveFromIndex(int idx);
int WaveToIndex(WaveType w);
NoiseType NoiseFromIndex(int idx);
int NoiseToIndex(NoiseType n);
int SourceTypeIndex(const SourceConfig& src);
bool ChannelConfigEquals(const ChannelConfig& a, const ChannelConfig& b);
bool ChannelMixStateEquals(const ChannelMixState& a, const ChannelMixState& b);
void WriteJSONEscaped(std::ostream& out, const std::string& s);
} // namespace gui
