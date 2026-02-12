/**************************************************************************
 *
 */

#include "ShaderProgramTest.h"
#include "ShaderProgramTestBase.h"
#include <iostream>
#include "SPTConsole.h"
#include "ConfigLoader.h"
#include "DynamicMemoryOpt.h"

using namespace arch;
using namespace std;

int main(int argc, char **argv)
{   
    cgsArchConfig arch;

    ConfigLoader cl("CG1GPU.ini");
    cl.getParameters(&arch);
    
    DynamicMemoryOpt::initialize(arch.objectSize0, arch.bucketSize0, arch.objectSize1, arch.bucketSize1,
        arch.objectSize2, arch.bucketSize2);

    SPTConsole* sptconsole = new SPTConsole(arch, "MySPTest");

    vector<string> params;
    if ( argc > 1 ) {
        for ( int i = 1; i < argc; ++i ) {
            params.push_back(argv[i]);
        }
    }

    sptconsole->parseParams(params);

    sptconsole->printWelcomeMessage();

    while ( !sptconsole->finished() ) 
    {
        sptconsole->execute();   
    }

    return 0;
}
