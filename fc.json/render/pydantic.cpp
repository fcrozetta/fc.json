#include "pydantic.hpp"

#include <cctype>
#include <string>
#include <unordered_set>
#include <vector>

namespace fc {
namespace {

const std::unordered_set<std::string>& pythonKeywords() {
    static const std::unordered_set<std::string> kw = {
        "False", "None", "True", "and", "as", "assert", "async", "await",
        "break", "class", "continue", "def", "del", "elif", "else", "except",
        "finally", "for", "from", "global", "if", "import", "in", "is",
        "lambda", "nonlocal", "not", "or", "pass", "raise", "return", "try",
        "while", "with", "yield",
    };
    return kw;
}

// Turn a raw JSON key into a valid Python identifier. Non-alphanumerics become
// '_', a leading digit is prefixed, and keywords are suffixed. When the result
// differs from the key, the caller emits a Field(alias=...) to keep the wire
// name.
std::string sanitizeIdent(const std::string& key) {
    std::string out;
    out.reserve(key.size());
    for (unsigned char c : key) {
        out += std::isalnum(c) ? static_cast<char>(c) : '_';
    }
    if (out.empty()) out = "_";
    if (std::isdigit(static_cast<unsigned char>(out[0]))) out = "_" + out;
    if (pythonKeywords().count(out)) out += "_";
    return out;
}

class PyRenderer {
public:
    explicit PyRenderer(const SchemaModule& mod) : mod_(mod) {}

    std::string run() {
        std::string body;
        for (const Schema& s : mod_.schemas) {
            body += "\nclass " + s.name + "(BaseModel):\n";
            if (s.fields.empty()) {
                body += "    pass\n";
            } else {
                for (const Field& f : s.fields) body += renderField(f);
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
            case TypeKind::Null:    return "None";
            case TypeKind::Bool:    return "bool";
            case TypeKind::Int:     return "int";
            case TypeKind::Float:   return "float";
            case TypeKind::String:  return "str";
            case TypeKind::Unknown: usedAny_ = true; return "Any";
            case TypeKind::Object:  return mod_.at(t.objectSchema).name;
            case TypeKind::List:    return "list[" + renderType(t.args.front()) + "]";
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

    std::string renderField(const Field& f) {
        const std::string ident = sanitizeIdent(f.key);
        const bool needAlias = ident != f.key;

        std::string type = renderType(f.type);
        if (f.optional) {
            usedOptional_ = true;
            type = "Optional[" + type + "]";
        }

        std::string line = "    " + ident + ": " + type;
        if (f.optional && needAlias) {
            usedField_ = true;
            line += " = Field(default=None, alias=\"" + f.key + "\")";
        } else if (f.optional) {
            line += " = None";
        } else if (needAlias) {
            usedField_ = true;
            line += " = Field(alias=\"" + f.key + "\")";
        }
        return line + "\n";
    }

    std::string header() const {
        std::string h;
        std::vector<std::string> typing;
        if (usedAny_)      typing.push_back("Any");
        if (usedOptional_) typing.push_back("Optional");
        if (usedUnion_)    typing.push_back("Union");
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
