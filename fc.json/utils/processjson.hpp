//
//  processjson.hpp
//  fc.json
//
//  Created by Fernando Crozetta on 21/10/2024.
//

#ifndef processjson_hpp
#define processjson_hpp

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <functional>

#include "schemaGenerator.h"

using namespace std;
using namespace rapidjson;

std::string generateClass(const std::unordered_map<std::string, std::string>& schema, const std::string& className) {
    std::ostringstream oss;

    oss << "class " << className << " {\n";
    oss << "public:\n";

    for (const auto& field : schema) {
        if (field.first.find(className + "_") == 0) {
            std::string fieldName = field.first.substr(className.length() + 1);
            oss << "    " << field.second << " " << fieldName << ";\n";
        }
    }

    oss << "};\n\n";

    return oss.str();
}

std::string generateClasses(const std::unordered_map<std::string, std::string>& schema) {
    std::ostringstream oss;

    oss << "#ifndef DYNAMICMODEL_H\n";
    oss << "#define DYNAMICMODEL_H\n\n";
    oss << "#include <string>\n";
    oss << "#include <vector>\n\n";

    std::unordered_map<std::string, bool> generatedClasses;
    for (const auto& field : schema) {
        std::string className = field.first.substr(0, field.first.find('_'));
        if (generatedClasses.find(className) == generatedClasses.end()) {
            oss << generateClass(schema, className);
            generatedClasses[className] = true;
        }
    }

    oss << "#endif // DYNAMICMODEL_H\n";

    return oss.str();
}


class FCJson {
private:
    string filename;
    Document doc;
    vector<string> actions;
    unordered_map<string, string> schema;
    

    string read_json(){
        ifstream file(this->filename);
        if (!file.is_open()){
            throw runtime_error("Could not read json file");
        }
        
        stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    
    void load_json(){
        this->doc.Parse(read_json().c_str());
    }
    
    void processJsonValue(Value& value, unordered_map<string, string>& schema, const string& parentKey = ""){
        string key = parentKey.empty() ? value.name.GetString() : parentKey + "/" + value.name.GetString();
//            cout <<key << endl;
            if (itr->value.IsString()) {
    //            stringFunc(value);
                schema[key] = "string";
            } else if (itr->value.IsInt()) {
    //            intFunc(value);
                schema[key] = "int";
            } else if (itr->value.IsFloat()) {
    //            floatFunc(value);
                schema[key] = "float";
            } else if (itr->value.IsBool()) {
    //            boolFunc(value);
                schema[key] = "bool";
            } else if (itr->value.IsNull()) {
    //            nullFunc(value);
                schema[key] = "null";
            } else if (itr->value.IsObject()) {
    //            objectFunc(value);
                for (auto itr = value.MemberBegin(); itr != value.MemberEnd(); ++itr){
                
                schema[key] = key + "_t";
                processJsonValue(itr, schema, key);
            } else if (itr->value.IsArray()) {
    //                arrayFunc(value);
                schema[key] = "std::vector<" + key + "_t>";
                for (auto& v: itr->value.GetArray()){
                    processJsonValue(v[0], schema, key);
                }
            } else {
                schema[key] = "auto?";
            }
        }
        
    }
    
    bool hasAction(const string& target){
        return find(this->actions.begin(), this->actions.end(), target) != this->actions.end();
    }
    
    
    
public:
    FCJson(string& filename){
        this->filename = filename;
        this->load_json();
    }
    
    void setActions(vector<string> actions){
        this->actions = actions;
    }
    
    void addAction(string action){
        this->actions.push_back(action);
    }
    
    
    string prettyPrint(){
        StringBuffer buffer;
        PrettyWriter<StringBuffer> writer(buffer);
    //        Why is this needed?
        this->doc.Accept(writer);
        
        return buffer.GetString();
    }
    
    void processJson(){
        processJsonValue(doc, this->schema);
        printRawSchema(this->schema);
        
    }

};




#endif /* processjson_hpp */
