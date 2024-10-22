//
//  main.cpp
//  fc.json
//
//  Created by Fernando Crozetta on 21/10/2024.
//

#include <iostream>


#include "include/argparse/argparse.hpp"
#include "include/rapidjson/document.h"
#include "include/rapidjson/prettywriter.h"
#include "include/rapidjson/stringbuffer.h"

#include "utils/processjson.hpp"

using namespace std;



int main(int argc, const char * argv[]) {
    argparse::ArgumentParser parser("fc.json");
    parser.add_description("This Application is used to perform different tasks on a given json file.");
    
    string filename;
    parser.add_argument("input_filename")
        .help("json filename to be processed")
        .store_into(filename);
    
    bool pretty = false;
    parser.add_argument("-p","--pretty")
        .help("pretty print the json")
        .flag()
        .store_into(pretty);
    
//    TODO: Implement this functionality
    string diff;
    parser.add_argument("--diff")
        .help("Compare input with diff filename.")
        .store_into(diff);
    
    bool redact = false;
    parser.add_argument("--redact")
        .help("Print the json redacting all the fields, and adding an example based on the type of the field")
        .flag()
        .store_into(redact);
    
    bool schema = false;
    parser.add_argument("-s","--schema")
        .help("modify output to schema instead of json")
        .flag()
        .store_into(schema);
    
    bool pydantic = false;
    parser.add_argument("--pydantic")
        .help("create pydantic classes based on the json file")
        .flag()
        .store_into(pydantic);
    
    bool table = false;
    parser.add_argument("-t","--table")
        .help("print as table")
        .flag()
        .store_into(table);
    
//    Parser loading varables here
    try {
        parser.parse_args(argc,argv);
    } catch (const std::exception& err) {
        std::cerr << err.what() << std::endl;
        std::cerr <<parser;
        std::exit(1);
    }
    
    FCJson doc = FCJson(filename);
    
    if (redact){
        doc.addAction("redact");
    }
    if (table){
        doc.addAction("table");
    }
    if (schema) {
        doc.addAction("schema");
    }
    
    doc.processJson();
//    cout << doc.prettyPrint() <<endl;
    return 0;
}
