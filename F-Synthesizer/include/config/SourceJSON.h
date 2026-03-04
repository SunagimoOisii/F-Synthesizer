#pragma once

#include <iosfwd>

#include "SynthEngine/SynthEngine.h"

namespace config
{
// 目的: SourceConfig を "source" オブジェクト形式でJSON出力する。
// 前提: indent は 0 以上を想定。出力先ストリームのエラー処理は呼び出し側で扱う。
void WriteSourceJSON(std::ostream& out, const SourceConfig& src, int indent);
} // namespace config
