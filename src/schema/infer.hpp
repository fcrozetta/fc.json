#pragma once

// Schema inference: ValueIR -> SchemaIR. Pure; operates only on the IR (no
// * rapidjson). Merge semantics and naming are specified in ADR 0001.

#include "../ir/node.hpp"
#include "../ir/schema.hpp"

namespace fc {

SchemaModule inferSchema(const Tree& tree);

} // namespace fc
