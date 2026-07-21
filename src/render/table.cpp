#include "table.hpp"

#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <tabulate/table.hpp> // aggregate tabulate.hpp is only a banner in vcpkg

namespace fc {
namespace {

// Compact, neutral type label for a cell. Optionality is shown on the path
// (a trailing "?"), so it is not encoded here.
std::string typeLabel(const SchemaModule& mod, const TypeRef& t) {
    switch (t.kind) {
        case TypeKind::Null:    return "None";
        case TypeKind::Bool:    return "bool";
        case TypeKind::Int:     return "int";
        case TypeKind::Float:   return "float";
        case TypeKind::String:  return "str";
        case TypeKind::Unknown: return "Any";
        case TypeKind::Object:  return mod.at(t.objectSchema).name;
        case TypeKind::List:    return "list[" + typeLabel(mod, t.args.front()) + "]";
        case TypeKind::Union: {
            std::string s = "Union[";
            for (std::size_t i = 0; i < t.args.size(); ++i) {
                if (i) s += ", ";
                s += typeLabel(mod, t.args[i]);
            }
            return s + "]";
        }
    }
    return "Any";
}

class TableBuilder {
public:
    explicit TableBuilder(const SchemaModule& mod) : mod_(mod) {}

    std::vector<std::pair<std::string, std::string>> rows() {
        if (mod_.root.kind == TypeKind::Object) {
            expand("", mod_.root);
        } else {
            rows_.emplace_back("(root)", typeLabel(mod_, mod_.root));
            expand("", mod_.root);
        }
        return std::move(rows_);
    }

private:
    const SchemaModule& mod_;
    std::vector<std::pair<std::string, std::string>> rows_;

    // Emit a row per field, then descend into nested objects (and objects that
    // are the element of a list, tagged with "[]").
    void expand(const std::string& path, const TypeRef& t) {
        if (t.kind == TypeKind::Object) {
            for (const Field& f : mod_.at(t.objectSchema).fields) {
                const std::string p = path.empty() ? f.key : path + "." + f.key;
                rows_.emplace_back(p + (f.optional ? "?" : ""), typeLabel(mod_, f.type));
                expand(p, f.type);
            }
        } else if (t.kind == TypeKind::List && t.args.front().kind == TypeKind::Object) {
            expand(path + "[]", t.args.front());
        }
    }
};

} // namespace

std::string renderTable(const SchemaModule& mod) {
    tabulate::Table table;
    table.add_row({"Path", "Type"});
    for (auto& [path, type] : TableBuilder(mod).rows()) {
        table.add_row({path, type});
    }
    table[0].format().font_style({tabulate::FontStyle::bold});

    std::ostringstream out;
    out << table << "\n";
    return out.str();
}

} // namespace fc
