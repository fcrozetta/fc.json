#pragma once

// The SchemaIR: a merged, typed schema derived from the ValueIR by inferSchema.
// * Vendor-agnostic and render-agnostic — schema renderers (Pydantic,
// * JSON-Schema) read only this. Raw JSON keys are kept verbatim; per-target
// * identifier sanitization is the renderer's job. See ADR 0001.

#include <cstdint>
#include <string>
#include <vector>

namespace fc {

using SchemaId = std::uint32_t;

enum class TypeKind {
    Null,
    Bool,
    Int,
    Float,
    String, // scalars
    Object,  // objectSchema is valid
    List,    // args[0] is the element type
    Union,   // args holds 2+ distinct member types
    Unknown, // element type of an empty array (renders as Any)
};

struct TypeRef {
    TypeKind kind = TypeKind::Unknown;
    SchemaId objectSchema = 0;   // valid iff kind == Object
    std::vector<TypeRef> args;   // List: 1 element; Union: members (stable order)
};

struct Field {
    std::string key;   // raw JSON key; the renderer sanitizes for its target
    TypeRef type;
    bool optional = false;
};

struct Schema {
    std::string name;            // unique, PascalCase class name
    std::vector<Field> fields;   // document order
};

struct SchemaModule {
    // Definition order: children precede parents, so a renderer can emit
    // classes top-to-bottom and every reference is already defined.
    std::vector<Schema> schemas;
    TypeRef root;                // usually Object(rootId); may be List or scalar

    const Schema& at(SchemaId id) const { return schemas.at(id); }
};

} // namespace fc
