/**************************************************************************
 *
 */

#ifndef JTGENERATOR
    #define JTGENERATOR

#include "CodeGenerator.h"


class JTGenerator : public CodeGenerator
{
private:

    const FuncExtractor& fe;

    const FuncExtractor& libFuncsSL;

    char jtName[256];

    bool generateJTFields( std::ostream& fout, const FuncExtractor& fe );
    
    bool generateJumpTableStaticInit( std::ostream& f, const FuncExtractor& libFuncsSL,
                                       const char* jtName );

public:

    JTGenerator( const FuncExtractor& fe, const FuncExtractor& libFuncsSL, const char* oDir, const char* jtName );
    
    bool generateCode();

    void dumpInfo() const;
};

#endif // JTGENERATOR
