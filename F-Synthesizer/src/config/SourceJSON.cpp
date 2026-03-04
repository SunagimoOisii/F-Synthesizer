#include "config/SourceJSON.h"

#include "ConfigFileInternal.h"

namespace config
{
void WriteSourceJSON(std::ostream& out, const SourceConfig& src, int indent)
{
    internal::WriteSourceConfig(out, src, indent);
}
} // namespace config
