#pragma once

#include <ostream>
#include <string>

#include "SynthEngine/SynthEngine.h"

namespace gui
{
WaveType WaveFromIndex(int idx);
int WaveToIndex(WaveType w);
NoiseType NoiseFromIndex(int idx);
int NoiseToIndex(NoiseType n);
int SourceTypeIndex(const SourceConfig& src);
SourceConfig DefaultSourceByType(int idx);
bool ChannelConfigEquals(const ChannelConfig& a, const ChannelConfig& b);
bool ChannelMixStateEquals(const ChannelMixState& a, const ChannelMixState& b);
void WriteJsonEscaped(std::ostream& out, const std::string& s);
std::string WaveToText(WaveType w);
std::string NoiseToText(NoiseType n);
std::string DrumTypeToText(DrumType d);
void WriteSourceJson(std::ostream& out, const SourceConfig& src, int indent);
} // namespace gui
