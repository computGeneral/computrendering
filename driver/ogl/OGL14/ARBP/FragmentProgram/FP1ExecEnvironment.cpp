/**************************************************************************
 *
 */

#include <io.h>
#include "ProgramExecutionEnvironment.h"
#include "support.h"
#include "FP1Bison.gen"
#include "SemanticTraverser.h"
#include "CodeGenTraverser.h"
#include "ShaderInstructionTranslator.h"

void FP1ExecEnvironment::dependantCompilation(const U08* code, U32 size, SemanticTraverser*& ssc, ShaderCodeGeneration*& scg)
{
    YY_BUFFER_STATE parseBufferHandler = fp1_scan_bytes((const char*)code,size);

    // Intermediate Representation creation
    
    IRProgram *irtree = new IRProgram();

    fp1StartParse((void *)irtree);

    // Deleting Input Buffer used by Flex to scan bytes.
    
    fp1_delete_buffer(parseBufferHandler);

    //cout << endl << "Printing tree: " << endl << irtree << endl;

    /**
    * SemanticTraverser & ShaderInstructionTranslator must inherit from classes ShaderSemanticCheck
    * & ShaderCodeGeneration. This classes are probably just interfaces... (are not still written)
    */

    SemanticTraverser* semtrav = new SemanticTraverser;

    // Semantic Analysis

    irtree->traverse(semtrav);

    if (semtrav->foundErrors())
    {
        cout << semtrav->getErrorString() << endl;
        CG_ASSERT("Semantic error in Fragment Program");
    }

    CodeGenTraverser cgtrav;

    // Generic code generation
    irtree->traverse(&cgtrav);

    delete irtree;
    
    //cgtrav.dumpGenericCode(cout);

    // GPU instructions generation

    /**
     * Hardware-dependent translator from generic code
     * to hardware-dependent code.
     */

    ShaderInstructionTranslator* shTranslator =
        new ShaderInstructionTranslator(cgtrav.getParametersBank(),
                                        cgtrav.getTemporariesBank());

    shTranslator->translateCode(cgtrav.getGenericCode(), true, true);

    ssc = semtrav;
    scg = shTranslator;

}
