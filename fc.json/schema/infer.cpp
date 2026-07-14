#include "infer.hpp"

#include <algorithm>
#include <cctype>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace fc {
namespace {

// PascalCase a raw JSON key into a class-name fragment, preserving internal
// camelCase and splitting on non-alphanumerics. "list_of lists" -> "ListOfLists",
// "nestedObject" -> "NestedObject".
std::string pascalCase(const std::string& in) {
    std::string out;
    bool upNext = true;
    for (unsigned char ch : in) {
        if (std::isalnum(ch)) {
            out += upNext ? static_cast<char>(std::toupper(ch)) : static_cast<char>(ch);
            upNext = false;
        } else {
            upNext = true;
        }
    }
    if (out.empty() || std::isdigit(static_cast<unsigned char>(out[0]))) {
        out = "Schema" + out;
    }
    return out;
}

// Naive, deterministic singularization for array-element name hints (ADR 0001):
// trailing 's' -> drop it ("users" -> "user"); otherwise append "Item"
// ("a" -> "aItem"). Applied to the raw hint before PascalCase.
std::string singularHint(const std::string& h) {
    if (h.size() > 1 && (h.back() == 's' || h.back() == 'S')) {
        return h.substr(0, h.size() - 1);
    }
    return h + "Item";
}

TypeRef scalar(TypeKind k) { return TypeRef{k, 0, {}}; }

class Inferrer {
public:
    explicit Inferrer(const Tree& tree) : tree_(tree) {}

    SchemaModule run() {
        mod_.root = mergeNodes({tree_.root}, "Root");
        return std::move(mod_);
    }

private:
    const Tree& tree_;
    SchemaModule mod_;
    std::unordered_map<std::string, SchemaId> bySigHint_;  // hint\x1f sig -> id (dedup)
    std::unordered_map<SchemaId, std::string> sigOf_;      // id -> structural signature
    std::unordered_set<std::string> usedNames_;

    // Merge a set of value nodes into a single type. This is the whole engine:
    // one scalar kind -> that scalar; all objects -> a merged schema; all arrays
    // -> List of the merge of all their pooled elements; a mix -> a Union.
    TypeRef mergeNodes(const std::vector<NodeId>& nodes, const std::string& hint) {
        if (nodes.empty()) return scalar(TypeKind::Unknown);

        std::vector<NodeId> objs, arrs;
        bool hasNull = false, hasBool = false, hasInt = false, hasFloat = false, hasStr = false;
        for (NodeId id : nodes) {
            switch (tree_.at(id).kind) {
                case NodeKind::Null:   hasNull = true; break;
                case NodeKind::Bool:   hasBool = true; break;
                case NodeKind::Int:    hasInt = true; break;
                case NodeKind::Float:  hasFloat = true; break;
                case NodeKind::String: hasStr = true; break;
                case NodeKind::Object: objs.push_back(id); break;
                case NodeKind::Array:  arrs.push_back(id); break;
            }
        }

        std::vector<TypeRef> members;
        if (hasNull)  members.push_back(scalar(TypeKind::Null));
        if (hasBool)  members.push_back(scalar(TypeKind::Bool));
        if (hasInt)   members.push_back(scalar(TypeKind::Int));
        if (hasFloat) members.push_back(scalar(TypeKind::Float));
        if (hasStr)   members.push_back(scalar(TypeKind::String));
        if (!objs.empty()) {
            members.push_back(TypeRef{TypeKind::Object, mergeObjects(objs, hint), {}});
        }
        if (!arrs.empty()) {
            std::vector<NodeId> elems;
            for (NodeId a : arrs) {
                const auto& kids = tree_.at(a).children;
                elems.insert(elems.end(), kids.begin(), kids.end());
            }
            TypeRef elem = mergeNodes(elems, singularHint(hint));
            members.push_back(TypeRef{TypeKind::List, 0, {elem}});
        }

        if (members.size() == 1) return members.front();

        // Deterministic union order.
        std::sort(members.begin(), members.end(),
                  [this](const TypeRef& a, const TypeRef& b) { return typeSig(a) < typeSig(b); });
        return TypeRef{TypeKind::Union, 0, std::move(members)};
    }

