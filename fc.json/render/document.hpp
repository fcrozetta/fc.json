#pragma once

// Document renderers: emit a JSON document that mirrors the input structure,
// * substituting leaf values. Both are pure walks over the ValueIR (no schema
// * inference needed) and share one emitter. See ADR 0001.
//   - example: each value becomes a type default (int 0, float 1.0, "string", ...)
//   - redact:  each value becomes "***"

#include <string>

#include "../ir/node.hpp"

namespace fc {

std::string renderExample(const Tree& tree);
std::string renderRedacted(const Tree& tree);

} // namespace fc
