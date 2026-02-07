/**************************************************************************
 *
 */

#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>

#include "IniFile.h"
#include "StringUtils.h"

using namespace std;


void IniFileSection::setName(string s) {
    name = s;
}

string IniFileSection::getName() {
    return name;
}

bool IniFileSection::existsVariable(std::string variable) {
    return (variables.find(variable) != variables.end());
}

std::string IniFileSection::getValue(std::string variable) {
    return variables[variable];
}

void IniFileSection::assign(string variable, string value) {
    variables[variable] = value;
}

void IniFileSection::save(ostream &stream) {
    if(name != "")
        stream << '[' << name << ']' << endl;

    map<string, string>::iterator itVar;

    for(itVar = variables.begin(); itVar != variables.end(); itVar++) {
        stream << (*itVar).first << '=' << (*itVar).second << endl;
    }
        
}

bool IniFile::load(string filename) {
    ifstream f;
    f.open(filename.c_str());
    if(!f.is_open())
        return false;

    string line;
    IniFileSection *section = new IniFileSection();
    section->setName("");
    sections[""] = section;

    while(!f.eof()) {
        getline(f, line);
        if(isSectionName(line)) {
            section = new IniFileSection();
            section->setName(getSectionName(line));
            sections[section->getName()] = section;
        }
        if(isAssignation(line)) {
            section->assign(getVariable(line), getValue(line));
        }
    }

    f.close();

    return true;
}



bool IniFile::save(string filename) {
    ofstream f;
    f.open(filename.c_str());
    if(!f.is_open()) return false;
    
    map<string, IniFileSection *>::iterator itSec;

    for(itSec = sections.begin(); itSec != sections.end(); itSec++) {
        (*itSec).second->save(f);
    }

    f.close();

    return true;
}

IniFile::~IniFile()    {
    map<string, IniFileSection *>::iterator itSec;
    for(itSec = sections.begin(); itSec != sections.end(); itSec++) {
        delete (*itSec).second;
    }
}


IniFileSection *IniFile::getSection(string name) {
    return sections[name];
}

bool IniFile::isComentary(string s) {
    return (s.find_first_of(';') != s.npos);
}

bool IniFile::isSectionName(string s) {
    size_t iniMark = s.find_first_of('[');
    size_t endMark = s.find_last_of(']');
    return ((iniMark != s.npos) & (endMark != s.npos) & (iniMark < endMark));
}

string IniFile::getSectionName(string s) {
    size_t iniMark = s.find_first_of('[');
    size_t endMark = s.find_last_of(']');

    string result = s.substr(iniMark + 1, endMark - iniMark - 1);

    return trim(result);
}

bool IniFile::isAssignation(string s) {
    return (s.find_last_of('=') != s.npos);
}

string IniFile::getVariable(string s) {
    vector<string> v = explode(s, "=");
    return trim(v[0]);
}

string IniFile::getValue(string s) {
    vector<string> v = explode(s, "=");
    return trim(v[1]);
}

