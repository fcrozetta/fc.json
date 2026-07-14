#pragma once

// The ValueIR: a faithful, vendor-agnostic tree of a parsed JSON document.
// * No parser type (rapidjson) appears here — that stays behind the parse
// * adapter. No rendering logic lives here either. See ADR 0001.

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace fc {

// Index into Tree::nodes. Children are held by id, not by pointer, so the
// arena can grow (and be copied/pruned) without invalidating references.
using NodeId = std::uint32_t;

enum class NodeKind { Null, Bool, Int, Float, String, Array, Object };

struct Node {
    NodeKind kind = NodeKind::Null;

    // Field name when this node is a member of an object; empty otherwise
    // (array elements and the root are unnamed).
    std::string key;

    // Retained scalar value for value-consuming renderers (example, redact,
    // diff). monostate for Null / Array / Object.
    std::variant<std::monostate, bool, std::int64_t, double, std::string> scalar;

    // Object members or array elements, in document order.
    std::vector<NodeId> children;
};

class Tree {
public:
    std::vector<Node> nodes;
    NodeId root = 0;

    NodeId add(Node node) {
        nodes.push_back(std::move(node));
        return static_cast<NodeId>(nodes.size() - 1);
    }

    const Node& at(NodeId id) const { return nodes.at(id); }
    Node& at(NodeId id) { return nodes.at(id); }

    bool empty() const { return nodes.empty(); }
    std::size_t size() const { return nodes.size(); }
};

} // namespace fc
