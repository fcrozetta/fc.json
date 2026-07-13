#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>

#include <cstdint>
#include <string>

#include "../parse/parser.hpp"

using namespace fc;

namespace {

// Find an object member by key. Returns its NodeId or throws if absent.
NodeId childByKey(const Tree& tree, NodeId parent, const std::string& key) {
    for (NodeId id : tree.at(parent).children) {
        if (tree.at(id).key == key) return id;
    }
    FAIL("no child with key '", key, "'");
    return 0;
}

} // namespace

TEST_CASE("primitives get distinct kinds and retained values") {
    Tree t = parse(R"({"i":42,"f":3.14,"s":"hi","b":true,"z":null})");

    CHECK(t.at(t.root).kind == NodeKind::Object);
    CHECK(t.at(t.root).children.size() == 5);

    const Node& i = t.at(childByKey(t, t.root, "i"));
    CHECK(i.kind == NodeKind::Int);
    CHECK(std::get<std::int64_t>(i.scalar) == 42);

    const Node& f = t.at(childByKey(t, t.root, "f"));
    CHECK(f.kind == NodeKind::Float);
    CHECK(std::get<double>(f.scalar) == doctest::Approx(3.14));

    const Node& s = t.at(childByKey(t, t.root, "s"));
    CHECK(s.kind == NodeKind::String);
    CHECK(std::get<std::string>(s.scalar) == "hi");

    const Node& b = t.at(childByKey(t, t.root, "b"));
    CHECK(b.kind == NodeKind::Bool);
    CHECK(std::get<bool>(b.scalar) == true);

    CHECK(t.at(childByKey(t, t.root, "z")).kind == NodeKind::Null);
}

TEST_CASE("integer vs float is decided at the boundary") {
    CHECK(parse("1").at(0).kind == NodeKind::Int);
    CHECK(parse("1.0").at(0).kind == NodeKind::Float);
    CHECK(parse("-7").at(0).kind == NodeKind::Int);
    CHECK(parse("1e3").at(0).kind == NodeKind::Float);
}

TEST_CASE("nested objects preserve keys and structure") {
    Tree t = parse(R"({"a":{"b":1}})");
    NodeId a = childByKey(t, t.root, "a");
    CHECK(t.at(a).kind == NodeKind::Object);
    NodeId b = childByKey(t, a, "b");
    CHECK(t.at(b).kind == NodeKind::Int);
    CHECK(std::get<std::int64_t>(t.at(b).scalar) == 1);
}

TEST_CASE("top-level array does not crash (old assert bug)") {
    Tree t = parse("[1,2,3]");
    CHECK(t.at(t.root).kind == NodeKind::Array);
    CHECK(t.at(t.root).children.size() == 3);
    for (NodeId id : t.at(t.root).children) {
        CHECK(t.at(id).kind == NodeKind::Int);
        CHECK(t.at(id).key.empty()); // array elements are unnamed
    }
}

TEST_CASE("object inside an array is unnamed but keeps its fields") {
    Tree t = parse(R"({"a":[{"b":10}]})");
    NodeId a = childByKey(t, t.root, "a");
    CHECK(t.at(a).kind == NodeKind::Array);
    REQUIRE(t.at(a).children.size() == 1);

    NodeId elem = t.at(a).children[0];
    CHECK(t.at(elem).kind == NodeKind::Object);
    CHECK(t.at(elem).key.empty());

    NodeId b = childByKey(t, elem, "b");
    CHECK(t.at(b).kind == NodeKind::Int);
    CHECK(std::get<std::int64_t>(t.at(b).scalar) == 10);
}

TEST_CASE("empty object and empty array") {
    CHECK(parse("{}").at(0).children.empty());
    CHECK(parse("[]").at(0).children.empty());
}

TEST_CASE("malformed JSON throws ParseError") {
    CHECK_THROWS_AS(parse("{bad"), ParseError);
    CHECK_THROWS_AS(parse(""), ParseError);
}
