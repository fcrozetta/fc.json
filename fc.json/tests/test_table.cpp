#include <doctest/doctest.h>

#include <string>

#include "../parse/parser.hpp"
#include "../render/table.hpp"
#include "../schema/infer.hpp"

using namespace fc;

namespace {

std::string table(const std::string& json) {
    return renderTable(inferSchema(parse(json)));
}

bool has(const std::string& hay, const std::string& needle) {
    return hay.find(needle) != std::string::npos;
}

} // namespace

TEST_CASE("table has a header and one row per field") {
    std::string out = table(R"({"a":{"b":"x"},"n":1})");
    CHECK(has(out, "Path"));
    CHECK(has(out, "Type"));
    CHECK(has(out, "n"));   // scalar field
    CHECK(has(out, "int"));
}

TEST_CASE("nested objects expand to dotted paths") {
    std::string out = table(R"({"a":{"b":"x"}})");
    CHECK(has(out, "a.b")); // nested path
    CHECK(has(out, "A"));   // a's type is the class name
    CHECK(has(out, "str"));
}

TEST_CASE("optional fields are marked with ? on the path") {
    std::string out = table(R"({"list":[{"a":1},{"a":1,"b":2}]})");
    CHECK(has(out, "list[].a"));  // element field, list-expanded
    CHECK(has(out, "list[].b?")); // optional element field
}

TEST_CASE("union type is shown as a label") {
    CHECK(has(table(R"({"d":[1,"x"]})"), "list[Union[int, str]]"));
}

TEST_CASE("top-level array shows a (root) row") {
    std::string out = table(R"([{"a":1}])");
    CHECK(has(out, "(root)"));
    CHECK(has(out, "list[RootItem]"));
}
