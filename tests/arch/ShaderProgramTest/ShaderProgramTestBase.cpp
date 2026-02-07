/**************************************************************************
 *
 */

#include "ShaderProgramTestBase.h"
#include "ConfigLoader.h"
#include "GALxProgramExecutionEnvironment.h"
#include "ShaderOptimization.h"
#include "ShaderOptimizer.h"
#include "bmShader.h"
#include "bmTexture.h"
#include <limits>

// gene:
#define VS2_CONSTANT_NUM_REGS 64
#define VS2_INSTRUCTION_MEMORY_SIZE 32768

using namespace cg1gpu;
using namespace libGAL;

ShaderProgramTestBase::ShaderProgramTestBase( const cgsArchConfig& arch )
:  compiledCode(0), codeSize(0), loadedProgram(false)
{
    // Initialize all the Compilers and Shader Emulation classes

    //  Create ARB 1.0 compilers.
    arbvp10comp = new libGAL::GALxVP1ExecEnvironment;
    arbfp10comp = new libGAL::GALxFP1ExecEnvironment;

    libGAL_opt::SHADER_ARCH_PARAMS shArchPar;

    shArchPar.nWay = 4;
    shArchPar.shArchParams->selectArchitecture("SIMD4VarLat");
    shArchPar.temporaries = 32;

    //  Create the shader optimizer module (static instruction scheduling)
    shOptimizer = new libGAL_opt::ShaderOptimizer(shArchPar);

    //  Create texture behaviorModel object.
    bmTexture = new bmoTextureProcessor(
        STAMP_FRAGMENTS,                //  Fragments per stamp.  
        arch.ush.textBlockDim,          //  Texture block dimension (texels): 2^n x 2^n.  
        arch.ush.textSBlockDim,         //  Texture superblock dimension (blocks): 2^m x 2^m.  
        arch.ush.anisoAlgo,             //  Anisotropy algorithm selected.  
        arch.ush.forceMaxAniso,         //  Force the maximum anisotropy from the configuration file for all textures.  
        arch.ush.maxAnisotropy,         //  Maximum anisotropy allowed for any texture.  
        arch.ush.triPrecision,          //  Trilinear precision.  
        arch.ush.briThreshold,          //  Brilinear threshold.  
        arch.ush.anisoRoundPrec,        //  Aniso ratio rounding precision.  
        arch.ush.anisoRoundThres,       //  Aniso ratio rounding threshold.  
        arch.ush.anisoRatioMultOf2,     //  Aniso ratio must be multiple of two.            
        arch.ras.overScanWidth,         //  Over scan tile width (scan tiles).  
        arch.ras.overScanHeight,        //  Over scan tile height (scan tiles).  
        arch.ras.scanWidth,             //  Scan tile width (pixels).  
        arch.ras.scanHeight,            //  Scan tile height (pixels).  
        arch.ras.genWidth,              //  Generation tile width (pixels).  
        arch.ras.genHeight              //  Generation tile height (pixels).  
        );

    //  Create the shader behaviorModel object.
    bmShader = new bmoShader(
        "ShaderUnit",             //  Shader name.
        UNIFIED,                  //  Shader model. 
        1,                        //  Threads supported by the shader (state).
        1,                        //  Threads active in the shader (executable).
        bmTexture,                   //  Pointer to the texture behaviorModel attached to the shader.
        STAMP_FRAGMENTS           //  Fragments per stamp for texture accesses.
    );

    //  Initialize input attributes vector
    programInputs = new Vec4FP32[32];

    //  Initialize the constant registers;
    programConstants = new Vec4FP32[512];
}

void ShaderProgramTestBase::printCompiledProgram(vector<ShaderInstruction*> program, bool showBinary)
{
    //  Disassemble the program.
    for(unsigned int instr = 0; instr < program.size(); instr++)
    {
        char instrDisasm[256];
        unsigned char instrCode[16];

        //  Disassemble the instruction.
        program[instr]->disassemble(instrDisasm);

        //  Print instruction PC
        printf("%03d :", instr);

        if (showBinary)
        {
            //  Get the instruction code.
            program[instr]->getCode(instrCode);

            printf("%04x :", instr * 16);
            for(U32 b = 0; b < 16; b++)
                printf(" %02x", instrCode[b]);
        }

        //  Print the disassembled instruction.
        printf("    %s\n", instrDisasm);
    }
}

