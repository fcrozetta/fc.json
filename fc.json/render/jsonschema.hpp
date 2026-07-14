#pragma once

// JSON Schema renderer: SchemaIR -> a JSON Schema (2020-12) document.
// * Pure function over the SchemaIR. Property names are kept raw (JSON Schema
// * allows any string key, so no identifier sanitization here, unlike the
// * Pydantic renderer). No rapidjson: the JSON is hand-built. See ADR 0001.

#include <string>

#include "../ir/schema.hpp"

namespace fc {

std::string renderJsonSchema(const SchemaModule& mod);

} // namespace fc
