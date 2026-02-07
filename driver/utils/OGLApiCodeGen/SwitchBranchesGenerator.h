/**************************************************************************
 *
 */

#ifndef SWITCHBRANCHESGENERATOR_H
    #define SWITCHBRANCHESGENERATOR_H

#include "CodeGenerator.h"

class SwitchBranchesGenerator : public CodeGenerator
{
private:

    const FuncExtractor& fe;
    char jt[256];
    char uct[256];
    char tr[256];

public:

    SwitchBranchesGenerator( const FuncExtractor& fe, const char* trName,
                             const char* jtName, const char* uct, const char* oDir );
    
    bool generateCode();

    void dumpInfo() const;
};


#endif // SWITCHBRANCHESGENERATOR_H
