/**************************************************************************
 *
 */

#include "GALxProgramExecutionEnvironment.h"
#include "GALxImplementationLimits.h"
#include "GALxSemanticTraverser.h"

using namespace std;
using namespace libGAL;

U08* GALxProgramExecutionEnvironment::compile(const string& code, U32& size_binary, GALxRBank<float>& clusterBank, ResourceUsage& resourceUsage)
{
    U08* binary = new U08[16*MAX_PROGRAM_INSTRUCTIONS_ARB*10];
    size_binary = 16*MAX_PROGRAM_INSTRUCTIONS_ARB*10;

    GenerationCode::GALxSemanticTraverser * sem;
    GALxShaderCodeGeneration* scg;    
    
    // Perform dependant compilation, depending on program type
    dependantCompilation((const U08 *)code.c_str(), (U32)code.size(), sem, scg); 
        
    scg->returnGPUCode(binary, size_binary); 
    
    clusterBank = scg->getParametersBank();

    resourceUsage.numberOfInstructions = sem->getNumberOfInstructions();
    resourceUsage.numberOfParamRegs = sem->getNumberOfParamRegs();
    resourceUsage.numberOfTempRegs = sem->getNumberOfTempRegs();
    resourceUsage.numberOfAddrRegs = sem->getNumberOfAddrRegs();
    resourceUsage.numberOfAttribs = sem->getNumberOfAttribs();

    for (int tu = 0; tu < MAX_TEXTURE_UNITS_ARB; tu++)
        resourceUsage.textureUnitUsage[tu] = sem->getTextureUnitTarget(tu);

    resourceUsage.killInstructions = sem->getKillInstructions();
    
    delete sem;    
    delete scg;

    return binary;
}
