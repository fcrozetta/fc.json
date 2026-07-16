//
//  main.cpp
//  fc.json
//
//  Created by Fernando Crozetta on 21/10/2024.
//

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include <argparse/argparse.hpp>

#include "parse/parser.hpp"
#include "schema/infer.hpp"
#include "render/pydantic.hpp"
#include "render/jsonschema.hpp"
#include "render/document.hpp"
#include "render/table.hpp"

namespace {

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("could not open file: " + path);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

} // namespace

int main(int argc, char* argv[]) {
    argparse::ArgumentParser parser("fc.json", "{{VERSION}}");
    parser.add_description(
        "Inspect JSON and generate schemas, examples, and classes.");

    std::string filename;
    parser.add_argument("input_filename")
        .help("json file to process")
        .store_into(filename);

    std::string format;
    parser.add_argument("-f", "--format")
        .help("output format: pydantic (default), json-schema, example, redact, table")
        .default_value(std::string("pydantic"))
        .store_into(format);

    try {
        parser.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << err.what() << "\n" << parser;
        return 1;
    }

    // Validated here rather than via argparse .choices() so a bad value gets a
    // clear message instead of argparse's misleading positional-count error.
    if (format != "pydantic" && format != "json-schema" &&
        format != "example" && format != "redact" && format != "table") {
        std::cerr << "fc.json: unknown format '" << format
                  << "' (expected: pydantic, json-schema, example, redact, table)\n";
        return 1;
    }

    try {
        const std::string text = readFile(filename);
        const fc::Tree tree = fc::parse(text);

        std::string output;
        if (format == "example") {
            output = fc::renderExample(tree);
        } else if (format == "redact") {
            output = fc::renderRedacted(tree);
        } else {
            const fc::SchemaModule schema = fc::inferSchema(tree);
            if (format == "json-schema") {
                output = fc::renderJsonSchema(schema);
            } else if (format == "table") {
                output = fc::renderTable(schema);
            } else {
                output = fc::renderPydantic(schema);
            }
        }
        std::cout << output;
    } catch (const std::exception& err) {
        std::cerr << "fc.json: " << err.what() << "\n";
        return 1;
    }

    return 0;
}
