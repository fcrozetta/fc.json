#include <doctest/doctest.h>

#include <string>

#include "../parse/parser.hpp"
#include "../render/jsonschema.hpp"
#include "../schema/infer.hpp"

using namespace fc;

namespace {

std::string js(const std::string& json) {
    return renderJsonSchema(inferSchema(parse(json)));
}

bool has(const std::string& hay, const std::string& needle) {
    return hay.find(needle) != std::string::npos;
}

} // namespace

TEST_CASE("emits 2020-12 dialect, $defs and a root $ref") {
    std::string out = js(R"({"i":1})");
    CHECK(has(out, R"("$schema": "https://json-schema.org/draft/2020-12/schema")"));
    CHECK(has(out, R"("$defs")"));
    CHECK(has(out, R"("Root":)"));
    CHECK(has(out, R"("$ref": "#/$defs/Root")"));
}

TEST_CASE("scalar types map to JSON Schema types") {
    std::string out = js(R"({"i":1,"f":1.5,"s":"x","b":true,"z":null})");
    CHECK(has(out, R"("i": { "type": "integer" })"));
    CHECK(has(out, R"("f": { "type": "number" })"));
    CHECK(has(out, R"("s": { "type": "string" })"));
    CHECK(has(out, R"("b": { "type": "boolean" })"));
    CHECK(has(out, R"("z": { "type": "null" })"));
}

TEST_CASE("required lists non-optional fields only") {
    std::string out = js(R"({"list":[{"a":1,"b":2},{"a":1}]})");
    // ListItem: a required, b optional
    CHECK(has(out, R"("required": ["a"])"));
    CHECK_FALSE(has(out, R"("required": ["a", "b"])"));
}

TEST_CASE("nested object becomes a $ref into $defs") {
    std::string out = js(R"({"address":{"city":"x"}})");
    CHECK(has(out, R"("Address":)"));
    CHECK(has(out, R"("address": { "$ref": "#/$defs/Address" })"));
}

TEST_CASE("array renders items; heterogeneous renders anyOf") {
    CHECK(has(js(R"({"xs":[1,2]})"), R"("type": "array", "items": { "type": "integer" })"));
    std::string u = js(R"({"d":[1,"x"]})");
    CHECK(has(u, R"("anyOf")"));
}

TEST_CASE("property names are kept raw, not sanitized (unlike pydantic)") {
    // Key with a space is a valid JSON Schema property name verbatim; no alias.
    std::string out = js(R"({"a b":1})");
    CHECK(has(out, R"("a b": { "type": "integer" })"));
    CHECK_FALSE(has(out, "alias"));
    CHECK_FALSE(has(out, "a_b"));
}

TEST_CASE("top-level array emits an array root, not a $ref") {
    std::string out = js(R"([{"a":1}])");
    CHECK(has(out, R"("type": "array")"));
    CHECK(has(out, R"("items": { "$ref": "#/$defs/RootItem" })"));
    CHECK(has(out, R"("RootItem":)"));
}
