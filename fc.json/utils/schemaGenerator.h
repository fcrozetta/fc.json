//
//  schemaGenerator.h
//  fc.json
//
//  Created by Fernando Crozetta on 21/10/2024.
//

#ifndef schemaGenerator_h
#define schemaGenerator_h

#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <functional>

using namespace std;

void printRawSchema(const unordered_map<string, string>& schema){
    for (const auto& field: schema){
        cout << field.first << endl;
    }
}


#endif /* schemaGenerator_h */
