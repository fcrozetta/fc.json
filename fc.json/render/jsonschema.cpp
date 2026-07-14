#include "jsonschema.hpp"

#include <string>
#include <vector>

namespace fc {
namespace {

// A JSON string literal for an arbitrary key/value. Escapes per JSON rules;
// control characters below 0x20 use \u00XX.
std::string jsonString(const std::string& s) {
    static const char* hex = "0123456789abcdef";
    std::string out = "\"";
    for (unsigned char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            default:
                if (c < 0x20) {
                    out += "\\u00";
                    out += hex[(c >> 4) & 0xF];
                    out += hex[c & 0xF];
                } else {
                    out += static_cast<char>(c); // UTF-8 bytes pass through
                }
        }
    }
    return out + "\"";
}

class JsonSchemaRenderer {
public:
    explicit JsonSchemaRenderer(const SchemaModule& mod) : mod_(mod) {}

    std::string run() {
        std::string out = "{\n";
        out += ind(1) + "\"$schema\": \"https://json-schema.org/draft/2020-12/schema\"";

        if (!mod_.schemas.empty()) {
            out += ",\n" + ind(1) + "\"$defs\": {\n";
            for (std::size_t i = 0; i < mod_.schemas.size(); ++i) {
                out += ind(2) + jsonString(mod_.schemas[i].name) + ": " +
                       objectSchema(mod_.schemas[i], 2);
                out += (i + 1 < mod_.schemas.size()) ? ",\n" : "\n";
            }
            out += ind(1) + "}";
        }

        // Splice the root type onto the top-level object so $schema/$defs stay
        // siblings of the document type.
        out += rootMembers();
        out += "\n}\n";
        return out;
    }

private:
    const SchemaModule& mod_;

    static std::string ind(int n) { return std::string(static_cast<std::size_t>(n) * 2, ' '); }

    // Top-level document type, emitted as sibling members of $schema/$defs.
    std::string rootMembers() {
        const TypeRef& r = mod_.root;
        switch (r.kind) {
            case TypeKind::Object:
                return ",\n" + ind(1) + "\"$ref\": " +
                       jsonString("#/$defs/" + mod_.at(r.objectSchema).name);
            case TypeKind::List:
                return ",\n" + ind(1) + "\"type\": \"array\",\n" + ind(1) +
                       "\"items\": " + inlineType(r.args.front(), 1);
            case TypeKind::Null:    return ",\n" + ind(1) + "\"type\": \"null\"";
            case TypeKind::Bool:    return ",\n" + ind(1) + "\"type\": \"boolean\"";
            case TypeKind::Int:     return ",\n" + ind(1) + "\"type\": \"integer\"";
            case TypeKind::Float:   return ",\n" + ind(1) + "\"type\": \"number\"";
            case TypeKind::String:  return ",\n" + ind(1) + "\"type\": \"string\"";
            case TypeKind::Union:   // a single JSON document value is never a union
            case TypeKind::Unknown: return ""; // top-level accepts anything
        }
        return "";
    }

    // Render a named object schema: type/properties/required.
    std::string objectSchema(const Schema& s, int depth) {
        std::string out = "{\n";
        out += ind(depth + 1) + "\"type\": \"object\"";
        if (!s.fields.empty()) {
            out += ",\n" + ind(depth + 1) + "\"properties\": {\n";
            std::vector<std::string> required;
            for (std::size_t i = 0; i < s.fields.size(); ++i) {
                const Field& f = s.fields[i];
                out += ind(depth + 2) + jsonString(f.key) + ": " + inlineType(f.type, depth + 2);
                out += (i + 1 < s.fields.size()) ? ",\n" : "\n";
                if (!f.optional) required.push_back(f.key);
            }
            out += ind(depth + 1) + "}";
            if (!required.empty()) {
                out += ",\n" + ind(depth + 1) + "\"required\": [";
                for (std::size_t i = 0; i < required.size(); ++i) {
                    if (i) out += ", ";
                    out += jsonString(required[i]);
                }
                out += "]";
            }
        }
        out += "\n" + ind(depth) + "}";
        return out;
    }

    // A type as a compact JSON Schema fragment.
    std::string inlineType(const TypeRef& t, int depth) {
        switch (t.kind) {
            case TypeKind::Null:    return R"({ "type": "null" })";
            case TypeKind::Bool:    return R"({ "type": "boolean" })";
            case TypeKind::Int:     return R"({ "type": "integer" })";
            case TypeKind::Float:   return R"({ "type": "number" })";
            case TypeKind::String:  return R"({ "type": "string" })";
            case TypeKind::Unknown: return "{}"; // any
            case TypeKind::Object:
                return "{ \"$ref\": " + jsonString("#/$defs/" + mod_.at(t.objectSchema).name) + " }";
            case TypeKind::List:
                return "{ \"type\": \"array\", \"items\": " + inlineType(t.args.front(), depth) + " }";
            case TypeKind::Union: {
                std::string s = "{ \"anyOf\": [";
                for (std::size_t i = 0; i < t.args.size(); ++i) {
                    s += (i ? ", " : " ");
                    s += inlineType(t.args[i], depth);
                }
                return s + " ] }";
            }
        }
        return "{}";
    }
};

} // namespace

std::string renderJsonSchema(const SchemaModule& mod) {
    return JsonSchemaRenderer(mod).run();
}

} // namespace fc
