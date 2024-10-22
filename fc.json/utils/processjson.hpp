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

enum DataType{
    Null,
    Bool,
    String,
    Int,
    Float,
    Array,
    Object,
    UNDEFINED,
};

DataType getType(Value& v){
    DataType result;
    if (v.IsNull()) {
        result = DataType::Null;
    } else if (v.IsBool()) {
        result = DataType::Bool;
    } else if (v.IsString()) {
        result = DataType::String;
    } else if (v.IsInt()) {
        result = DataType::Int;
    } else if (v.IsFloat()) {
        result = DataType::Float;
    } else if (v.IsArray()) {
        result = DataType::Array;
    } else if (v.IsObject()) {
        result = DataType::Object;
    }else{
        result = DataType::UNDEFINED;
    }
    
    return result;
}

string type2Text(DataType t){
    string result;
    switch (t) {
        case Null:
            result = "null";
            break;
        case Bool:
            result = "boolean";
            break;
        case String:
            result = "string";
            break;
        case Int:
            result = "int";
            break;
        case Float:
            result = "float";
            break;
        case Array:
            result = "<Vector>";
            break;
        case Object:
            result = "<Object>";
            break;
        default:
            result = "???";
            break;
    }
    
    return result;
}


class FCJson {
private:
    string filename;
    Document doc;
    vector<string> actions;
    map<string, string> schema;
    

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
    
    void processJsonValue(Value& value, const string& name, map<string, string>& schema, const string& parentKey = ""){
        string key;
        if (name.empty()){
            key = parentKey;
        }else{
            key = parentKey.empty() ? name : parentKey + "/" + name;
        }
        
        DataType tp = getType(value);
        
        switch (tp) {
            case Null:{
                schema[key] = "null";
                break;
            }
                
            case Bool: {
                schema[key] = "boolean";
                break;
            }
                
            case Int:{
                schema[key] = "int";
                break;
            }
                
            case Float: {
                schema[key] = "float";
                break;
            }
                
            case String: {
                schema[key] = "string";
                break;
            }
                
            case Object: {
                for (auto itr = value.MemberBegin(); itr != value.MemberEnd(); ++itr){
//                    schema[key] = key + "/t";
//                    schema[key] = parentKey + "<Object>";
                    schema[key] = "<Object>";
                    const string& _name = itr->name.GetString();
                    processJsonValue(itr->value, _name, schema, key);
                }
                break;
            }
                
            case Array: {
//                I have no idea how I managed to implement this.
//                Just realized debugging will be hell... Sorry.
                auto subType = getType(value[0]);
                if (subType == DataType::Object){
                    schema[key] = "Vector<" + key + "/<Object>>";
                    processJsonValue(value[0],  "<Object>", schema,key);
                } else if (subType == DataType::Array){
//                    FIXME: This is not working as it should
                    processJsonValue(value[0],  type2Text(getType(value[0])), schema,key + "/<innerArray>");
                } else{
                    schema[key] = "Vector<" + type2Text(getType(value[0])) + ">";
                }
                
                break;
            }
                
            default:{
                schema[key] = "???";
                break;
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
        this->processJsonValue(doc,"root", schema);
        
        if (hasAction("table")){
            printTableSchema(this->schema);
        }
        else if (hasAction("schema")){
            printRawSchema(this->schema);
        }
    }

};




#endif /* processjson_hpp */
