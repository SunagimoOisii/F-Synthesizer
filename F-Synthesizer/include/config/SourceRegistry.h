#pragma once

#include <string_view>

#include "SynthEngine/SynthEngine.h"

namespace config
{
enum class SourceKind
{
    Waveform = 0,
    Noise = 1,
    Fm = 2,
    Drum = 3,
    DrumKit = 4,
    Count
};

constexpr int kSourceKindCount = static_cast<int>(SourceKind::Count);

bool TryParseSourceKind(std::string_view typeName, SourceKind& outKind);
const char* SourceKindToTypeName(SourceKind kind);
const char* SourceKindToDisplayName(SourceKind kind);
SourceKind SourceKindFromIndex(int index);
int SourceKindToIndex(SourceKind kind);

SourceKind SourceConfigKind(const SourceConfig& src);
SourceConfig DefaultSourceConfig(SourceKind kind);
} // namespace config
