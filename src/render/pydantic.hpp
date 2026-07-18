#pragma once

// Pydantic renderer: SchemaIR -> Python source (pydantic BaseModel classes).
// * A pure function over the SchemaIR — no IR mutation, no vendor types.
// * Python-specific identifier sanitization lives here, not in the IR. ADR 0001.

#include <string>

#include "../ir/schema.hpp"

namespace fc {

std::string renderPydantic(const SchemaModule& mod);

} // namespace fc
