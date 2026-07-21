#pragma once

// Shared JSON output helpers for renderers that emit JSON (json-schema,
// example, redact). No rapidjson: hand-built, per the ADR 0001 quarantine.

#include <string>

namespace fc {

// A JSON string literal for an arbitrary key or value. Escapes per JSON rules;
// control characters below 0x20 use \u00XX. UTF-8 bytes pass through.
inline std::string jsonString(const std::string& s) {
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
                    out += static_cast<char>(c);
                }
        }
    }
    return out + "\"";
}

} // namespace fc
