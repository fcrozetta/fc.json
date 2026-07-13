#include <doctest/doctest.h>

#include <string>

#include "../parse/parser.hpp"
#include "../render/pydantic.hpp"
#include "../schema/infer.hpp"

using namespace fc;

namespace {

std::string py(const std::string& json) {
    return renderPydantic(inferSchema(parse(json)));
}

bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

} // namespace

TEST_CASE("basic object renders a BaseModel with typed fields") {
    std::string out = py(R"({"i":1,"s":"x"})");
    CHECK(contains(out, "from pydantic import BaseModel"));
    CHECK(contains(out, "class Root(BaseModel):"));
    CHECK(contains(out, "    i: int"));
    CHECK(contains(out, "    s: str"));
}

TEST_CASE("optional field is Optional[...] with a None default") {
    std::string out = py(R"({"list":[{"a":1},{"a":1,"b":2}]})");
    CHECK(contains(out, "from typing import Optional"));
    CHECK(contains(out, "    b: Optional[int] = None"));
}

TEST_CASE("invalid identifiers are sanitized and aliased (the old bug)") {
    // Key with a space -> valid attribute + alias preserving the wire name.
    std::string out = py(R"({"a b":1})");
    CHECK(contains(out, "from pydantic import BaseModel, Field"));
    CHECK(contains(out, R"(    a_b: int = Field(alias="a b"))"));
}

TEST_CASE("python keyword key is suffixed and aliased") {
    std::string out = py(R"({"class":1})");
    CHECK(contains(out, R"(    class_: int = Field(alias="class"))"));
}

TEST_CASE("optional + aliased field combines default and alias") {
    std::string out = py(R"({"list":[{"a":1},{"a":1,"x y":2}]})");
    CHECK(contains(out, R"(x_y: Optional[int] = Field(default=None, alias="x y"))"));
}

TEST_CASE("empty object renders pass") {
    CHECK(contains(py(R"({"o":{}})"), "class O(BaseModel):\n    pass\n"));
}

TEST_CASE("heterogeneous array renders Union") {
    std::string out = py(R"({"d":[1,"x"]})");
    CHECK(contains(out, "from typing import Union"));
    CHECK(contains(out, "d: list[Union[int, str]]"));
}

TEST_CASE("top-level array emits a Root alias, not a class") {
    std::string out = py(R"([{"a":1}])");
    CHECK(contains(out, "class RootItem(BaseModel):"));
    CHECK(contains(out, "Root = list[RootItem]"));
}