    // Merge N object instances into one schema: union of fields in first-seen
    // order; a field missing from any instance is Optional (decision (2)).
    SchemaId mergeObjects(const std::vector<NodeId>& objs, const std::string& hint) {
        // Reserve this object's name before recursing into its fields, so a
        // descendant sharing the hint cannot take it first (children register
        // before parents). Without this, {"root":{...},...} emits the top-level
        // model as Root2 because the "root" field grabs Root.
        const std::string name = uniqueName(pascalCase(hint));

        std::vector<std::string> keyOrder;
        std::unordered_set<std::string> seen;
        std::unordered_map<std::string, std::vector<NodeId>> values;
        for (NodeId obj : objs) {
            for (NodeId childId : tree_.at(obj).children) {
                const std::string& key = tree_.at(childId).key;
                if (seen.insert(key).second) keyOrder.push_back(key);
                values[key].push_back(childId);
            }
        }

        std::vector<Field> fields;
        fields.reserve(keyOrder.size());
        for (const std::string& key : keyOrder) {
            const std::vector<NodeId>& vs = values[key];
            Field f;
            f.key = key;
            f.type = mergeNodes(vs, key);       // registers nested schemas first
            f.optional = vs.size() < objs.size();
            fields.push_back(std::move(f));
        }

        const std::string sig = schemaSig(fields);
        const std::string dedupKey = hint + '\x1f' + sig;
        if (auto it = bySigHint_.find(dedupKey); it != bySigHint_.end()) {
            return it->second;
        }

        const SchemaId id = static_cast<SchemaId>(mod_.schemas.size());
        mod_.schemas.push_back(Schema{name, std::move(fields)});
        sigOf_[id] = sig;
        bySigHint_[dedupKey] = id;
        return id;
    }

    // Structural signature: name-agnostic, so identical shapes collide (and,
    // under the same hint, dedup). Object types recurse through sigOf_, which
    // is populated bottom-up before the parent is registered.
    std::string typeSig(const TypeRef& t) const {
        switch (t.kind) {
            case TypeKind::Null:    return "null";
            case TypeKind::Bool:    return "bool";
            case TypeKind::Int:     return "int";
            case TypeKind::Float:   return "float";
            case TypeKind::String:  return "str";
            case TypeKind::Unknown: return "any";
            case TypeKind::Object:  return "{" + sigOf_.at(t.objectSchema) + "}";
            case TypeKind::List:    return "[" + typeSig(t.args.front()) + "]";
            case TypeKind::Union: {
                std::vector<std::string> ms;
                ms.reserve(t.args.size());
                for (const TypeRef& a : t.args) ms.push_back(typeSig(a));
                std::sort(ms.begin(), ms.end());
                std::string s = "(";
                for (std::size_t i = 0; i < ms.size(); ++i) {
                    if (i) s += "|";
                    s += ms[i];
                }
                return s + ")";
            }
        }
        return "?";
    }

    std::string schemaSig(const std::vector<Field>& fields) const {
        std::string s;
        for (const Field& f : fields) {
            s += f.key;
            if (f.optional) s += "?";
            s += ":" + typeSig(f.type) + ";";
        }
        return s;
    }

    std::string uniqueName(const std::string& base) {
        if (usedNames_.insert(base).second) return base;
        for (int n = 2;; ++n) {
            std::string candidate = base + std::to_string(n);
            if (usedNames_.insert(candidate).second) return candidate;
        }
    }
};

} // namespace

SchemaModule inferSchema(const Tree& tree) {
    return Inferrer(tree).run();
}

} // namespace fc
