/**************************************************************************
 *
 */

#include "stdafx.h"
#include "IniFile.h"
#include "D3DTraceConfig.h"

using namespace std;

D3DTraceConfig &D3DTraceConfig::instance() {
    static D3DTraceConfig me;
    return me;
}

void D3DTraceConfig::load(string filename) {
    ini.load(filename);
}

string D3DTraceConfig::getValue(std::string section, std::string variable) {
    IniFileSection *pSection = ini.getSection(section);
    if(pSection == 0) return "";
    else return pSection->getValue(variable);
}

bool D3DTraceConfig::existsVariable(std::string section, std::string variable)
{
    IniFileSection *pSection = ini.getSection(section);
    if(pSection == 0) return false;
    else return pSection->existsVariable(variable);
}
