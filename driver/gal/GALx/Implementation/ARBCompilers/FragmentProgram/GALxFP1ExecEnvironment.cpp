/**************************************************************************
 *
 */

#include "GALxProgramExecutionEnvironment.h"
#include "support.h"
#include "GALxFP1Bison.gen"
#include "GALxSemanticTraverser.h"
#include "GALxCodeGenTraverser.h"
#include "GALxShaderInstructionTranslator.h"

#include "GlobalProfiler.h"

void GALxFP1ExecEnvironment::dependantCompilation(const U08* code, U32 size, GALxSemanticTraverser *& ssc, GALxShaderCodeGeneration*& scg)
{
    YY_BUFFER_STATE parseBufferHandler = galxfp1_scan_bytes((const char*)code,size);

    // Intermediate Representation creation
    
    IRProgram *irtree = new IRProgram();

    GLOBAL_PROFILER_ENTER_REGION("FP compilation parser", "", "")
    GALxFp1StartParse((void *)irtree);
    GLOBAL_PROFILER_EXIT_REGION()

    // Deleting Input Buffer used by Flex to scan bytes.
    
    galxfp1_delete_buffer(parseBufferHandler);

    //cout << endl << "Printing tree: " << endl << irtree << endl;

   /**
    * GALxSemanticTraverser  & GALxShaderInstructionTranslator must inherit from classes ShaderSemanticCheck
    * & GALxShaderCodeGeneration. This classes are probably just interfaces... (are not still written)
    */

    GALxSemanticTraverser * semtrav = new GALxSemanticTraverser ;

    GLOBAL_PROFILER_ENTER_REGION("FP compilation semantic analysis", "", "")
    // Semantic Analysis

    irtree->traverse(semtrav);
    GLOBAL_PROFILER_EXIT_REGION()

    if (semtrav->foundErrors())
    {
        cout << semtrav->getErrorString() << endl;
        CG_ASSERT("Semantic error in Fragment Program");
    }

    GALxCodeGenTraverser cgtrav;

    GLOBAL_PROFILER_ENTER_REGION("FP compilation code generation", "", "")
    // Generic code generation
    irtree->traverse(&cgtrav);
    GLOBAL_PROFILER_EXIT_REGION()

    delete irtree;
    
    //cgtrav.dumpGenericCode(cout);

    // GPU instructions generation

    /**
     * Hardware-dependent translator from generic code
     * to hardware-dependent code.
     */

    GALxShaderInstructionTranslator* shTranslator =
        new GALxShaderInstructionTranslator(cgtrav.getParametersBank(),
                                        cgtrav.getTemporariesBank());

    GLOBAL_PROFILER_ENTER_REGION("FP compilation semantic code translation", "", "")
    shTranslator->translateCode(cgtrav.getGenericCode(), true, true);
    GLOBAL_PROFILER_EXIT_REGION()
    
    ssc = semtrav;
    scg = shTranslator;

}