void ShaderProgramTestBase::compileProgram(const std::string& code, bool optimize, bool transform, compileModel coFspModel)
{
    vector<GALVector<gal_float,4> > constants;

    vector<ShaderInstruction*> compiledProgram;
    vector<ShaderInstruction*> optimizedProgram;
    vector<ShaderInstruction*> transformedProgram;

    ResourceUsage resourceUsage;
    unsigned int maxLiveTempRegs;

    cout << code;

    if (compiledCode)
        delete compiledCode;

    codeSize = 0;
    maxLiveTempRegs = 0;

    shaderModel = coFspModel;

    switch(shaderModel)
    {
        case OGL_ARB_1_0:
        {
            GALxRBank<float> constantBank(96,"");

            if (!code.compare( 0, 10, "!!ARBvp1.0" ))
            {    
                target = SPT_VERTEX_TARGET;
                compiledCode = arbvp10comp->compile(code, codeSize, constantBank, resourceUsage);
            }
            else if (!code.compare( 0, 10, "!!ARBfp1.0" ))
            {    
                target = SPT_FRAGMENT_TARGET;
                compiledCode = arbfp10comp->compile(code, codeSize, constantBank, resourceUsage);
            }
            else
                CG_ASSERT("Unexpected or not supported ARB 1.0 program target");
        
            constants.resize(constantBank.size());

            //  Copy constant values.
            for (unsigned int i = 0; i < constantBank.size(); i++)
            {
                constants[0] = constantBank.get(i)[0];
                constants[1] = constantBank.get(i)[1];   
                constants[2] = constantBank.get(i)[2];   
                constants[3] = constantBank.get(i)[3];   
            }

            break;
        }
        default:
            CG_ASSERT("Unexpected or not supported shader model");

    }
    // gene
    bool hasJumps = false;
    U32 numTemps = 64;
    // gene
    ShaderOptimization::decodeProgram(compiledCode, codeSize, compiledProgram, numTemps, hasJumps);

    //  Allocate space for the new transformed code.
    delete compiledCode;

    compiledCode = new unsigned char[compiledProgram.size() * ShaderInstruction::SHINSTRSIZE];

    if (!compiledCode)
    {
        CG_ASSERT("Could not allocate memory for compiled program");
    }

    codeSize = compiledProgram.size() * ShaderInstruction::SHINSTRSIZE;

    //  Encode binary.
    ShaderOptimization::encodeProgram(compiledProgram, compiledCode, codeSize);

    if (optimize)
    {        
        shOptimizer->setCode(compiledCode, codeSize);
        shOptimizer->setConstants(constants);

        shOptimizer->optimize();
      
        unsigned int optCodeSize;
        const gal_ubyte* optCode = shOptimizer->getCode(optCodeSize);
        
        U08* optimizedCode = new U08[optCodeSize];

        memcpy(optimizedCode, optCode, optCodeSize);
    
        //  Decode optimized program version again.
        ShaderOptimization::deleteProgram(compiledProgram);
        ShaderOptimization::decodeProgram(optimizedCode, optCodeSize, compiledProgram, numTemps, hasJumps); // gene add last two params

        delete optimizedCode;

        //  Perform low level driver optimizations.
        ShaderOptimization::optimize(compiledProgram, optimizedProgram, maxLiveTempRegs, false, false, false);
    
        //  Overwrite with the optimized version.
        ShaderOptimization::deleteProgram(compiledProgram);
        ShaderOptimization::copyProgram(optimizedProgram, compiledProgram);
    }
    
    //  Allocate space for the new transformed code.
    delete compiledCode;

    compiledCode = new unsigned char[compiledProgram.size() * ShaderInstruction::SHINSTRSIZE];

    if (!compiledCode)
    {
        CG_ASSERT("Could not allocate memory for compiled program");
    }

    codeSize = compiledProgram.size() * ShaderInstruction::SHINSTRSIZE;

    //  Encode binary.
    ShaderOptimization::encodeProgram(compiledProgram, compiledCode, codeSize);

    //  Set loaded flag.
    loadedProgram = true;

    //  Print compiled program.
    printCompiledProgram(compiledProgram, false);

    cout << "\nCompilation info: \n_________________\n";

    cout << "Number of instructions: " << compiledProgram.size();
    cout << "\nTotal program size: " << codeSize;
    cout << "\nMax ALive Registers: " << maxLiveTempRegs;
    cout << "\nALU:TEX Ratio: " << ShaderOptimization::getALUTEXRatio(compiledProgram) << "\n\n";

    //  Delete program.
    ShaderOptimization::deleteProgram(compiledProgram);
}

void ShaderProgramTestBase::defineProgramInput(unsigned int index, float* value)
{
    if (loadedProgram)
    {
        programInputs[index][0] = value[0];
        programInputs[index][1] = value[1];
        programInputs[index][2] = value[2];
        programInputs[index][3] = value[3];
    }
    else
    {
        cout << "Error: no shader program loaded." << endl;
    }
}

