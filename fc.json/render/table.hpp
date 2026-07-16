#pragma once

// Table renderer: SchemaIR -> a path/type overview table.
// * Pure over the SchemaIR. The formatting is done by the tabulate library
// * (confined to table.cpp); this header exposes no vendor type. See ADR 0001.

#include <string>

#include "../ir/schema.hpp"

namespace fc {

std::string renderTable(const SchemaModule& mod);

} // namespace fc
