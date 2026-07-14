#include <doctest/doctest.h>

#include <string>

#include "../parse/parser.hpp"
#include "../render/document.hpp"

using namespace fc;

namespace {

std::string example(const std::string& json) { return renderExample(parse(json)); }
std::string redact(const std::string& json) { return renderRedacted(parse(json)); }

bool has(const std::string& hay, const std::string& needle) {
    return hay.find(needle) != std::string::npos;
}

} // namespace

TEST_CASE("example substitutes type-illustrative defaults per leaf") {
    std::string out = example(R"({"i":42,"f":3.14,"s":"hi","b":false,"z":null})");
    CHECK(has(out, R"("i": 0)"));
    CHECK(has(out, R"("f": 1.0)"));
    CHECK(has(out, R"("s": "string")"));
    CHECK(has(out, R"("b": true)"));
    CHECK(has(out, R"("z": null)"));
}

TEST_CASE("redact masks every leaf with asterisks, keeps structure") {
    std::string out = redact(R"({"i":42,"s":"secret","nested":{"k":"v"}})");
    CHECK(has(out, R"("i": "***")"));
    CHECK(has(out, R"("s": "***")"));
    CHECK(has(out, R"("k": "***")"));
    CHECK(has(out, R"("nested": {)")); // structure preserved
}

TEST_CASE("both mirror structure: keys, nesting, array element count") {
    // Two array elements in, two out (faithful mirror, not collapsed).
    std::string ex = example(R"({"xs":[1,2,3]})");
    CHECK(has(ex, R"("xs": [)"));
    // three zeros
    CHECK(ex.find("0") != std::string::npos);
    std::string rd = redact(R"({"xs":[1,2]})");
    // two masked entries
    std::size_t first = rd.find("\"***\"");
    std::size_t second = rd.find("\"***\"", first + 1);
    CHECK(second != std::string::npos);
}

TEST_CASE("empty object and array round-trip") {
    CHECK(has(example(R"({"o":{},"a":[]})"), R"("o": {})"));
    CHECK(has(example(R"({"o":{},"a":[]})"), R"("a": [])"));
}

TEST_CASE("raw keys are preserved verbatim (no sanitization)") {
    CHECK(has(redact(R"({"a b":1})"), R"("a b": "***")"));
}

TEST_CASE("top-level array is handled") {
    CHECK(has(example("[1,2]"), "["));
    CHECK(has(redact(R"(["x","y"])"), R"("***")"));
}
