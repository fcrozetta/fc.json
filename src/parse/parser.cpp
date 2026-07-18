#include "parser.hpp"

// * rapidjson is confined to this translation unit. Nothing above the parse
// * adapter (IR, renderers, CLI) sees a rapidjson type. See ADR 0001.
#include <rapidjson/document.h>
#include <rapidjson/error/en.h>

#include <string>
#include <vector>

namespace fc {
namespace {

// Recursively lower a rapidjson value into the arena, returning its NodeId.
// Children ids are collected into a local vector and assigned after recursion
// — we never hold a Node& across a Tree::add(), because add() may reallocate
// the underlying vector. Indices survive; references would not.
NodeId lower(const rapidjson::Value& value, std::string key, Tree& tree) {
    Node node;
    node.key = std::move(key);

    switch (value.GetType()) {
        case rapidjson::kNullType:
            node.kind = NodeKind::Null;
            break;
        case rapidjson::kFalseType:
            node.kind = NodeKind::Bool;
            node.scalar = false;
            break;
        case rapidjson::kTrueType:
            node.kind = NodeKind::Bool;
            node.scalar = true;
            break;
        case rapidjson::kStringType:
            node.kind = NodeKind::String;
            node.scalar = std::string(value.GetString(), value.GetStringLength());
            break;
        case rapidjson::kNumberType:
            // Integer vs float is decided here, at the boundary — the old code
            // collapsed everything to float.
            if (value.IsInt64()) {
                node.kind = NodeKind::Int;
                node.scalar = value.GetInt64();
            } else if (value.IsUint64()) {
                node.kind = NodeKind::Int;
                // ! Values above INT64_MAX wrap; kind stays Int (correct for
                // ! type inference), but the retained value is lossy. Rare.
                node.scalar = static_cast<std::int64_t>(value.GetUint64());
            } else {
                node.kind = NodeKind::Float;
                node.scalar = value.GetDouble();
            }
            break;
        case rapidjson::kObjectType:
            node.kind = NodeKind::Object;
            break;
        case rapidjson::kArrayType:
            node.kind = NodeKind::Array;
            break;
    }

    const NodeId id = tree.add(std::move(node));

    if (value.IsObject()) {
        std::vector<NodeId> children;
        children.reserve(value.MemberCount());
        for (auto it = value.MemberBegin(); it != value.MemberEnd(); ++it) {
            children.push_back(lower(
                it->value,
                std::string(it->name.GetString(), it->name.GetStringLength()),
                tree));
        }
        tree.at(id).children = std::move(children);
    } else if (value.IsArray()) {
        std::vector<NodeId> children;
        children.reserve(value.Size());
        for (const auto& element : value.GetArray()) {
            children.push_back(lower(element, std::string(), tree));
        }
        tree.at(id).children = std::move(children);
    }

    return id;
}

} // namespace

Tree parse(const std::string& text) {
    rapidjson::Document doc;
    doc.Parse(text.data(), text.size());

    if (doc.HasParseError()) {
        throw ParseError(
            "JSON parse error at offset " + std::to_string(doc.GetErrorOffset()) +
            ": " + rapidjson::GetParseError_En(doc.GetParseError()));
    }

    Tree tree;
    tree.root = lower(doc, std::string(), tree);
    return tree;
}

} // namespace fc