void ShaderProgramTestBase::defineProgramConstant(unsigned int index, float* value)
{
    unsigned int offset;

    if (loadedProgram)
    {
        if (target == FRAGMENT_TARGET)
        {
            switch(shaderModel)
            {
                case OGL_ARB_1_0: 
                case D3D_SHADER_MODEL_2_0:
                case D3D_SHADER_MODEL_3_0:

                    offset = VS2_CONSTANT_NUM_REGS;
                    
                    break;
                
                default:

                    offset = 0;
            }
        }
        else
        {
            offset = 0;
        }

        programConstants[index + offset][0] = value[0];
        programConstants[index + offset][1] = value[1];
        programConstants[index + offset][2] = value[2];
        programConstants[index + offset][3] = value[3];
    }
    else
    {
        cout << "Error: no shader program loaded." << endl;
    }
}

void ShaderProgramTestBase::executeProgram(int stopPC)
{
    ShaderInstruction::ShaderInstructionDecoded* instr;
    U32 partition = (target == SPT_VERTEX_TARGET)? VERTEX_PARTITION : FRAGMENT_PARTITION;
    U32 PC;
    bool lastInstr;
    U32 initialPC;
    bool killFlag[MAX_MSAA_SAMPLES];
    U32 i;

    if (loadedProgram)
    {
        //  Load current compiled shader program.
        bmShader->loadShaderProgram(compiledCode, VS2_INSTRUCTION_MEMORY_SIZE * partition, codeSize, partition);

        //  Initialize shader state (registers and kill flags set to zero/false).
        bmShader->resetShaderState(0);

        //  Load current input attributes
        bmShader->loadShaderState(0, IN, programInputs);
        
        //  Load shader program constants
        bmShader->loadShaderState(0, PARAM, programConstants);

        if (partition == FRAGMENT_PARTITION)
        {
            switch(shaderModel)
            {
                case OGL_ARB_1_0: 
                case D3D_SHADER_MODEL_2_0:
                case D3D_SHADER_MODEL_3_0:

                    initialPC = VS2_INSTRUCTION_MEMORY_SIZE;
                    
                    if (stopPC == -1)
                        stopPC = std::numeric_limits<int>::max();
                    else
                        stopPC += initialPC;
                    
                    break;
                
                default:
                    initialPC = 0;
            }
        }
        else
        {
            initialPC = 0;
        }

        lastInstr = false;

        PC = initialPC;

        //  Execution loop
        while (!lastInstr && PC < stopPC)
        {
            //  Fetch next instruction.
            instr = bmShader->fetchShaderInstruction(0, PC);

            //  Execute/emulate instruction.
            bmShader->execShaderInstruction(instr);

            //  Check if shader instruction was last.
            lastInstr = instr->getShaderInstruction()->isEnd();

            //  Increment PC.
            if (!lastInstr) PC++;
        }

        //  Allocate space to read registers.
        Vec4FP32* programTemps = new Vec4FP32[32];
        Vec4FP32* programOutputs = new Vec4FP32[32];

        //  Read temporary registers back.
        bmShader->readShaderState(0, TEMP, programTemps);

        //  Read output registers back.
        bmShader->readShaderState(0, OUT, programOutputs);

        //  Read per-sample thread kill state.
        for (i = 0; i < MAX_MSAA_SAMPLES; i++)
            killFlag[i] = bmShader->threadKill(0,i);

        //  Print registers.
        cout << "Shader state: PC=" << (PC - initialPC) << " kill vector (per sample): ";
        for (i = 0; i < MAX_MSAA_SAMPLES; i++)
            cout << "(" << i << "," << (killFlag[i]? "k":"a") << ")";
        
        cout << endl;
         
        cout << "Program temps" << endl;
        for (unsigned int i = 0; i < 32; i++)
        {
            cout << "{ " << programTemps[i][0];
            cout << ", " << programTemps[i][1];
            cout << ", " << programTemps[i][2];
            cout << ", " << programTemps[i][3];
            cout << "}" << endl;
        }

        cout << endl << "Program outputs" << endl;
        for (unsigned int i = 0; i < 32; i++)
        {
            cout << "{ " << programOutputs[i][0];
            cout << ", " << programOutputs[i][1];
            cout << ", " << programOutputs[i][2];
            cout << ", " << programOutputs[i][3];
            cout << "}" << endl;
        }

        delete[] programTemps;
        delete[] programOutputs; 
    }
    else
    {
        cout << "Error: no shader program loaded." << endl;
    }
}

void ShaderProgramTestBase::dumpProgram(ostream& out)
{
    if (loadedProgram)
    {
        out << hex;

        for (unsigned int i = 0; i < codeSize; i++)
        {
            if ((i % 16) == 0) out << endl;
            out << "0x" << (unsigned int) compiledCode[i] << ", ";
        }
    }
    else
    {
        cout << "Error: no shader program loaded." << endl;
    }

}
