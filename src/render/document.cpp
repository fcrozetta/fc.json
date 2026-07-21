#include "document.hpp"

#include <string>

#include "json_util.hpp"

namespace fc {
namespace {

// How a leaf (scalar) value is rendered.
enum class LeafMode { Example,
                      Redact };

std::string ind(int depth) { return std::string(static_cast<std::size_t>(depth) * 2, ' '); }

// Type-illustrative default for a scalar node in example mode.
std::string exampleLeaf(NodeKind kind) {
    switch (kind) {
        case NodeKind::Null: return "null";
        case NodeKind::Bool: return "true";
        case NodeKind::Int: return "0";
        case NodeKind::Float: return "1.0";
        case NodeKind::String: return "\"string\"";
        default: return "null"; // Object/Array handled by caller
    }
}

class DocumentRenderer {
public:
    DocumentRenderer(const Tree& tree, LeafMode mode) : tree_(tree), mode_(mode) {}

    std::string run() { return emit(tree_.root, 0) + "\n"; }

private:
    const Tree& tree_;
    LeafMode mode_;

    std::string emit(NodeId id, int depth) {
        const Node& node = tree_.at(id);
        switch (node.kind) {
            case NodeKind::Object: return emitObject(node, depth);
            case NodeKind::Array: return emitArray(node, depth);
            default:
                return mode_ == LeafMode::Redact ? "\"***\"" : exampleLeaf(node.kind);
        }
    }

    std::string emitObject(const Node& node, int depth) {
        if (node.children.empty()) return "{}";
        std::string out = "{\n";
        for (std::size_t i = 0; i < node.children.size(); ++i) {
            const Node& child = tree_.at(node.children[i]);
            out += ind(depth + 1) + jsonString(child.key) + ": " +
                   emit(node.children[i], depth + 1);
            out += (i + 1 < node.children.size()) ? ",\n" : "\n";
        }
        return out + ind(depth) + "}";
    }

    std::string emitArray(const Node& node, int depth) {
        if (node.children.empty()) return "[]";
        std::string out = "[\n";
        for (std::size_t i = 0; i < node.children.size(); ++i) {
            out += ind(depth + 1) + emit(node.children[i], depth + 1);
            out += (i + 1 < node.children.size()) ? ",\n" : "\n";
        }
        return out + ind(depth) + "]";
    }
};

} // namespace

std::string renderExample(const Tree& tree) {
    return DocumentRenderer(tree, LeafMode::Example).run();
}

std::string renderRedacted(const Tree& tree) {
    return DocumentRenderer(tree, LeafMode::Redact).run();
}

} // namespace fc
