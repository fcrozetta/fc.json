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

#include "../include/tabulate/tabulate.hpp"


using namespace std;
using namespace tabulate;

void printRawSchema(const map<string, string>& schema){
    for (const auto& field: schema){
        cout << field.first + " -> " + field.second<< endl;
    }
}

void printTableSchema(const map<string, string>& schema){
//    TODO: Improve the output here
    Table tb;
    tb.add_row({"Path","value"});
    for (const auto& field : schema){
        tb.add_row({field.first, field.second});
    }
    tb.format()
        .font_style({FontStyle::bold})
        .border_top(" ")
        .border_bottom(" ")
        .border_left(" ")
        .border_right(" ")
        .corner(" ");
    
    tb[0].format()
        .padding_top(1)
        .padding_bottom(1)
        .font_align(FontAlign::center)
        .font_style({FontStyle::underline})
        .font_background_color(Color::blue)
        .font_color(Color::white);
    tb[0][1].format()
        .font_background_color(Color::yellow);
    
    cout << tb <<endl;
    
}


#endif /* schemaGenerator_h */
