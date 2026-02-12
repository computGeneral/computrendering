/**************************************************************************
 *
 */

#ifndef SHADERPROGRAMTESTBASE_H
    #define SHADERPROGRAMTESTBASE_H

#include <vector>
#include <string>
#include "Vec4FP32.h"

namespace libGAL 
{
    //  class prototype (GALxProgramExecutionEnvironment.h included in implementation file)
    class GALxVP1ExecEnvironment;
    class GALxFP1ExecEnvironment;

    namespace libGAL_opt
    {
        //  class prototype (ShaderOptimizer.h included in implementation file)
        class ShaderOptimizer;
    }
}

namespace arch {

//  class prototype (ShaderInstruction.h included in implementation file)
class ShaderInstruction;

// From ConfigLoader.h (included in implementation .cpp file)
struct cgsArchConfig;

//  class prototype (bmShader.h included in implementation file)
class bmoShader;

//  class prototype (bmTexture.h included in implementation file)
class bmoTextureProcessor;

enum compileModel
{   
    OGL_ARB_1_0,
    D3D_SHADER_MODEL_2_0,
    D3D_SHADER_MODEL_3_0,
};

enum compileTarget
{
    SPT_VERTEX_TARGET,
    SPT_FRAGMENT_TARGET
};

struct ProgramInput
{
    float components[4];

    ProgramInput()
    {
        components[0] = components[1] = components[2] = components[3] = 0.0f;
    };
};

class ShaderProgramTestBase
{
private:
    
    libGAL::GALxVP1ExecEnvironment* arbvp10comp;
    libGAL::GALxFP1ExecEnvironment* arbfp10comp;

    libGAL::libGAL_opt::ShaderOptimizer* shOptimizer;

    // Current shader program state.
    bool loadedProgram;
    compileTarget target;
    compileModel shaderModel;
    Vec4FP32* programInputs;
    Vec4FP32* programConstants;

    // Program binary code storage.
    unsigned char* compiledCode;
    unsigned int codeSize;

    bmoShader* bmShader;
    bmoTextureProcessor* bmTexture;

    //  Auxiliary functions.
    void printCompiledProgram(std::vector<ShaderInstruction*> program, bool showBinary);

public:

    virtual std::string parseParams(const std::vector<std::string>& params)
    {
        return std::string(); // OK
    }

    ShaderProgramTestBase( const cgsArchConfig& arch );

    virtual bool finished() = 0;

    //  Console commands implementation

    void compileProgram(const std::string& code, bool optimize, bool transform, compileModel coFspModel);
    void defineProgramInput(unsigned int index, float* value);
    void defineProgramConstant(unsigned int index, float* value);
    void executeProgram(int stopPC);
    void dumpProgram(std::ostream& out);
};

} // namespace arch

#endif // SHADERPROGRAMTESTBASE_H
