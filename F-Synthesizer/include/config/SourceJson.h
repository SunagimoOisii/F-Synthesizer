#pragma once

#include <iosfwd>

#include "SynthEngine/SynthEngine.h"

namespace config
{
void WriteSourceJson(std::ostream& out, const SourceConfig& src, int indent);
} // namespace config
