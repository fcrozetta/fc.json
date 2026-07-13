#pragma once

// Parse adapter: JSON text -> ValueIR. This header is vendor-agnostic; the
// * only translation unit that includes rapidjson is parser.cpp.

#include <stdexcept>
#include <string>

#include "../ir/node.hpp"

namespace fc {

// Thrown when the input is not well-formed JSON.
struct ParseError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

// Parse JSON text into the ValueIR.
// Accepts any JSON root (object, array, or scalar) — there is no
// "must be an object" precondition. Throws ParseError on malformed input.
Tree parse(const std::string& text);

} // namespace fc
