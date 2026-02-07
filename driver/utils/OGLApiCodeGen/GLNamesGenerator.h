/**************************************************************************
 *
 */

#ifndef GLNAMESGENERATOR_H
    #define GLNAMESGENERATOR_H

#include "CodeGenerator.h"
#include "FuncExtractor.h"
#include "ConstExtractor.h"
#include <iostream>

class GLNamesGenerator : public CodeGenerator
{

private:

    const FuncExtractor& fe;
    const ConstExtractor& ce;

    char* oDir2;

    bool generateGLConstantNames( std::ostream& f, const ConstExtractor& ce );
    bool generateGLFunctionNames( std::ostream& f, const FuncExtractor& fe );
    bool generateGLFunctionNamesList( std::ostream& f, const FuncExtractor& fe, bool withCommas = true, const char* prefix = "");


public:

    GLNamesGenerator( const FuncExtractor& fe, const ConstExtractor& ce, 
                      const char* oDir1, const char* oDir2 = 0);

    bool generateCode();

    void dumpInfo() const;

    ~GLNamesGenerator();
};


#endif // GLNAMESGENERATOR_H
