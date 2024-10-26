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
    vector<FCItem>* children;
    FCItem* parent;
    
    
    FCItem(const string& name, Type rapidJsonType): name(name), rapidJsonType(rapidJsonType) {
        this->type = FCItemType::Undefined;
        auto c = vector<FCItem>{};
        this->children = &c;
        
    }
    
    void addChild(FCItem child){
        child.parent = this;
        children->push_back(child);
    }
    
    string getPath(){
        if (!this->parent){
            return name;
        }
        return this->parent->getPath()+"/"+this->name + "\n";
    }
    
    string getSchemaType(){
        switch (type) {
            case FCItemType::Primitive:{
                return kTypeNames[rapidJsonType];
                break;
            }
                
            case FCItemType::Object:{
                assert(children->size() == 1);
                return children->front().getSchemaType();
                break;
            }
                
            case FCItemType::Array: {
                if (children->size() == 0){
                    return "list[None]";
                }
                vector<string>* childrenTypes = new vector<string>;
                for (FCItem child: *children){
                    childrenTypes->push_back(child.getSchemaType());
                }
                
//                Make the responses unique
                sort(childrenTypes->begin(),childrenTypes->end());
                childrenTypes->erase(unique(childrenTypes->begin(),childrenTypes->end()),childrenTypes->end());
                
                for (auto c : *childrenTypes){
                    cout << c << endl;
                }
                
                return "";
                break;
            }
                
            
            default: {
                return "???";
                break;
            }
                
                
        }
        
    }
    
    
};

//class FCMap{
//public:
//    vector<FCItem> items;
//    
//    
//    vector<FCItem> getByPathPrefix(string path){
//        vector<FCItem> result;
//        for (const auto& item : items) {
//            if (item.path.starts_with(path)){
//                result.push_back(item);
//            }
//        }
//        return result;
//    }
//    
//    bool hasItem(const string& path){
//        for (const auto& item : items) {
//            if (item.path == path) return true;
//        }
//        return false;
//    }
//    
//    void createTypes(){
////        Loop the vector and generate the types. If the type points to other item, resolves that as well.
//    }
//    
//    void addItem(const string& path,const string& name = "", const string& type = ""){
//        if (!hasItem(path)){
//            items.push_back(FCItem(path,name, type));
//        } else {
//            cout<< "OOPS" << endl;
//            cout << path <<endl;
//        }
//    }
//    
//    void printItems(){
////        for (const auto& item : items) {
////            cout << item.path + "\t" + item.type +"\t" + item.name<< endl;
////        }
//    }
//};

class FCJson {
private:
    string filename;
    Document doc;
    vector<string> actions;
//    FCMapItem map;
    FCItem* root;
    

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
    
    string removeSlash(const string& input){
        string result = input;
        for (char& c : result){
            if (c == '/') {
                c = '_';                
            }
        }
        return result;
    }
    
    void processObject(const Value& object, string name, FCItem* parent){
        auto current = FCItem(name, Type::kObjectType);
        current.type = FCItemType::Object;
        if (parent->type == FCItemType::Undefined) {
            root = &current;
        }else{
            current.parent = parent;
            parent->children->push_back(current);
            
        }
        
        
        for (rapidjson::Value::ConstMemberIterator itr = object.MemberBegin(); itr != object.MemberEnd(); ++itr) {
            this->processValue(itr->value, itr->name.GetString(), &current );
        }
    }
    
    void processArray(const Value& array, string name, FCItem* parent){
        auto current = FCItem(name, Type::kArrayType);
        
//        for (const Value& v : array.GetArray()) {
//            processValue(v, "", &parent);
//        }
//        for (rapidjson::Value::ConstValueIterator itr = array.Begin(); itr != array.End(); ++itr) {
////            TODO: Array<Array<T>> need to be covered
//            if (!itr->IsArray() && !itr->IsObject()){
//                auto childType = itr->
//            }
            
//            break;
//        }
    }
    
    void processValue(const Value& value, string name, FCItem* parent){
        if (value.IsObject()) {
            processObject(value,name,parent);
        } else if (value.IsArray()) {
            processArray(value,name,parent);
        } else if (value.IsString()) {
            auto current = FCItem(name, Type::kStringType);
            parent->children->push_back(current);
        } else if (value.IsInt()) {
            auto current = FCItem(name, Type::kNumberType);
            parent->children->push_back(current);
        } else if (value.IsDouble()) {
            auto current = FCItem(name, Type::kNumberType);
            parent->children->push_back(current);
        } else if (value.IsBool()) {
            auto current = FCItem(name, Type::kTrueType);
            parent->children->push_back(current);
        } else if (value.IsNull()) {
            auto current = FCItem(name, Type::kNullType);
            parent->children->push_back(current);
        }
    }
    
    
    
    
public:
    FCJson(string& filename){
        this->filename = filename;
        this->load_json();
        auto empty = FCItem("", Type::kNullType);
        root = &empty;
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
        processValue(doc, "Root", root);
        
        cout<< root->getPath()<< endl;
        
    }
    

};




#endif /* processjson_hpp */
