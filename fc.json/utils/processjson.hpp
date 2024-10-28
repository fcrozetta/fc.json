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
#include <typeinfo>
#include "../include/rapidjson/prettywriter.h"
#include "../include/rapidjson/stringbuffer.h"
#include "../include/rapidjson/pointer.h"

#include "schemaGenerator.hpp"

using namespace std;
using namespace rapidjson;

static const char* kTypeNames[] =
{ "Null", "False", "True", "<Object>", "<Array>", "String", "Number" };

enum FCItemType {
    Undefined,
    Primitive,
    Array,
    Object,
};

class FCItem{
public:
    FCItemType type;
    string name = "";
    Type rapidJsonType;
    list<FCItem*> children;
    FCItem* parent;
    
       
    FCItem(){
        this->name="";
        this->type = FCItemType::Undefined;
        this->rapidJsonType = Type::kNullType;
        this->parent = nullptr;
        
    }
    
    string removeSlash(const string& input){
        string result = input;
        for (char& c : result){
            if (c == '/') {
                c = '_';
            }
        }
        return result;
    }
    
    void printSummary(){
        cout << getPath() +" -> "+kTypeNames[rapidJsonType]<< endl;
    }
    
    string getPath(){
        if (this->parent == NULL){
            return name;
        }
        return this->parent->getPath()+"/"+this->name;
    }
    
    string getSchemaName(){
        //        TODO: Update response to match the correct schema  (pydantic for now)
        switch (type) {
            case FCItemType::Primitive:{
                switch (rapidJsonType) {
                    case Type::kNullType:
                        return "None";
                        break;
                    case Type::kTrueType:
                    case Type::kFalseType:
                        return "bool";
                        break;
                    case Type::kNumberType:
//                        This has to be solved
                        return "float";
                        break;
                    case Type::kStringType:
                        return "str";
                        break;
                    default:
                        break;
                }
                return kTypeNames[rapidJsonType];
                break;
            }
                
            case FCItemType::Object:{
                return removeSlash(getPath() + "Schema");
                break;
            }
                
            case FCItemType::Array: {
                if (children.size() == 0){
                    return "list[None]";
                }
                vector<string> childrenTypes = vector<string>{};
                for (FCItem* child: children){
                    childrenTypes.push_back(child->getSchemaName());
                }
                
                //                This makes the responses unique.
                sort(childrenTypes.begin(),childrenTypes.end());
                childrenTypes
                    .erase(unique(childrenTypes.begin(),childrenTypes.end()),
                           childrenTypes.end());
                
                //                If it is a list, we may have the prefix/suffix. This also changes if you have multiple object types
                string prefix = "list[";
                string suffix = "]";
                
                if (childrenTypes.size()>1){
                    prefix += "Union[";
                    suffix += "]";
                }
                
                ostringstream oss;
                for (size_t i = 0; i< childrenTypes.size();++i){
                    if (i != 0){
                        oss << ',';
                    }
                    oss << childrenTypes[i];
                }
                
                return prefix + oss.str() + suffix;
                break;
            }
                
            
            default: {
                return "???";
                break;
            }
                
                
        }
        
    }
    
    
};

class FCJson {
private:
    string filename;
    Document doc;
    vector<string> actions;
    list<FCItem> tree;
    //    FCMapItem map;
    //    FCItem root;
    

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
        assert(this->doc.IsObject());
    }
    
    
    bool hasAction(const string& target){
        return find(this->actions.begin(), this->actions.end(), target) != this->actions.end();
    }
    
    
    
    void processObject(const Value& object, string name, FCItem* parent){
        tree.emplace_back(FCItem());
        FCItem& current = tree.back();
        current.type = FCItemType::Object;
        current.name = name;
        current.rapidJsonType = Type::kObjectType;
        if (parent){
            parent->children.push_back(&current);
            current.parent = parent;
            
        }
        
        for (rapidjson::Value::ConstMemberIterator itr = object.MemberBegin(); itr != object.MemberEnd(); ++itr) {
            this->processValue(itr->value, itr->name.GetString(), &current );
        }
        
    }
    
    void processArray(const Value& array, string name, FCItem* parent){
        tree.emplace_back(FCItem());
        FCItem& current = tree.back();
        current.name = name;
        current.rapidJsonType = Type::kArrayType;
        current.type = FCItemType::Array;
        parent->children.push_back(&current);
        current.parent = parent;
        
        size_t index = 0;
        for (const Value& v: array.GetArray()){
            this->processValue(v, to_string(index), &current);
            index++;
        }
    }
    
    void processValue(const Value& value, string name, FCItem* parent){
        if (value.IsObject()) {
            processObject(value, name, parent);
        } else if (value.IsArray()) {
            processArray(value,name,parent);
        } else {
            this->tree.emplace_back(FCItem());
            FCItem& current = tree.back();
            current.name = name;
            current.rapidJsonType = value.GetType();
            current.type = FCItemType::Primitive;
            current.parent = parent;
            parent->children.push_back(&current);
        }
    }
    
    
    
    
public:
    FCJson(string& filename){
        this->filename = filename;
        this->load_json();
        this->tree = list<FCItem>{};
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
    
    void process(){
        processValue(doc, "Root", nullptr);
        
        cout << generate_schemas() << endl;
//        for (auto itr = this->tree.begin(); itr != this->tree.end(); ++itr) {
//            cout <<itr->name << " -> " << itr->getSchemaName() << endl;
//        }
    }
    
    string generate_schemas(const string& model = "pydantic"){
//        TODO: Later modify to accept other templates
        string result = "from typing import Union\nfrom pydantic import BaseModel\n";
        
        for (auto it = tree.rbegin(); it != tree.rend(); ++it) {
            if (it->type == FCItemType::Object){
                result += "\nclass " + it->getSchemaName() + "(BaseModel):\n";
                
                for (const auto& v : it->children) {
                    result += "    " + v->name + ": " + v->getSchemaName() + "\n";
                }
            }
        }
        return result;
    }
    

};




#endif /* processjson_hpp */
