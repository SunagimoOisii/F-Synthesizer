#include "config/SourceJson.h"

#include "ConfigFileInternal.h"

namespace config
{
void WriteSourceJson(std::ostream& out, const SourceConfig& src, int indent)
{
    internal::WriteSourceConfig(out, src, indent);
}
} // namespace config
