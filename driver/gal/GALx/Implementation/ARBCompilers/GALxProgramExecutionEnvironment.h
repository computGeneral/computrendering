/**************************************************************************
 *
 */

#ifndef GALx_PROGRAMEXECUTIONENVIRONMENT_H
    #define GALx_PROGRAMEXECUTIONENVIRONMENT_H

#include "GALxRBank.h"
#include "GALxImplementationLimits.h"
#include "GPUType.h"
#include <list>
#include <bitset>
#include <string>
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

namespace libGAL
{

namespace GenerationCode
{
    class GALxSemanticTraverser ;
}

struct ResourceUsage
{
    U32 numberOfInstructions;
    U32 numberOfParamRegs;
    U32 numberOfTempRegs;
    U32 numberOfAddrRegs;
    U32 numberOfAttribs;
    int textureUnitUsage [MAX_TEXTURE_UNITS_ARB];
    bool killInstructions;
};

// interface for code generators
class GALxShaderCodeGeneration
{
public:
    
    /**
     * Methods for retrieving results
     */
    virtual libGAL::GALxRBank<F32> getParametersBank() = 0;
    virtual void returnGPUCode(U08* bin, U32& size) = 0;
    
    virtual ~GALxShaderCodeGeneration() {}
};

class GALxProgramExecutionEnvironment
{
protected:
        
    /**
     * @param po Input Program Object
     * @retval ssc Semantic Check object returned after perform this method
     * @retval scg Code Generator object returned after perform this method
     */     
    virtual void dependantCompilation(const U08* code, U32 codeSize, GenerationCode::GALxSemanticTraverser *& ssc, GALxShaderCodeGeneration*& scg)=0;
    
public:

    // this method implements independent type program compilation 
    // Fetch rate indicates how many instructions can fetch the underlying architecture
    //void compile(ProgramObject& po);

    U08* compile(const std::string& code, U32& size_binary, libGAL::GALxRBank<float>& clusterBank, ResourceUsage& resourceUsage);

};

class GALxVP1ExecEnvironment : public GALxProgramExecutionEnvironment
{
    void dependantCompilation(const U08* code, U32 codeSize, GenerationCode::GALxSemanticTraverser *& ssc, GALxShaderCodeGeneration*& scg);
};

class GALxFP1ExecEnvironment : public GALxProgramExecutionEnvironment
{
    void dependantCompilation(const U08* code, U32 codeSize, GenerationCode::GALxSemanticTraverser *& ssc, GALxShaderCodeGeneration*& scg);
};

} // namespace libGAL

#endif // GALx_PROGRAMEXECUTIONENVIRONMENT_H
