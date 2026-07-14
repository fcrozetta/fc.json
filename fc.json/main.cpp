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

    try {
        parser.parse_args(argc, argv);
    } catch (const std::exception& err) {
        std::cerr << err.what() << "\n" << parser;
        return 1;
    }

    try {
        const std::string text = readFile(filename);
        const fc::SchemaModule schema = fc::inferSchema(fc::parse(text));
        std::cout << fc::renderPydantic(schema);
    } catch (const std::exception& err) {
        std::cerr << "fc.json: " << err.what() << "\n";
        return 1;
    }

    return 0;
}
