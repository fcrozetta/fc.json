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

class FCMapItem{
public:
    string path;
    string name;
    string type; // either value type, or name of other FCMapItem
    
    FCMapItem(const string& path, const string& name, const string& type): path(path), name(name),type(type) {}
    
};

class FCMap{
public:
    vector<FCMapItem> items;
    
    
    vector<FCMapItem> getByPathPrefix(string path){
        vector<FCMapItem> result;
        for (const auto& item : items) {
            if (item.path.starts_with(path)){
                result.push_back(item);
            }
        }
        return result;
    }
    
    bool hasItem(const string& path){
        for (const auto& item : items) {
            if (item.path == path) return true;
        }
        return false;
    }
    
    void createTypes(){
//        Loop the vector and generate the types. If the type points to other item, resolves that as well.
    }
    
    void addItem(const string& path,const string& name = "", const string& type = ""){
        if (!hasItem(path)){
            items.push_back(FCMapItem(path,name, type));
        } else {
            cout<< "OOPS" << endl;
            cout << path <<endl;
        }
    }
    
    void printItems(){
        for (const auto& item : items) {
            cout << item.path + "\t" + item.type +"\t" + item.name<< endl;
        }
    }
    
    
    
    
    
    
};

class FCJson {
private:
    string filename;
    Document doc;
    vector<string> actions;
    FCMap map;
    

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
    
    string processObject(const Value& object, string name, string parentPath = ""){
        string result;
        string path = parentPath.empty()? name: parentPath + "/" + name;
//        std::cout << "\n\nStarting object " << parentPath + " + " + name << std::endl;
        result += "\n\nclass " + removeSlash(path) + "Schema(BaseModel):\n";
        this->map.addItem(path, name, "<object>");
        
        string subClasses;
        for (rapidjson::Value::ConstMemberIterator itr = object.MemberBegin(); itr != object.MemberEnd(); ++itr) {
            subClasses += processValue(itr->value, itr->name.GetString(), path );
        }
        return result;
    }
    
    string processArray(const Value& array, string name, string parentPath = ""){
        string result;
//        cout << "Starting Vector:" << parentPath << name << endl;
        result += "    " + name + ": list[" + parentPath + "]\n";
        
        for (rapidjson::Value::ConstValueIterator itr = array.Begin(); itr != array.End(); ++itr) {
            if (!itr->IsArray() && !itr->IsObject()){
                string path = parentPath + "/" + name;
                string type = kTypeNames[itr->GetType()];
                this->map.addItem(path, removeSlash(name), "list[" + type + "]");
//                result += processValue(*itr, "asdasds", parentPath);
                
            }
            
            break;
        }
        
        return result;
        
    }
    
    string processValue(const Value& value, string name, string parentPath = ""){
        string result;
        if (value.IsObject()) {
                result += processObject(value,name,parentPath);
            } else if (value.IsArray()) {
                result += processArray(value,name,parentPath);
            } else if (value.IsString()) {
//                std::cout << "Path:" << parentPath + "/" +  name << " Type:String Value: " << value.GetString() << std::endl;
                map.addItem(parentPath+"/"+name,name,"string");
                result += "    " + name + ": str\n";
            } else if (value.IsInt()) {
                cout << "    " << name + ": int\n" <<endl;
//                std::cout << "Path:" << parentPath + "/" +  name << "Type:Int Value:" << value.GetInt() << std::endl;
                map.addItem(parentPath+"/"+name,name, "int");
            } else if (value.IsDouble()) {
                cout << "    " << name + ": float\n" <<endl;
//                std::cout << "Path:" << parentPath + "/" +  name <<  "Type:Double Value: " << value.GetDouble() << std::endl;
                map.addItem(parentPath+"/"+name,name,"float");
            } else if (value.IsBool()) {
                cout << "    " << name + ": bool\n" <<endl;
//                std::cout << "Bool: " << value.GetBool() << std::endl;
                map.addItem(parentPath+"/"+name,name,"bool");
            } else if (value.IsNull()) {
                cout << "    " << name + ": None\n" <<endl;
//                std::cout << "Null" << std::endl;
                map.addItem(parentPath+"/"+name,name,"null");
            }
        return result;
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
    
    void process(){
//        auto root = this->doc.GetObject();
        string x = processValue(doc, "Root");
        
        map.printItems();
        
    }
    

};




#endif /* processjson_hpp */
