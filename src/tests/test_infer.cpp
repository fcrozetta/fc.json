#include <doctest/doctest.h>

#include <string>

#include "../parse/parser.hpp"
#include "../schema/infer.hpp"

using namespace fc;

namespace {

const Schema* schemaByName(const SchemaModule& m, const std::string& name) {
    for (const Schema& s : m.schemas) {
        if (s.name == name) return &s;
    }
    return nullptr;
}

const Field* fieldByKey(const Schema& s, const std::string& key) {
    for (const Field& f : s.fields) {
        if (f.key == key) return &f;
    }
    return nullptr;
}

SchemaModule infer(const std::string& json) { return inferSchema(parse(json)); }

} // namespace

TEST_CASE("root object becomes a Root schema with typed fields") {
    SchemaModule m = infer(R"({"i":1,"f":1.5,"s":"x","b":true,"z":null})");
    REQUIRE(m.root.kind == TypeKind::Object);
    const Schema* root = schemaByName(m, "Root");
    REQUIRE(root != nullptr);
    CHECK(fieldByKey(*root, "i")->type.kind == TypeKind::Int);
    CHECK(fieldByKey(*root, "f")->type.kind == TypeKind::Float);
    CHECK(fieldByKey(*root, "s")->type.kind == TypeKind::String);
    CHECK(fieldByKey(*root, "b")->type.kind == TypeKind::Bool);
    CHECK(fieldByKey(*root, "z")->type.kind == TypeKind::Null);
}

TEST_CASE("nested object is named from its key") {
    SchemaModule m = infer(R"({"address":{"city":"x"}})");
    REQUIRE(schemaByName(m, "Address") != nullptr);
    const Field* addr = fieldByKey(*schemaByName(m, "Root"), "address");
    REQUIRE(addr->type.kind == TypeKind::Object);
    CHECK(m.at(addr->type.objectSchema).name == "Address");
}

TEST_CASE("array of objects merges fields; missing ones become optional") {
    SchemaModule m = infer(R"({"list":[{"a":1,"b":2},{"a":1,"c":3}]})");

    const Field* list = fieldByKey(*schemaByName(m, "Root"), "list");
    REQUIRE(list->type.kind == TypeKind::List);
    REQUIRE(list->type.args.front().kind == TypeKind::Object);

    // "list" singularizes to "listItem" -> "ListItem"
    const Schema* item = schemaByName(m, "ListItem");
    REQUIRE(item != nullptr);
    CHECK(fieldByKey(*item, "a")->optional == false); // in both
    CHECK(fieldByKey(*item, "b")->optional == true);  // first only
    CHECK(fieldByKey(*item, "c")->optional == true);  // second only
}

TEST_CASE("trailing-s array key singularizes for element schema") {
    SchemaModule m = infer(R"({"users":[{"name":"x"}]})");
    CHECK(schemaByName(m, "User") != nullptr);
}

TEST_CASE("identical array elements dedup to a single schema") {
    SchemaModule m = infer(R"({"items":[{"a":1},{"a":2},{"a":3}]})");
    // Root + one Item schema, not three.
    CHECK(m.schemas.size() == 2);
    CHECK(schemaByName(m, "Item") != nullptr);
}

TEST_CASE("name-first: identical shapes under different keys stay separate") {
    SchemaModule m = infer(R"({"billing":{"x":1},"shipping":{"x":1}})");
    CHECK(schemaByName(m, "Billing") != nullptr);
    CHECK(schemaByName(m, "Shipping") != nullptr);
    CHECK(m.schemas.size() == 3); // Billing, Shipping, Root
}

TEST_CASE("heterogeneous array yields a Union element") {
    SchemaModule m = infer(R"({"d":[1,"x"]})");
    const Field* d = fieldByKey(*schemaByName(m, "Root"), "d");
    REQUIRE(d->type.kind == TypeKind::List);
    CHECK(d->type.args.front().kind == TypeKind::Union);
    CHECK(d->type.args.front().args.size() == 2);
}

TEST_CASE("empty array element type is Unknown") {
    SchemaModule m = infer(R"({"e":[]})");
    const Field* e = fieldByKey(*schemaByName(m, "Root"), "e");
    REQUIRE(e->type.kind == TypeKind::List);
    CHECK(e->type.args.front().kind == TypeKind::Unknown);
}

TEST_CASE("root keeps its name when a field shares the hint (N1)") {
    SchemaModule m = infer(R"({"root":{"x":1},"y":2})");
    REQUIRE(m.root.kind == TypeKind::Object);
    CHECK(m.at(m.root.objectSchema).name == "Root"); // not Root2
}

TEST_CASE("top-level array does not crash and names element RootItem") {
    SchemaModule m = infer(R"([{"a":1}])");
    REQUIRE(m.root.kind == TypeKind::List);
    REQUIRE(m.root.args.front().kind == TypeKind::Object);
    CHECK(m.at(m.root.args.front().objectSchema).name == "RootItem");
}
