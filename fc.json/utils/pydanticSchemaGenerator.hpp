//
//  pydanticSchemaGenerator.h
//  fc.json
//
//  Created by Fernando Crozetta on 22/10/2024.
//

#ifndef pydanticSchemaGenerator_h
#define pydanticSchemaGenerator_h
#include <stdio.h>
#include <iostream>
#include <format>
#include <fstream>
#include <sstream>
#include <string>
#include <functional>


namespace Pydantic{
using namespace std;
const string FILE_HEADER="from pydantic import BaseModel\n";

string getFieldName(const string& str){
    size_t pos =str.rfind('/');
    if (pos != string::npos){
        return str.substr(pos+1);
    }
    return str;
}

string generateSingleSchema(const map<string,string>& schema){
    string result;
    for (const auto& field:schema){
        if (field.second == "<Object>"){
//            Need to revisit this to create the correct name in nested objects
            string className = field.first + "Schema";
            className[0] = toupper(className[0]);
            result += "\nclass " + className + "(BaseModel):\n";
            map<string,string> subSchema;
            auto it = schema.lower_bound(field.first + "/");
            while (it != schema.end() && it->first.find(field.first + "/") == 0 ){
                subSchema.insert(*it);
                ++it;
            }
            result += generateSingleSchema(subSchema);
        } else {
            result += "    " + getFieldName(field.first) + ": " + field.second + "\n";
        }
    }
//    cout <<result <<endl;
    return result;
}

string generateMultipleSchemas(const map<string,string>& schema){
    string result = FILE_HEADER;
    result += generateSingleSchema(schema);
    cout << result;
    return "";
}


}



#endif /* pydanticSchemaGenerator_h */
