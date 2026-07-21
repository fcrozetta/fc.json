#include "pydantic.hpp"

#include <cctype>
#include <string>
#include <unordered_set>
#include <vector>

namespace fc {
namespace {

// Names a generated class or field identifier must not take: Python keywords
// (a plain syntax error), plus the helper names this renderer imports. A class
// named BaseModel would shadow the import so siblings inherit the wrong base;
// a field named Field/Optional/Union/Any shadows a symbol used in the same
// class body and breaks at import time.
const std::unordered_set<std::string>& reservedNames() {
    static const std::unordered_set<std::string> names = {
        // keywords
        "False",
        "None",
        "True",
        "and",
        "as",
        "assert",
        "async",
        "await",
        "break",
        "class",
        "continue",
        "def",
        "del",
        "elif",
        "else",
        "except",
        "finally",
        "for",
        "from",
        "global",
        "if",
        "import",
        "in",
        "is",
        "lambda",
        "nonlocal",
        "not",
        "or",
        "pass",
        "raise",
        "return",
        "try",
        "while",
        "with",
        "yield",
        // imported helpers
        "BaseModel",
        "Field",
        "Any",
        "Optional",
        "Union",
    };
    return names;
}

// Turn a raw JSON key into a valid, Pydantic-usable Python identifier. When the
// result differs from the key, the caller emits Field(alias=...) to keep the
// wire name.
std::string sanitizeIdent(const std::string& key) {
    std::string out;
    out.reserve(key.size());
    for (unsigned char c : key) {
        out += std::isalnum(c) ? static_cast<char>(c) : '_';
    }
    // Pydantic rejects leading-underscore names (treated as private attributes),
    // and a leading digit is not a valid identifier. Prefix rather than mutate
    // in place so the alias still carries the original key. Covers "1st", but
    // also "_id" and "__typename".
    if (out.empty() || out.front() == '_' ||
        std::isdigit(static_cast<unsigned char>(out.front()))) {
        out = "field_" + out;
    }
    if (reservedNames().count(out)) out += "_";
    return out;
}

// Guard a structural class name against reserved names (keywords and imported
// helpers like BaseModel). PascalCase already guarantees a valid, non-empty,
// non-digit start, so a reserved-name clash is the only remaining illegality.
std::string pyClassName(const std::string& name) {
    if (reservedNames().count(name)) return name + "_";
    return name;
}

// Emit a Python double-quoted string literal for an arbitrary key, escaping so
// the generated source stays valid regardless of quotes, backslashes, or
// control characters in the key.
std::string pyStringLiteral(const std::string& s) {
    static const char* hex = "0123456789abcdef";
    std::string out = "\"";
    for (unsigned char c : s) {
        switch (c) {
            case '\\': out += "\\\\"; break;
            case '"': out += "\\\""; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) {
                    out += "\\x";
                    out += hex[(c >> 4) & 0xF];
                    out += hex[c & 0xF];
                } else {
                    out += static_cast<char>(c); // UTF-8 bytes pass through
                }
        }
    }
    return out + "\"";
}

class PyRenderer {
public:
    explicit PyRenderer(const SchemaModule& mod) : mod_(mod) {}

    std::string run() {
        std::string body;
        for (const Schema& s : mod_.schemas) {
            body += "\nclass " + pyClassName(s.name) + "(BaseModel):\n";
            if (s.fields.empty()) {
                body += "    pass\n";
            } else {
                std::unordered_set<std::string> used; // disambiguate per class
                for (const Field& f : s.fields) body += renderField(f, used);
            }
        }

        // A non-object root (top-level array or scalar) has no class of its
        // own; give it a named alias so the document type is still referenceable.
        if (mod_.root.kind != TypeKind::Object) {
            body += "\nRoot = " + renderType(mod_.root) + "\n";
        }

        return header() + body;
    }

private:
    const SchemaModule& mod_;
    bool usedOptional_ = false;
    bool usedUnion_ = false;
    bool usedAny_ = false;
    bool usedField_ = false;

    std::string renderType(const TypeRef& t) {
        switch (t.kind) {
            case TypeKind::Null: return "None";
            case TypeKind::Bool: return "bool";
            case TypeKind::Int: return "int";
            case TypeKind::Float: return "float";
            case TypeKind::String: return "str";
            case TypeKind::Unknown: usedAny_ = true; return "Any";
            case TypeKind::Object: return pyClassName(mod_.at(t.objectSchema).name);
            case TypeKind::List: return "list[" + renderType(t.args.front()) + "]";
            case TypeKind::Union: {
                usedUnion_ = true;
                std::string s = "Union[";
                for (std::size_t i = 0; i < t.args.size(); ++i) {
                    if (i) s += ", ";
                    s += renderType(t.args[i]);
                }
                return s + "]";
            }
        }
        return "Any";
    }

    std::string renderField(const Field& f, std::unordered_set<std::string>& used) {
        std::string ident = sanitizeIdent(f.key);
        // Two distinct keys can sanitize to the same identifier (e.g. "a-b" and
        // "a_b"); suffix collisions so no field is silently dropped.
        if (!used.insert(ident).second) {
            const std::string base = ident;
            for (int n = 2;; ++n) {
                ident = base + std::to_string(n);
                if (used.insert(ident).second) break;
            }
        }
        const bool needAlias = ident != f.key;

        std::string type = renderType(f.type);
        if (f.optional) {
            usedOptional_ = true;
            type = "Optional[" + type + "]";
        }

        std::string line = "    " + ident + ": " + type;
        if (f.optional && needAlias) {
            usedField_ = true;
            line += " = Field(default=None, alias=" + pyStringLiteral(f.key) + ")";
        } else if (f.optional) {
            line += " = None";
        } else if (needAlias) {
            usedField_ = true;
            line += " = Field(alias=" + pyStringLiteral(f.key) + ")";
        }
        return line + "\n";
    }

    std::string header() const {
        std::string h;
        std::vector<std::string> typing;
        if (usedAny_) typing.push_back("Any");
        if (usedOptional_) typing.push_back("Optional");
        if (usedUnion_) typing.push_back("Union");
        if (!typing.empty()) {
            h += "from typing import ";
            for (std::size_t i = 0; i < typing.size(); ++i) {
                if (i) h += ", ";
                h += typing[i];
            }
            h += "\n";
        }
        h += "from pydantic import BaseModel";
        if (usedField_) h += ", Field";
        h += "\n";
        return h;
    }
};

} // namespace

std::string renderPydantic(const SchemaModule& mod) {
    return PyRenderer(mod).run();
}

} // namespace fc
