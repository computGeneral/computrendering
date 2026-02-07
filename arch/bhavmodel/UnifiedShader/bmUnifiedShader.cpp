/**************************************************************************
 *
 * Shader Unit Behavior Model.
 * Implementes the bmoUnifiedShader class.  This class provides services
 * to emulate the functional behaviour of a Shader unit.
 *
 */

#include "bmUnifiedShader.h"
#include <cstring>
#include <cstdio>
#include <sstream>
#include "FixedPoint.h"
//#include "DebugDefinitions.h"

/*
 *  Shader Behavior Model constructor.
 *  This function creates a multithreaded Shader Behavior Model of the demanded model.
 */
using namespace cg1gpu;

bmoUnifiedShader::bmoUnifiedShader(char *shaderName, 
                                   bmeShaderModel shaderModel,
                                   cgeModelAbstractLevel ModelType,
                                   U32 numThreadsShader,
                                   bool storeDecInstr,
                                   U32 stampFrags,
                                   U32 fxpDecBits) :
    stampFragments(stampFrags), fxpDecBits(fxpDecBits), model(shaderModel),
    numThreads(numThreadsShader), storeDecodedInstr(storeDecInstr)
{
    //  Name and type of the shader are copied to the object attributes.
    name = new char[strlen(shaderName) + 1];
    strcpy(name, shaderName);

    //  Allocate the PC table and initialize it.
    PCTable = new U32[numThreads];
    for(U32 t = 0; t < numThreads; t++) //  Initialize the array of per-thread program counters.    
        PCTable[t] = 0;

    //  Allocate structures (register banks and instruction memory)
    //  for the type of shader defined.
    switch(model)
    {
        case UNIFIED:
            //  Instruction Memory.
            InstrMemory = new cgoShaderInstr*[2 * UNIFIED_INSTRUCTION_MEMORY_SIZE];
            for(U32 i = 0; i < (2 * UNIFIED_INSTRUCTION_MEMORY_SIZE); i++)
                InstrMemory[i] = new cgoShaderInstr(INVOPC);
            //  Check if storing decoded instructions is enabled.
            if (storeDecodedInstr)
            {
                InstrDecoded = new cgoShaderInstr::cgoShaderInstrEncoding**[2 * UNIFIED_INSTRUCTION_MEMORY_SIZE];
                for(U32 i = 0; i < (2 * UNIFIED_INSTRUCTION_MEMORY_SIZE); i++)
                {
                    InstrDecoded[i] = new cgoShaderInstr::cgoShaderInstrEncoding*[numThreads];
                    //  There is no shader program yet.  Set to NULL.
                    for(U32 j = 0; j < numThreads; j++)
                        InstrDecoded[i][j] = NULL;
                }
            }
            else
            {
                InstrDecoded = NULL;
            }
            instructionMemorySize = 2 * UNIFIED_INSTRUCTION_MEMORY_SIZE;
           
            inputBank = new Vec4FP32*[numThreads]; //  Input register bank.
            for(U32 i = 0; i < numThreads; i++)
                inputBank[i] = new Vec4FP32[UNIFIED_INPUT_NUM_REGS];
            numInputRegs = UNIFIED_INPUT_NUM_REGS;

            outputBank = new Vec4FP32*[numThreads]; //  Output register bank.
            for(U32 i = 0; i < numThreads; i++)
                outputBank[i] = new Vec4FP32[UNIFIED_OUTPUT_NUM_REGS];
            numOutputRegs = UNIFIED_OUTPUT_NUM_REGS;

            //  Temporary register bank.
            temporaryBank = new U08[numThreads * UNIFIED_TEMPORARY_NUM_REGS * UNIFIED_TEMP_REG_SIZE] ;
            numTemporaryRegs = UNIFIED_TEMPORARY_NUM_REGS;

            //  Constant register bank.
            constantBank = new U08[2 * UNIFIED_CONSTANT_NUM_REGS * UNIFIED_CONST_REG_SIZE];

            //  Check memory allocation.
            numConstantRegs = 2 * UNIFIED_CONSTANT_NUM_REGS;

            //  Address Register bank.
            addressBank = new Vec4Int*[numThreads];
            for(U32 i = 0; i < numThreads; i++)
                addressBank[i] = new Vec4Int[UNIFIED_ADDRESS_NUM_REGS];
            numAddressRegs = UNIFIED_ADDRESS_NUM_REGS;

            //  Predicate Register bank.
            predicateBank = new bool*[numThreads];
            for(U32 t = 0; t < numThreads; t++)
                predicateBank[t] = new bool[UNIFIED_PREDICATE_NUM_REGS];
            numPredicateRegs = UNIFIED_PREDICATE_NUM_REGS;
            break;

        case UNIFIED_MICRO:

            //  Instruction Memory.
            InstrMemory = new cgoShaderInstr*[2 * UNIFIED_INSTRUCTION_MEMORY_SIZE];
            for(U32 i = 0; i < (2 * UNIFIED_INSTRUCTION_MEMORY_SIZE); i++)
                InstrMemory[i] = new cgoShaderInstr(INVOPC);

            //  Check if storing decoded instructions is enabled.
            if (storeDecodedInstr)
            {
                InstrDecoded = new cgoShaderInstr::cgoShaderInstrEncoding**[2 * UNIFIED_INSTRUCTION_MEMORY_SIZE];

                for(U32 i = 0; i < (2 * UNIFIED_INSTRUCTION_MEMORY_SIZE); i++)
                {
                    InstrDecoded[i] = new cgoShaderInstr::cgoShaderInstrEncoding*[numThreads];
                    //  There is no shader program yet.  Set to NULL.
                    for(U32 j = 0; j < numThreads; j++)
                        InstrDecoded[i][j] = NULL;
                }
            }
            else
            {
                InstrDecoded = NULL;
            }
            
            instructionMemorySize = 2 * UNIFIED_INSTRUCTION_MEMORY_SIZE;

            //  Shader Input register bank.
            inputBank = new Vec4FP32*[numThreads];
            for(U32 i = 0; i < numThreads; i++)
            {
                inputBank[i] = new Vec4FP32[MAX_MICROFRAGMENT_ATTRIBUTES];
            }
            
            numInputRegs = MAX_MICROFRAGMENT_ATTRIBUTES;

            //  Output register bank.
            outputBank = new Vec4FP32*[numThreads];
            for(U32 i = 0; i < numThreads; i++)
            {
                outputBank[i] = new Vec4FP32[UNIFIED_OUTPUT_NUM_REGS];
            }
            
            numOutputRegs = UNIFIED_OUTPUT_NUM_REGS;

            //  Temporary register bank.
            temporaryBank = new U08[numThreads * UNIFIED_TEMPORARY_NUM_REGS * UNIFIED_TEMP_REG_SIZE] ;
            numTemporaryRegs = UNIFIED_TEMPORARY_NUM_REGS;

            //  Constant register bank.
            constantBank = new U08[2 * UNIFIED_CONSTANT_NUM_REGS * UNIFIED_CONST_REG_SIZE];
            numConstantRegs = 2 * UNIFIED_CONSTANT_NUM_REGS;

            //  Address Register bank.
            addressBank = new Vec4Int*[numThreads];

            for(U32 i = 0; i < numThreads; i++)
            {
                addressBank[i] = new Vec4Int[UNIFIED_ADDRESS_NUM_REGS];
            }
            numAddressRegs = UNIFIED_ADDRESS_NUM_REGS;

            //  Special Fixed Point accumulator bank used for rasterization.
            accumFXPBank = new FixedPoint*[numThreads];

            for(U32 i = 0; i < numThreads; i++)
            {
                accumFXPBank[i] = new FixedPoint[4];
            }
            
            //  Predicate Register bank.
            predicateBank = new bool*[numThreads];

            for(U32 t = 0; t < numThreads; t++)
            {
                predicateBank[t] = new bool[UNIFIED_PREDICATE_NUM_REGS];
            }
            
            numPredicateRegs = UNIFIED_PREDICATE_NUM_REGS;

            break;

        default:

            CG_ASSERT("Undefined Shader Model");
            break;
    }

    //  Initialize the per-sample kill flag array for all the threads.
    kill = new bool*[numThreads];

    //  Initialize each thread's per-sample kill flag array.
    for(U32 i = 0; i < numThreads; i++)
    {
        kill[i] = new bool[MAX_MSAA_SAMPLES];
        // Initialize per-thread flag array.
        for (U32 j = 0; j < MAX_MSAA_SAMPLES; j++)
        {
            kill[i][j] = false;
        }
    }

    //  Initialize the zexport thread (per sample) array.
    zexport = new F32*[numThreads];

    //  Initialize each thread's per-sample zexport array.
    for(U32 i = 0; i < numThreads; i++)
    {
        zexport[i] = new F32[MAX_MSAA_SAMPLES];

        // Initialize per-thread zexport array.
        for (U32 j = 0; j < MAX_MSAA_SAMPLES; j++)
        {
            zexport[i][j] = 0.0f;
        }
    }

    //  Initialize the sampleIdx thread array.
    sampleIdx = new U32[numThreads];
    // Initialize per-thread sampleIdx array.
    for (U32 i = 0; i < numThreads; i++)
    {
        sampleIdx[i] = 0;
    }

    //  Initialize texture queue state.
    numFree = TEXT_QUEUE_SIZE;
    numWait = 0;
    firstFree = lastFree = firstWait = lastWait = 0;

    //  Initialize list of free entries in the texture queue.
    for(U32 i = 0; i < TEXT_QUEUE_SIZE; i++)
    {
        freeTexture[i] = i;
    }

    //  Initialize Texture queue structures.
    for(U32 i = 0; i < TEXT_QUEUE_SIZE; i++)
    {
        //  Allocate structures for a whole stamp.
        textQueue[i].shInstrD = new cgoShaderInstr::cgoShaderInstrEncoding*[stampFragments];
        textQueue[i].coordinates = new Vec4FP32[stampFragments];
        textQueue[i].parameter = new F32[stampFragments];
        //textQueue[i].accessCookies = new DynamicObject;

        //  Check allocation.
        GPU_ASSERT(
            if (textQueue[i].shInstrD == NULL)
                CG_ASSERT("Error allocating array of instructions for texture queue entry.");
            if (textQueue[i].coordinates == NULL)
                CG_ASSERT("Error allocating array of texture coordinates for texture queue entry.");
            if (textQueue[i].parameter == NULL)
                CG_ASSERT("Error allocating array of per fragment parameter (lod/bias) for texture queue entry.");
            //if (textQueue[i].accessCookies == NULL)
            //    CG_ASSERT("Error allocating texture access cookies container.");
        )

        //  Reset requests counter.
        textQueue[i].requested = 0;
    }
    
    //  Initialize the current derivation operation structure.
    currentDerivation.baseThread = 0;
    currentDerivation.derived = 0;
    currentDerivation.shInstrD = new cgoShaderInstr::cgoShaderInstrEncoding*[4];
}

//  Resets the thread shader state.
void bmoUnifiedShader::resetShaderState(U32 numThread)
{
    Vec4FP32 auxQF;
    Vec4Int auxQI;
#if CG_ARCH_MODEL_DEVEL
    if(CurMdlType == CG_BEHV_MODEL)
        ArchModelSMKick();
#endif
    CG_ASSERT_COND((numThread < numThreads), "Incorrect thread number."); //  Check if it is a valid thread number.
    PCTable[numThread] = 0; //  Reset thread program C to zero.
    auxQF[0] =  auxQF[1] =  auxQF[2] =  auxQF[3] = 0.0f; //  Load Input Bank default value.
   
    for(U32 r = 0; r < numInputRegs; r++) //  Write default value in all Input Bank registers.
        inputBank[numThread][r] = auxQF;

    //  Load Output Bank default value.
    auxQF[0] =  auxQF[1] =  auxQF[2] =  auxQF[3] = 0.0f;

    //  Write default value in all Output Bank registers.
    for(U32 r = 0; r < numOutputRegs; r++)
        outputBank[numThread][r] = auxQF;

    //  Set temporary register bank to 0.
    memset(&temporaryBank[numThread * numTemporaryRegs * UNIFIED_TEMP_REG_SIZE], 0, numTemporaryRegs * UNIFIED_TEMP_REG_SIZE);

    //  Load the address register bank default value.
    auxQI[0] = auxQI[1] = auxQI[2] = auxQI[3] = 0;

    //  Write default value in all the Address Bank registers.
    for(U32 r = 0; r < numAddressRegs; r++)
        addressBank[numThread][r] = auxQI;

    //  Reset thread per-sample kill flag.
    for(U32 s = 0; s < MAX_MSAA_SAMPLES; s++)
        kill[numThread][s] = false;

    //  Reset thread per-sample zexport values.
    for(U32 s = 0; s < MAX_MSAA_SAMPLES; s++)
        zexport[numThread][s] = 0.0f;

    //  Reset thread´s sample index value.
    sampleIdx[numThread] = 0;
}

//  Loads a value into one of the thread registers.  Vec4FP32 version.
void bmoUnifiedShader::loadShaderState(U32 numThread, Bank bank, U32 reg, Vec4FP32 data)
{
    switch(bank) //  Load the register in the appropiate register bank.
    {
        case IN:
            //  Check the range of the register index.
            CG_ASSERT_COND((reg < numInputRegs), "loadShaderState(Vec4FP32) Incorrect register number.");
            //  Check the range of the thread index.
            CG_ASSERT_COND((numThread < numThreads), "loadShaderState(Vec4FP32) Illegal thread number.");
            //  Write the value into the selected register
            inputBank[numThread][reg] = data;
            break;

        case PARAM:
            //  Check the range of the register index.
            CG_ASSERT_COND((reg < numConstantRegs), "loadShaderState(Vec4FP32) Incorrect register number.");
            //  Write the value into the selected register.
            ((F32 *) constantBank)[reg * (UNIFIED_CONST_REG_SIZE/4) + 0] = data[0];
            ((F32 *) constantBank)[reg * (UNIFIED_CONST_REG_SIZE/4) + 1] = data[1];
            ((F32 *) constantBank)[reg * (UNIFIED_CONST_REG_SIZE/4) + 2] = data[2];
            ((F32 *) constantBank)[reg * (UNIFIED_CONST_REG_SIZE/4) + 3] = data[3];
            break;
        default:
            CG_ASSERT("loadShaderState(Vec4FP32) Access not allowed.");
            break;

    }
}

//  Loads a value into one of the thread registers.  Vec4Int version.
void bmoUnifiedShader::loadShaderState(U32 numThread, Bank bank, U32 reg, Vec4Int data)
{
    //  Load the register in the appropiate register bank.
    switch(bank)
    {
        case PARAM:
        
            //  Check the range of the register index.
            CG_ASSERT_COND(!(reg >= numConstantRegs), "Incorrect register number.");
            //  Write the value into the selected register.
            ((U32 *) constantBank)[reg * (UNIFIED_CONST_REG_SIZE/4) + 0] = data[0];
            ((U32 *) constantBank)[reg * (UNIFIED_CONST_REG_SIZE/4) + 1] = data[1];
            ((U32 *) constantBank)[reg * (UNIFIED_CONST_REG_SIZE/4) + 2] = data[2];
            ((U32 *) constantBank)[reg * (UNIFIED_CONST_REG_SIZE/4) + 3] = data[3];

            break;

        default:

            CG_ASSERT("Access not allowed.");
            break;

    }
}

//  Loads a value into one of the thread registers.  Boolean version.
void bmoUnifiedShader::loadShaderState(U32 numThread, Bank bank, U32 reg, bool data)
{
    CG_ASSERT("Not implemented.");
}

//  Loads new values into one of the thread register banks.  Vec4FP32 version.
void bmoUnifiedShader::loadShaderState(U32 numThread, Bank bank, Vec4FP32 *data, U32 startReg, U32 nRegs)
{
    switch(bank)
    {
        case IN:
        
            //  Check the range of the thread identifier.
            CG_ASSERT_COND(!(numThread >= numThreads), "Illegal thread number.");            
            //  Check the range of the registers to write.
            CG_ASSERT_COND(!((startReg >= numInputRegs) || ((startReg + nRegs) > numInputRegs)), "Registers to load are out of range.");
            //  Write all registers in the bank with the new values.
            for(U32 r = startReg; r < (startReg + nRegs); r++)
                inputBank[numThread][r] = data[r - startReg];

            break;

        case PARAM:

            //  Check the range of the registers to write.
            CG_ASSERT_COND(!((startReg >= numConstantRegs) || ((startReg + nRegs) > numConstantRegs)), "Registers to load are out of range.");
            //  Write all registers in the bank with the new values.
            for(U32 r = startReg; r < (startReg + nRegs); r++)
            {
                ((F32 *) constantBank)[r * (UNIFIED_CONST_REG_SIZE/4) + 0] = data[r - startReg][0];
                ((F32 *) constantBank)[r * (UNIFIED_CONST_REG_SIZE/4) + 1] = data[r - startReg][1];
                ((F32 *) constantBank)[r * (UNIFIED_CONST_REG_SIZE/4) + 2] = data[r - startReg][2];
                ((F32 *) constantBank)[r * (UNIFIED_CONST_REG_SIZE/4) + 3] = data[r - startReg][3];
            }
            
            break;

        default:
            CG_ASSERT("Access not allowed.");
            break;
    }
}

//  Loads values into all the registers of a thread register bank.
void bmoUnifiedShader::loadShaderState(U32 numThread, Bank bank, Vec4FP32 *data)
{
    switch(bank)
    {
        case IN:
        
            //  Check if the thread number is correct.
            CG_ASSERT_COND(!(numThread >= numThreads), "Illegal thread number.");
            //  Write all registers in the bank with the new values.
            for(U32 r = 0; r < numInputRegs; r++)
                inputBank[numThread][r] = data[r];

            break;

        default:
            CG_ASSERT("Access not allowed.");
            break;
    }

}


//  Reads the value of one of the thread registers.  Vec4FP32 version.
void bmoUnifiedShader::readShaderState(U32 numThread, Bank bank, U32 reg, Vec4FP32 &value)
{
    //  Select the bank for the register.
    switch(bank)
    {
        case IN:
        
            //  Check the range of the register identifier.
            CG_ASSERT_COND(!(reg >= numInputRegs), "Incorrect register number.");
            //  Check the range of the thread identifier.
            CG_ASSERT_COND(!(numThread >= numThreads), "Illegal thread number.");
            //  Read the value from the input register bank,
            value = inputBank[numThread][reg];

            break;

        case OUT:
        
            //  Check the range of the register identifier.
            CG_ASSERT_COND(!(reg >= numOutputRegs), "Incorrect register number.");
            //  Check the range of the thread identifier.
            CG_ASSERT_COND(!(numThread >= numThreads), "Illegal thread number.");
            //  Read the value from the output register bank,
            value = outputBank[numThread][reg];

            break;

        case PARAM:
        
            //  Check the range of the register identifier.
            CG_ASSERT_COND(!(reg >= numConstantRegs), "Incorrect register number.");
            //  Read the value from the constant register bank.
            value[0] = ((F32 *) constantBank)[reg * (UNIFIED_CONST_REG_SIZE/4) + 0];
            value[1] = ((F32 *) constantBank)[reg * (UNIFIED_CONST_REG_SIZE/4) + 1];
            value[2] = ((F32 *) constantBank)[reg * (UNIFIED_CONST_REG_SIZE/4) + 2];
            value[3] = ((F32 *) constantBank)[reg * (UNIFIED_CONST_REG_SIZE/4) + 3];

            break;

        case TEMP:
        
            //  Check the range of the register identifier.
            CG_ASSERT_COND(!(reg >= numTemporaryRegs), "Incorrect register number.");
            //  Check the range of the thread identifier.
            CG_ASSERT_COND(!(numThread >= numThreads), "Illegal thread number.");
            //  Read the value from the temprary register bank.
            value[0] = ((F32 *) temporaryBank)[(numThread * numTemporaryRegs + reg) * (UNIFIED_TEMP_REG_SIZE/4) + 0];
            value[1] = ((F32 *) temporaryBank)[(numThread * numTemporaryRegs + reg) * (UNIFIED_TEMP_REG_SIZE/4) + 1];
            value[2] = ((F32 *) temporaryBank)[(numThread * numTemporaryRegs + reg) * (UNIFIED_TEMP_REG_SIZE/4) + 2];
            value[3] = ((F32 *) temporaryBank)[(numThread * numTemporaryRegs + reg) * (UNIFIED_TEMP_REG_SIZE/4) + 3];

            break;

        default:
        
            //  Access not allowed.
            CG_ASSERT("Access not allowed.");

            break;
    }
}

//  Reads the value of one of the thread registers.  Vec4Int version.
void bmoUnifiedShader::readShaderState(U32 numThread, Bank bank, U32 reg, Vec4Int &value)
{
    //  Select the register bank.
    switch(bank)
    {
        case ADDR:
        
            //  Check the range of the register identifier.
            CG_ASSERT_COND(!(reg >= numAddressRegs), "Incorrect register number.");
            //  Check the range of the thread identifier.
            CG_ASSERT_COND(!(numThread >= numThreads), "Illegal thread number.");
            value = addressBank[numThread][reg];

            break;

        case PARAM:
        
            //  Check the range of the register identifier.
            CG_ASSERT_COND(!(reg >= numConstantRegs), "Incorrect register number.");
            //  Read the value from the constant register bank.
            value[0] = ((U32 *) constantBank)[reg * (UNIFIED_CONST_REG_SIZE/4) + 0];
            value[1] = ((U32 *) constantBank)[reg * (UNIFIED_CONST_REG_SIZE/4) + 1];
            value[2] = ((U32 *) constantBank)[reg * (UNIFIED_CONST_REG_SIZE/4) + 2];
            value[3] = ((U32 *) constantBank)[reg * (UNIFIED_CONST_REG_SIZE/4) + 3];

            break;

        case TEMP:
        
            //  Check the range of the register identifier.
            CG_ASSERT_COND(!(reg >= numTemporaryRegs), "Incorrect register number.");
            //  Check the range of the thread identifier.
            CG_ASSERT_COND(!(numThread >= numThreads), "Illegal thread number.");
            //  Read the value from the temprary register bank.
            value[0] = ((U32 *) temporaryBank)[(numThread * numTemporaryRegs + reg) * (UNIFIED_TEMP_REG_SIZE/4) + 0];
            value[1] = ((U32 *) temporaryBank)[(numThread * numTemporaryRegs + reg) * (UNIFIED_TEMP_REG_SIZE/4) + 1];
            value[2] = ((U32 *) temporaryBank)[(numThread * numTemporaryRegs + reg) * (UNIFIED_TEMP_REG_SIZE/4) + 2];
            value[3] = ((U32 *) temporaryBank)[(numThread * numTemporaryRegs + reg) * (UNIFIED_TEMP_REG_SIZE/4) + 3];

            break;

        default:
        
            //  Access not allowed.
            CG_ASSERT("Access not allowed.");
            break;

    }
}

//  Reads the value of one of the thread registers.  Boolean version.
void bmoUnifiedShader::readShaderState(U32 numThread, Bank bank, U32 reg, bool &value)
{
    CG_ASSERT("Unimplemented.");
}

//  Reads the values of one of the thread register banks.  Vec4FP32 version.
void bmoUnifiedShader::readShaderState(U32 numThread, Bank bank, Vec4FP32 *data)
{
    //  Select register bank.
    switch(bank)
    {
        case IN:
            
            //  Check the range of the thread identifier.
            CG_ASSERT_COND(!(numThread >= numThreads), "Illegal thread number.");
            //  Reads all the registers in the input bank.
            for (U32 r = 0; r < numInputRegs; r++)
                data[r] = inputBank[numThread][r];

            break;

        case OUT:

            //  Check the range of the thread identifier.
            CG_ASSERT_COND(!(numThread >= numThreads), "Illegal thread number.");
            //  Reads all the registers in the output bank.
            for (U32 r = 0; r < numOutputRegs; r++)
                data[r] = outputBank[numThread][r];

            break;

        case PARAM:

            //  Read all the registers in the constants memory.
            for (U32 r = 0; r < numConstantRegs; r++)
            {
                data[r][0] = ((F32 *) constantBank)[r * (UNIFIED_CONST_REG_SIZE/4) + 0];
                data[r][1] = ((F32 *) constantBank)[r * (UNIFIED_CONST_REG_SIZE/4) + 1];
                data[r][2] = ((F32 *) constantBank)[r * (UNIFIED_CONST_REG_SIZE/4) + 2];
                data[r][3] = ((F32 *) constantBank)[r * (UNIFIED_CONST_REG_SIZE/4) + 3];
            }
            
            break;

        case TEMP:

            //  Check the range of the thread identifier.
            CG_ASSERT_COND(!(numThread >= numThreads), "Illegal thread number.");
            //  Read all the registers in the temporary register bank.
            for (U32 r = 0; r < numTemporaryRegs; r++)
            {
                //  Read the value from the temprary register bank.
                data[r][0] = ((F32 *) temporaryBank)[(numThread * numTemporaryRegs + r) * (UNIFIED_TEMP_REG_SIZE/4) + 0];
                data[r][1] = ((F32 *) temporaryBank)[(numThread * numTemporaryRegs + r) * (UNIFIED_TEMP_REG_SIZE/4) + 1];
                data[r][2] = ((F32 *) temporaryBank)[(numThread * numTemporaryRegs + r) * (UNIFIED_TEMP_REG_SIZE/4) + 2];
                data[r][3] = ((F32 *) temporaryBank)[(numThread * numTemporaryRegs + r) * (UNIFIED_TEMP_REG_SIZE/4) + 3];
            }
            
            break;

        default:
        
            CG_ASSERT("Access not allowed.");
            break;
    }

}

//  Reads the values of one of the thread register banks.  Vec4Int version.
void bmoUnifiedShader::readShaderState(U32 numThread, Bank bank, Vec4Int *data)
{
    //  Select bank to read.
    switch(bank)
    {
        case ADDR:

            //  Check the range of the thread identifier.
            CG_ASSERT_COND(!(numThread >= numThreads), "Illegal thread number.");
            //  Read all the registers in the address bank.
            for (U32 r = 0; r < numAddressRegs; r++)
                data[r] = addressBank[numThread][r];

            break;

        case PARAM:

            //  Read all the registers in the constants memory.
            for (U32 r = 0; r < numConstantRegs; r++)
            {
                data[r][0] = ((U32 *) constantBank)[r * (UNIFIED_CONST_REG_SIZE/4) + 0];
                data[r][1] = ((U32 *) constantBank)[r * (UNIFIED_CONST_REG_SIZE/4) + 1];
                data[r][2] = ((U32 *) constantBank)[r * (UNIFIED_CONST_REG_SIZE/4) + 2];
                data[r][3] = ((U32 *) constantBank)[r * (UNIFIED_CONST_REG_SIZE/4) + 3];
            }
            
            break;

        case TEMP:

            //  Check the range of the thread identifier.
            CG_ASSERT_COND(!(numThread >= numThreads), "Illegal thread number.");
            //  Read all the registers in the temporary register bank.
            for (U32 r = 0; r < numTemporaryRegs; r++)
            {
                //  Read the value from the temprary register bank.
                data[r][0] = ((U32 *) temporaryBank)[(numThread * numTemporaryRegs + r) * (UNIFIED_TEMP_REG_SIZE/4) + 0];
                data[r][1] = ((U32 *) temporaryBank)[(numThread * numTemporaryRegs + r) * (UNIFIED_TEMP_REG_SIZE/4) + 1];
                data[r][2] = ((U32 *) temporaryBank)[(numThread * numTemporaryRegs + r) * (UNIFIED_TEMP_REG_SIZE/4) + 2];
                data[r][3] = ((U32 *) temporaryBank)[(numThread * numTemporaryRegs + r) * (UNIFIED_TEMP_REG_SIZE/4) + 3];
            }

            break;
            
        default:
            
            CG_ASSERT("Access not allowed..");
            break;
    }
}


//  Reads the values of one of the thread register banks.  Vec4Int version.
void bmoUnifiedShader::readShaderState(U32 numThread, Bank bank, bool *data)
{
    CG_ASSERT("Not Implemented");
}


//  Load a shader program into the shader instruction memory and decode the shader program
//  for all the threads in the shader (expensive!!!).
void bmoUnifiedShader::loadShaderProgram(U08 *code, U32 address, U32 sizeCode, U32 partition)
{
    cgoShaderInstr *shInstr;

    //  Check.  The size of the code must be a multiple of the instruction size.
    CG_ASSERT_COND(!(((sizeCode & cgoShaderInstr::CG1_ISA_INSTR_SIZE_MASK) != 0) && (sizeCode > 0)), "Shader Program incorrect size.");
    //  Check the program address range.
    CG_ASSERT_COND(!(address >= instructionMemorySize ), "Out of range shader program load address.");
    //  Check if the Shader Program is too large for the instruction memory.
    CG_ASSERT_COND(!((address + (sizeCode >> cgoShaderInstr::CG1_ISA_INSTR_SIZE_LOG)) > instructionMemorySize), "Shader Program too large for Instruction Memory.");
//char buffer[200];
//printf("bmShader => loadShaderProgram address %x size %d\n", address, sizeCode);

    //  Load the shader program.
    for(U32 i = 0; i < (sizeCode >> cgoShaderInstr::CG1_ISA_INSTR_SIZE_LOG); i++)
    {
        //  Delete the old instruction.
        if (InstrMemory[i + address] != NULL)
            delete InstrMemory[i + address];

        //  Create and decode the new instruction.
        shInstr = InstrMemory[i + address] = new cgoShaderInstr(&code[i << cgoShaderInstr::CG1_ISA_INSTR_SIZE_LOG]);

//InstrMemory[i + address][j]->disassemble(buffer);
//printf("%04x : %s\n", i + address, buffer);

        //  Check if storing decoded shader instructions is enabled.
        if (storeDecodedInstr)
        {
            //  Decode the instruction for the 
            for(U32 t = 0; t < numThreads; t++)
            {
                //  Destroy the old decoded instruction.
                if (InstrDecoded[i + address][t] != NULL)
                    delete InstrDecoded[i + address][t];

                //  Decode the shader instruction for the thread.
                InstrDecoded[i + address][t] = DecodeInstr(shInstr, i + address, t, partition);
            }
        }
    }
}

//  Decode a shader instruction.
cgoShaderInstr::cgoShaderInstrEncoding *bmoUnifiedShader::DecodeInstr(cgoShaderInstr *shInstr,
                                                                      U32 address,
                                                                      U32 nThread,
                                                                      U32 partition)
{
    U32 numOps;  //  Number of instruction operands.
    cgoShaderInstr::cgoShaderInstrEncoding *shInstrDec;   //  Pointer to the Shader Instruction decoded per thread.

    //  Create and decode the new instruction.
    shInstrDec = new cgoShaderInstr::cgoShaderInstrEncoding(shInstr, address, nThread);

    //  Decode the registers that the instruction is going to use when executed.
    numOps = shInstr->getNumOperands();

    //  Get first operand address.
    if (numOps > 0)
    {
        if (shInstr->getRelativeModeFlag() && (shInstr->getBankOp1() == PARAM))
        {
            //  If this operand is a constant accessed through an address register
            //  load the address register in the first operand register pointer.
            shInstrDec->setShEmulOp1(decodeOpReg(nThread, ADDR, shInstr->getRelMAddrReg(), partition));
        }
        else
            shInstrDec->setShEmulOp1(decodeOpReg(nThread, shInstr->getBankOp1(), shInstr->getOp1(), partition));
    }

    //  Get second operand address.
    if (numOps > 1)
    {
        if (shInstr->getRelativeModeFlag() && (shInstr->getBankOp2() == PARAM))
        {
            //  If this operand is a constant accessed through an address register
            //  load the address register in the second operand register pointer.
            shInstrDec->setShEmulOp2(decodeOpReg(nThread, ADDR, shInstr->getRelMAddrReg(), partition));
        }
        else if (shInstr->getBankOp2() == IMM)
        {        
            //  Replicate four times the instruction 32-bit immediate value to the decoded instruction local store.
            U08 *data = shInstrDec->getOp2Data();
            U32 immediate = shInstr->getImmediate();
            for(U32 t = 0; t < 4; t++)
                ((U32 *) data)[t] = immediate;
            
            //  Set the decoded instruction to read the second operand from local data.
            shInstrDec->setShEmulOp2(0);
        }
        else
            shInstrDec->setShEmulOp2(decodeOpReg(nThread, shInstr->getBankOp2(), shInstr->getOp2(), partition));
    }

    //  Get third operand address.
    if (numOps > 2)
    {
        if (shInstr->getRelativeModeFlag() && (shInstr->getBankOp3() == PARAM))
        {
            //  If this operand is a constant accessed through an address register
            //    load the address register in the first operand register pointer.
            shInstrDec->setShEmulOp3(decodeOpReg(nThread, ADDR, shInstr->getRelMAddrReg()));
        }
        else
            shInstrDec->setShEmulOp3(decodeOpReg(nThread, shInstr->getBankOp3(), shInstr->getOp3(), partition));
    }

    if (shInstr->hasResult())
    {
        //  Get result operand address.
        shInstrDec->setShEmulResult(decodeOpReg(nThread, shInstr->getBankRes(), shInstr->getResult(), partition));
    }

    //  Decode the instruction predication.
    if (shInstr->getPredicatedFlag())
        shInstrDec->setShEmulPredicate((void *) &predicateBank[nThread][shInstr->getPredicateReg()]);

    //  Decodes and sets the function that will be used to emulate the instruction when executed.
    setEmulFunction(shInstrDec);

    return shInstrDec;
}

//  Load a shader program into the shader instruction memory.
void bmoUnifiedShader::loadShaderProgramLight(U08 *code, U32 address, U32 sizeCode)
{
    cgoShaderInstr *shInstr;     //  Pointer to the Shader Instruction.

    //  Check.  The size of the code must be a multiple of the instruction size.
    CG_ASSERT_COND(!(((sizeCode & cgoShaderInstr::CG1_ISA_INSTR_SIZE_MASK) != 0) && (sizeCode > 0)), "Shader Program incorrect size.");
    //  Check the program address range.
    CG_ASSERT_COND(!(address >= instructionMemorySize), "Out of range shader program load address.");
    //  Check if the Shader Program is too large for the instruction memory.
    CG_ASSERT_COND(!((address + (sizeCode >> cgoShaderInstr::CG1_ISA_INSTR_SIZE_LOG)) > instructionMemorySize), "Shader Program too large for Instruction Memory.");
//char buffer[200];
//printf("bmShader => loadShaderProgram address %x size %d\n", address, sizeCode);

    //  Load the shader program instructions.
    for(U32 i = 0; i < (sizeCode >> cgoShaderInstr::CG1_ISA_INSTR_SIZE_LOG); i++)
    {
        //  Destroy the old instructions.
        if (InstrMemory[i + address] != NULL)
            delete InstrMemory[i + address];

        //  Create and decode the new instruction.  
        // i << 4 (i * 16) is the position of the first byte of the next instruction.  
//printf("Byte Code : ");
//for(int cc = 0; cc < 16; cc++)
//printf("%02x ", code[(i << cgoShaderInstr::CG1_ISA_INSTR_SIZE_LOG) + cc]);
//printf("\n");
        shInstr = InstrMemory[i + address] = new cgoShaderInstr(&code[i << cgoShaderInstr::CG1_ISA_INSTR_SIZE_LOG]);

//InstrMemory[i + address]->disassemble(buffer);
//printf("%04x : %s\n", i + address, buffer);
    }
}

// Decodes and returns the address to the register that the instruction is
// going to access when executed (per thread!)
void *bmoUnifiedShader::decodeOpReg(U32 numThread, Bank bank, U32 reg, U32 partition)
{
    U32 offset;

    switch(bank)
    {
        case IN:
        
            //  Get the input register address.
            return (void *) inputBank[numThread][reg].getVector();
            break;

        case OUT:
        
            // Get the output register address.
            return (void *) outputBank[numThread][reg].getVector();
            break;

        case PARAM:
        
            //  WARNING:  RELATIVE MODE ADDRESSING IN THE CONSTANT BANK MUST
            //  BE EMULATED AT RUNTIME (ACCESS IS RELATIVE TO A REGISTER).

            offset = (partition == FRAGMENT_PARTITION)? UNIFIED_CONSTANT_NUM_REGS:(partition == TRIANGLE_PARTITION)?200:0;

            //  Get constant address.  Add offset for shader partition constant bank.
            return (void *) &constantBank[(reg + offset) * UNIFIED_CONST_REG_SIZE];
            break;

        case TEMP:
        
            //  Get temporary register address.
            return (void *) &temporaryBank[(numThread * numTemporaryRegs + reg) * UNIFIED_TEMP_REG_SIZE];
            break;

        case ADDR:
        
            //   Get address register address.
            return (void *) &addressBank[numThread][reg];
            break;

        case PRED:
            return (void *) &predicateBank[numThread][reg];
            break;

        case TEXT:

            //  Texture unit identifier, not a register.
            return NULL;
            break;

        case SAMP:
            
            //  Fragment sample identifier, not a register.            
            return NULL;
            break;
    }

    CG_ASSERT("Unimplemented register bank.");
    return 0;
}


//  Sets the pointer to the emulated function for the instruction in the tabled for predecoded instructions.
void bmoUnifiedShader::setEmulFunction(cgoShaderInstr::cgoShaderInstrEncoding *shInstrDec)
{
    //  Store the emulation function.
    shInstrDec->setEmulFunc(shInstrEmulationTable[shInstrDec->getShaderInstruction()->getOpcode()]);
}


//  Returns the instruction in the Instruction Memory pointed by PC.
cgoShaderInstr::cgoShaderInstrEncoding *bmoUnifiedShader::FetchInstr(U32 threadId, U32 PC)
{

    //  Check if the PC and the thread number is valid.
    CG_ASSERT_COND((PC < instructionMemorySize), "PC:%x overflows instruction memory size %d", PC, instructionMemorySize);
    CG_ASSERT_COND((threadId < numThreads), "Thread number not valid.");
    //  Check if storing decoded instructions is enabled.
    if (storeDecodedInstr)
    {
        //  Check if the instruction is already decoded.
        if (InstrDecoded[PC][threadId] == NULL)
            InstrDecoded[PC][threadId] = DecodeInstr(InstrMemory[PC], PC, threadId, (PC / UNIFIED_INSTRUCTION_MEMORY_SIZE));//  Decode on demand.
        //  Return the instruction pointed.
        return InstrDecoded[PC][threadId];
    }
    else
    {
        cgoShaderInstr::cgoShaderInstrEncoding *decInstr;
        decInstr = DecodeInstr(InstrMemory[PC], PC, threadId, (PC / UNIFIED_INSTRUCTION_MEMORY_SIZE));
        return decInstr;
    }
}

//  Fetch and decode a shader instruction for the corresponding thread.
cgoShaderInstr::cgoShaderInstrEncoding *bmoUnifiedShader::fetchShaderInstructionLight(U32 threadId, U32 PC, U32 partition)
{
    GPU_ASSERT(
        //  Check the instruction address range.
        if (PC >= instructionMemorySize)
        {
            std::stringstream ss;
            ss << "PC (" << PC << ") overflows instruction memory size (" << instructionMemorySize << ").";
            CG_ASSERT(ss.str().c_str());
        }

        //  Check the range of the thread identifier.
        if (threadId >= numThreads)
        {
            CG_ASSERT("Thread number not valid.");
        }

        //  Check if the instruction is defined.
        if (InstrMemory[PC] == NULL)
        {
            char buffer[128];
            sprintf(buffer, "Shader instruction not defined for partition %d Thread %d PC %x", partition,
                threadId, PC);
            CG_ASSERT(buffer);
        }
    )

    //  Fetch and decode the shader instruction.
    return DecodeInstr(InstrMemory[PC], PC, threadId, partition);
}

//  Read a shader instruction from shader instruction memory.
cgoShaderInstr *bmoUnifiedShader::readShaderInstruction(U32 pc)
{
    return InstrMemory[pc];
}


//  Executes a Shader Instruction.
void bmoUnifiedShader::execShaderInstruction(cgoShaderInstr::cgoShaderInstrEncoding *shInstrDec)
{
    //  Check the range of the thread identifier.
    CG_ASSERT_COND((shInstrDec->getNumThread() < numThreads), "Illegal thread number.");
    shInstrDec->getEmulFunc()(*shInstrDec, *this);
}

//  Returns the PC of a shader thread.
U32 bmoUnifiedShader::GetThreadPC(U32 numThread)
{
    //  Check the range of the thread identifier.
    CG_ASSERT_COND(!(numThread > numThreads), "Illegal thread number.");
    return PCTable[numThread];
}

//  Sets the thread PC.
U32 bmoUnifiedShader::setThreadPC(U32 numThread, U32 pc)
{
    CG_ASSERT_COND(!(numThread > numThreads), "Illegal thread number."); //  Check the range of the thread identifier.
    PCTable[numThread] = pc; //  Set the thread PC.  
    return 0;
}

//  Returns the thread kill flag.
bool bmoUnifiedShader::threadKill(U32 numThread)
{
    return threadKill(numThread, 0);
}

//  Returns the thread's sample kill flag.
bool bmoUnifiedShader::threadKill(U32 numThread, U32 sample)
{
    //  Check the range of the thread identifier.
    CG_ASSERT_COND(!(numThread > numThreads), "Illegal thread number.");
    return kill[numThread][sample];
}

//  Returns the thread's sample z export value.
F32 bmoUnifiedShader::threadZExport(U32 numThread, U32 sample)
{
    //  Check the range of the thread identifier.
    CG_ASSERT_COND(!(numThread > numThreads), "Illegal thread number.");
    return zexport[numThread][sample];
}

//  Generates the next complete texture request for a stamp available in the shader behaviorModel.
TextureAccess *bmoUnifiedShader::nextTextureAccess()
{
    TextureAccess *textAccess = NULL;

    //  Check if there are texture operations waiting to be processed.
    if (numWait > 0)
    {
        //  Create texture access from the entry.
        textAccess = GetTP()->textureOperation(
            waitTexture[firstWait],
            textQueue[waitTexture[firstWait]].texOp,
            textQueue[waitTexture[firstWait]].coordinates,
            textQueue[waitTexture[firstWait]].parameter,
            textQueue[waitTexture[firstWait]].textUnit
            );

        //  Update number of texture accesses waiting to be processed.
        numWait--;

        //  Update pointer to the next entry waiting to be processed.
        firstWait = GPU_MOD(firstWait + 1, TEXT_QUEUE_SIZE);
    }

    return textAccess;
}

//  Generates the next complete texture request for a vertex texture access available in the shader behaviorModel.
TextureAccess *bmoUnifiedShader::nextVertexTextureAccess()
{
    TextureAccess *textAccess = NULL;

    //  Check if there is a pending request.
    if (textQueue[freeTexture[firstFree]].requested > 0)
    {
        //  Fill with dummy data the other elements in the request.
        for(U32 e = 1; e < STAMP_FRAGMENTS; e++)
        {
            //  Copy texture coordinates (index) and parameter.
            textQueue[freeTexture[firstFree]].coordinates[e] = textQueue[freeTexture[firstFree]].coordinates[0];
            textQueue[freeTexture[firstFree]].parameter[e] = textQueue[freeTexture[firstFree]].parameter[0];

            //  Set the shader instruction that produced the access.
            textQueue[freeTexture[firstFree]].shInstrD[e] = textQueue[freeTexture[firstFree]].shInstrD[0];

            //  Update requested fragments.
            textQueue[freeTexture[firstFree]].requested++;
        }
        
        //  Set as a vertex texture access.
        textQueue[freeTexture[firstFree]].vertexTextureAccess = true;
    
        //  Add entry to the wait list.
        waitTexture[lastWait] = freeTexture[firstFree];

        //  Update pointer to the last waiting entry.
        lastWait = GPU_MOD(lastWait + 1, TEXT_QUEUE_SIZE);

        //  Update number of entries waiting to be processed.
        numWait++;

        //  Update pointer to first free entry.
        firstFree = GPU_MOD(firstFree + 1, TEXT_QUEUE_SIZE);

        //  Update number of free entries.
        numFree--;
    }
    
    //  Check if there are texture operations waiting to be processed.
    if (numWait > 0)
    {
        //  Create texture access from the entry.
        textAccess = GetTP()->textureOperation(
            waitTexture[firstWait],
            textQueue[waitTexture[firstWait]].texOp,
            textQueue[waitTexture[firstWait]].coordinates,
            textQueue[waitTexture[firstWait]].parameter,
            textQueue[waitTexture[firstWait]].textUnit
            );

        //  Update number of texture accesses waiting to be processed.
        numWait--;

        //  Update pointer to the next entry waiting to be processed.
        firstWait = GPU_MOD(firstWait + 1, TEXT_QUEUE_SIZE);
    }

    return textAccess;
}

//  Completes a texture access writing the sample in the result register.
void bmoUnifiedShader::writeTextureAccess(U32 id, Vec4FP32 *sample, U32 *threads, bool removeInstruction)
{
    //  Write the result of the texture accesses.
    for(U32 i = 0; i < stampFragments; i++)
    {
        //  For vertex texture accesses only the first element in the texture access is valid/must be written.
        if (!textQueue[id].vertexTextureAccess || (i == 0))
        {
            //  Write texture sample into the texture access result register.
            writeResult(*textQueue[id].shInstrD[i], *this, sample[i].getVector());

            threads[i] = textQueue[id].shInstrD[i]->getNumThread();

            //  Delete the shader decoded instruction that generated the request.
            if (removeInstruction)
                delete textQueue[id].shInstrD[i];
        }
    }

    //  Reset requested fragments counter.
    textQueue[id].requested = 0;

    //  Add entry to the free list.
    freeTexture[lastFree] = id;

    //  Update number of free entries.
    numFree++;

    //  Update pointer to the last the free list.
    lastFree = GPU_MOD(lastFree + 1, TEXT_QUEUE_SIZE);
}

//  Adds a texture operation for a shader processing element to the texture queue.
void bmoUnifiedShader::textureOperation(cgoShaderInstr::cgoShaderInstrEncoding &shInstrDec, TextureOperation texOp,
    Vec4FP32 coord, F32 parameter)
{
    cgoShaderInstr *shInstr = shInstrDec.getShaderInstruction();
    CG_ASSERT_COND( (numFree != 0), "No free entries in the texture queue."); 
    //  Determine if it is the first request for the stamp.
    if (textQueue[freeTexture[firstFree]].requested == 0)
    {
        //  Set the operation type.
        textQueue[freeTexture[firstFree]].texOp = texOp;
        //  Set the texture sampler or attribute for the operation.
        textQueue[freeTexture[firstFree]].textUnit = shInstr->getOp2();
        //  Set as not a vertex texture access.
        textQueue[freeTexture[firstFree]].vertexTextureAccess = false;
    }
    else
    {
        //  Check that the texture operation is the same for the whole 'stamp'.
        CG_ASSERT_COND(!(textQueue[freeTexture[firstFree]].texOp != texOp), "Different operation for elements of the same stamp.");
        //  Check that the texture sampler or attribute for the operation is the same for the whole 'stamp'.
        CG_ASSERT_COND(!(textQueue[freeTexture[firstFree]].textUnit != shInstr->getOp2()), "Accessing different texture units from the same stamp.");    }

    //  Copy texture coordinates (index) and parameter.
    textQueue[freeTexture[firstFree]].coordinates[textQueue[freeTexture[firstFree]].requested] = coord;
    textQueue[freeTexture[firstFree]].parameter[textQueue[freeTexture[firstFree]].requested] = parameter;

    //  Set the shader instruction that produced the access.
    textQueue[freeTexture[firstFree]].shInstrD[textQueue[freeTexture[firstFree]].requested] = &shInstrDec;

    //  Update requested fragments.
    textQueue[freeTexture[firstFree]].requested++;

    //  Check if all the fragments were requested.
    if (textQueue[freeTexture[firstFree]].requested == stampFragments)
    {
        //  Add entry to the wait list.
        waitTexture[lastWait] = freeTexture[firstFree];

        //  Update pointer to the last waiting entry.
        lastWait = GPU_MOD(lastWait + 1, TEXT_QUEUE_SIZE);

        //  Update number of entries waiting to be processed.
        numWait++;

        //  Update pointer to first free entry.
        firstFree = GPU_MOD(firstFree + 1, TEXT_QUEUE_SIZE);

        //  Update number of free entries.
        numFree--;
    }
}

//  Add information for a derivation operation.
void bmoUnifiedShader::derivOperation(cg1gpu::cgoShaderInstr::cgoShaderInstrEncoding &shInstr, cg1gpu::Vec4FP32 input)
{
    //  Compute the base thread/element for the derivation operation.
    U32 baseThread = shInstr.getNumThread();
    baseThread = baseThread - (baseThread & 0x03);
    
    //  Check the number of derivations already received.
    if (currentDerivation.derived == 0)
    {
        //  Initialize the derivation structure with the base thread/element.
        currentDerivation.baseThread = baseThread;
        
        //  Store the information about the derivation.
        currentDerivation.input[currentDerivation.derived] = input.getVector();
        currentDerivation.shInstrD[currentDerivation.derived] = &shInstr;
        currentDerivation.derived++;       
    }
    else if (currentDerivation.derived < 3)
    {
        //  Check the base thread for the current operation.
        CG_ASSERT_COND(!(baseThread != currentDerivation.baseThread), "The base thread for the incoming and on-going derivation operation is different.");        
        //  Store the information about the derivation.
        currentDerivation.input[currentDerivation.derived] = input.getVector();
        currentDerivation.shInstrD[currentDerivation.derived] = &shInstr;
        currentDerivation.derived++;       
    }
    else if (currentDerivation.derived == 3)
    {
        //  Check the base thread for the current operation.
        CG_ASSERT_COND(!(baseThread != currentDerivation.baseThread), "The base thread for the incoming and on-going derivation operation is different.");
        //  Store the information about the derivation.
        currentDerivation.input[currentDerivation.derived] = input.getVector();
        currentDerivation.shInstrD[currentDerivation.derived] = &shInstr;
        
        //  Perform the derivation computations.
        Vec4FP32 derivates[4];
        
        //  Check the derivation direction.
        switch(currentDerivation.shInstrD[0]->getShaderInstruction()->getOpcode())
        {
            case CG1_ISA_OPCODE_DDX:
            
                GPUMath::derivX(currentDerivation.input, derivates);
                break;
                
            case CG1_ISA_OPCODE_DDY:
                
                GPUMath::derivY(currentDerivation.input, derivates);
                break;
                
            default:
                CG_ASSERT("Expected a derivation instruction.");
                break;
        }

        //  Write derivation operation results to the corresponding registers.
        for(U32 e = 0; e < 4; e++)
            writeResult(*currentDerivation.shInstrD[e], *this, derivates[e].getVector());

        //  Clear current derivation operation.
        currentDerivation.derived = 0;
    }
    else
    {
        CG_ASSERT("Unexpected value for the number of derivation operations.");
    }
}


//  Swizzle function.  Reorders and replicates the components of a 4-component float.
inline Vec4FP32 bmoUnifiedShader::swizzle(SwizzleMode mode, Vec4FP32 qf)
{
    Vec4FP32 out;

    //
    //  As SwizzleMode definition is ordered per components
    //  we can get the index for the component in each position
    //  using this method:
    //
    //  mode & 0x03 -> bits 1 - 0 -> Source component to copy to W destination component.
    //  mode & 0x0C -> bits 3 - 2 -> Source component to copy to Z destination component.
    //  mode & 0x30 -> bits 5 - 4 -> Source component to copy to Y destination component.
    //  mode & 0xC0 -> bits 7 - 6 -> Source component to copy to X destination component.
    //

    out[0] = qf[(mode & 0xC0) >> 6];    //  Swizzle X component in the output Vec4FP32.
    out[1] = qf[(mode & 0x30) >> 4];    //  Swizzle Y component in the output Vec4FP32.
    out[2] = qf[(mode & 0x0C) >> 2];    //  Swizzle Z component in the output Vec4FP32.
    out[3] = qf[mode & 0x03];           //  Swizzle W component in the output Vec4FP32.

    return out;     //  Return the swizzled value.
}

//  Swizzle function.  Reorders and replicates the components of a 4-component integer.
inline Vec4Int bmoUnifiedShader::swizzle(SwizzleMode mode, Vec4Int qi)
{
    Vec4Int out;

    //
    //  As SwizzleMode definition is ordered per components
    //  we can get the index for the component in each position
    //  using this method:
    //
    //  mode & 0x03 -> bits 1 - 0 -> Source component to copy to W destination component.
    //  mode & 0x0C -> bits 3 - 2 -> Source component to copy to Z destination component.
    //  mode & 0x30 -> bits 5 - 4 -> Source component to copy to Y destination component.
    //  mode & 0xC0 -> bits 7 - 6 -> Source component to copy to X destination component.
    //

    out[0] = qi[(mode & 0xC0) >> 6];       //  Swizzle X component in the output Vec4Int.
    out[1] = qi[(mode & 0x30) >> 4];       //  Swizzle Y component in the output Vec4Int.
    out[2] = qi[(mode & 0x0C) >> 2];       //  Swizzle Z component in the output Vec4Int.
    out[3] = qi[mode & 0x03];              //  Swizzle W component in the output Vec4Int.

    return out;     //  Return the swizzled value.
}

//  Negate function.  Negates the 4 components in a 4-component float.
inline Vec4FP32 bmoUnifiedShader::negate(Vec4FP32 qf)
{
    Vec4FP32 out;

    //  Negate each component of the 4-component vector.
    out[0] = -qf[0];        //  Negate component X.
    out[1] = -qf[1];        //  Negate component Y.
    out[2] = -qf[2];        //  Negate component Z.
    out[3] = -qf[3];        //  Negate component W.

    return out;
}

//  Negate function.  Negates the 4 components in a 4-component integer.
inline Vec4Int bmoUnifiedShader::negate(Vec4Int qi)
{
    Vec4Int out;

    //  Negate each component of the 4-component vector.
    out[0] = -qi[0];        //  Negate component X.
    out[1] = -qi[1];        //  Negate component Y.
    out[2] = -qi[2];        //  Negate component Z.
    out[3] = -qi[3];        //  Negate component W.

    return out;
}

//  Absolute function.  Returns the absolute value for the 4 components in a 4-component float.
inline Vec4FP32 bmoUnifiedShader::absolute(Vec4FP32 qf)
{
    Vec4FP32 out;

    //  Negate each component of the 4-component vector.
    out[0] = GPUMath::ABS(qf[0]);        //  Negate component X.
    out[1] = GPUMath::ABS(qf[1]);        //  Negate component Y.
    out[2] = GPUMath::ABS(qf[2]);        //  Negate component Z.
    out[3] = GPUMath::ABS(qf[3]);        //  Negate component W.

    return out;
}


//  Absolute function.  Returns the absolute value for the 4 components in a 4-component integer.
inline Vec4Int bmoUnifiedShader::absolute(Vec4Int qi)
{
    Vec4Int out;

    //  Negate each component of the 4-component vector.
    out[0] = GPUMath::ABS(qi[0]);        //  Negate component X.
    out[1] = GPUMath::ABS(qi[1]);        //  Negate component Y.
    out[2] = GPUMath::ABS(qi[2]);        //  Negate component Z.
    out[3] = GPUMath::ABS(qi[3]);        //  Negate component W.

    return out;
}

//  Write result register with mask and predication.
inline void bmoUnifiedShader::writeResReg(cgoShaderInstr &shInstr, F32 *f, F32 *res, bool predicate)
{
    //  Check if the instruction predication allows writing the result.
    if (predicate)
    {
        //  Component X must be written?
        if (shInstr.getResultMaskMode() & 0x08)
            res[0] = f[0];      //  Write component X in result.

        //  Component Y must be written?
        if (shInstr.getResultMaskMode() & 0x04)
            res[1] = f[1];      //  Write component Y in result.

        //  Component Z must be written?
        if (shInstr.getResultMaskMode() & 0x02)
            res[2] = f[2];      //  Write component Z in result.

        //  Component W must be written?
        if (shInstr.getResultMaskMode() & 0x01)
            res[3] = f[3];      //  Write component W in result.

//printf("writing to res = {%f, %f, %f, %f}\n", res[0], res[1], res[2], res[3]);
    }
}

//  Write result register with mask and predication.
inline void bmoUnifiedShader::writeResReg(cgoShaderInstr &shInstr, S32 *f, S32 *res, bool predicate)
{
    //  Check if the instruction predication allows writing the result.
    if (predicate)
    {
        //  Component X must be written?
        if (shInstr.getResultMaskMode() & 0x08)
            res[0] = f[0];      //  Write component X in result.

        //  Component Y must be written?
        if (shInstr.getResultMaskMode() & 0x04)
            res[1] = f[1];      //  Write component Y in result.

        //  Component Z must be written?
        if (shInstr.getResultMaskMode() & 0x02)
            res[2] = f[2];      //  Write component Z in result.

        //  Component W must be written?
        if (shInstr.getResultMaskMode() & 0x01)
            res[3] = f[3];      //  Write component W in result.

//printf("writing to res = {%f, %f, %f, %f}\n", res[0], res[1], res[2], res[3]);
    }
}

//  Write result register with mask and predication.  Fixed point accumulator version.
inline void bmoUnifiedShader::writeResReg(cgoShaderInstr &shInstr, FixedPoint *f, FixedPoint *res, bool predicate)
{
    //  Check if the instruction predication allows writing the result.
    if (predicate)
    {
        //  Check the write mask each component.  Update the result register.

        //  Component X must be written?
        if (shInstr.getResultMaskMode() & 0x08)
            res[0] = f[0];      //  Write component X in result.

        //  Component Y must be written?
        if (shInstr.getResultMaskMode() & 0x04)
            res[1] = f[1];      //  Write component Y in result.

        //  Component Z must be written?
        if (shInstr.getResultMaskMode() & 0x02)
            res[2] = f[2];      //  Write component Z in result.

        //  Component W must be written?
        if (shInstr.getResultMaskMode() & 0x01)
            res[3] = f[3];      //  Write component W in result.
    }
}


/*
 *  Read the Shader Instruction First Input Operands.
 *
 *  THIS IS A MACRO.  THIS IS A MACRO. THIS IS A MACRO.
 *
 *      BANK    is a getBankOpN() function name
 *      OPERAND is a getShEmulOpN() function name
 *      SHIMDEC is a cgoShaderInstr::cgoShaderInstrEncoding reference
 *      SHINSTR is a cgoShaderInstr pointer
 *      bmUnifiedShader  is a bmoUnifiedShader reference
 *      OPREGV  is a variable to store the read operand
 *      TYPE    is the type (Vec4FP32, Vec4Int, Int, Float, Bool)
 *
 */

#define READOPERAND(OPERAND, SHINDEC, SHINSTR, bmUnifiedShader, OPREGV, TYPE)                    \
{                                                                                       \
    /*  If it a constant and relative mode addressing is enabled the */                 \
    /*    operand address must be recalculated.                      */                 \
    if ((SHINSTR->getBankOp##OPERAND() == PARAM) && (SHINSTR->getRelativeModeFlag()))   \
    {                                                                                   \
        /*  WARNING:  CHECK IF THIS IS THE CORRECT IMPLEMENTATION OF   */               \
        /*    RELATIVE MODE ADDRESSING TO CONSTANTS.                   */               \
        /*                                                             */               \
        /*    c[A0.x + offset]                                         */               \
        /*                                                             */               \
        /*    AddressBank[AddressRegister][Component] + relativeOffset.*/               \
        /*                                                             */               \
        /*    offset Should be Signed -256 to 255 (9 bits)             */               \
                                                                                        \
        /*  WARNING:  DECODING SHOULD ENSURE THAT ONLY A CONSTANT BANK */               \
        /*  REGISTER IS ACCESSED USING RELATIVE MODE.  CONSTANTS ARE   */               \
        /*  ALWAYS QUADFLOAT.                                          */               \
                                                                                        \
        OPREGV = (TYPE *) &bmUnifiedShader.constantBank[                                         \
         ((*(Vec4Int *)SHINDEC.getShEmulOp##OPERAND())[SHINSTR->getRelMAddrRegComp()] + \
          SHINSTR->getRelMOffset() + SHINSTR->getOp##OPERAND()) *                       \
          UNIFIED_CONST_REG_SIZE];                                                      \
    }                                                                                   \
    else                                                                                \
    {                                                                                   \
        /*  If not, just get the precalculated register address. */                     \
        OPREGV = (TYPE *) SHINDEC.getShEmulOp##OPERAND();                               \
    }                                                                                   \
}


//  Reads and returns the first quadfloat operand of a shader instruction.
inline void bmoUnifiedShader::readOperand1(cgoShaderInstr::cgoShaderInstrEncoding &shInstrDec, bmoUnifiedShader &bmUnifiedShader, Vec4FP32 &op)
{
    cgoShaderInstr *shInstr = shInstrDec.getShaderInstruction();

    //  Get a pointer to the input register.
    READOPERAND(1, shInstrDec, shInstr, bmUnifiedShader, op, F32)
}

//  Reads and returns the second quadfloat operand of a shader instruction.
inline void bmoUnifiedShader::readOperand2(cgoShaderInstr::cgoShaderInstrEncoding &shInstrDec, bmoUnifiedShader &bmUnifiedShader, Vec4FP32 &op)
{
    cgoShaderInstr *shInstr = shInstrDec.getShaderInstruction();

    //  Get a pointer to the input register.
    READOPERAND(2, shInstrDec, shInstr, bmUnifiedShader, op, F32)
}

//  Reads and returns the third quadfloat operand of a shader instruction.
inline void bmoUnifiedShader::readOperand3(cgoShaderInstr::cgoShaderInstrEncoding &shInstrDec, bmoUnifiedShader &bmUnifiedShader,
    Vec4FP32 &op)
{
    cgoShaderInstr *shInstr = shInstrDec.getShaderInstruction();

    //  Get a pointer to the input register.
    READOPERAND(3, shInstrDec, shInstr, bmUnifiedShader, op, F32)
}

//  Reads and returns the first quadint operand of a shader instruction.
inline void bmoUnifiedShader::readOperand1(cgoShaderInstr::cgoShaderInstrEncoding &shInstrDec, bmoUnifiedShader &bmUnifiedShader, Vec4Int &op)
{
    cgoShaderInstr *shInstr = shInstrDec.getShaderInstruction();

    //  Get a pointer to the input register.
    READOPERAND(1, shInstrDec, shInstr, bmUnifiedShader, op, S32)
}

//  Reads and returns the second quadint operand of a shader instruction.
inline void bmoUnifiedShader::readOperand2(cgoShaderInstr::cgoShaderInstrEncoding &shInstrDec, bmoUnifiedShader &bmUnifiedShader, Vec4Int &op)
{
    cgoShaderInstr *shInstr = shInstrDec.getShaderInstruction();

    //  Get a pointer to the input register.
    READOPERAND(2, shInstrDec, shInstr, bmUnifiedShader, op, S32)
}

//  Reads and returns the first float scalar operand.
inline void bmoUnifiedShader::readScalarOp1(cgoShaderInstr::cgoShaderInstrEncoding &shInstrDec, bmoUnifiedShader &bmUnifiedShader, F32 &op)
{
    cgoShaderInstr *shInstr = shInstrDec.getShaderInstruction();
    Vec4FP32 reg;

    //  Get pointer to first Vec4FP32 operand register.
    READOPERAND(1, shInstrDec, shInstr, bmUnifiedShader, reg, F32)

    //  Get component from the operand Vec4FP32 register.
    op = reg[shInstr->getOp1SwizzleMode() & 0x03];

    //  Check if absolute value flag is enable.
    if(shInstr->getOp1AbsoluteFlag())
        op = GPUMath::ABS(op);      //  Get absolute value of the first operand.

    //  Check if negate flag is enabled.
    if(shInstr->getOp1NegateFlag())
        op = -op;                   //  Negate.

//printf("reading scalar op1 = %f\n", op);
}

//  Reads and returns the second float scalar operand.
inline void bmoUnifiedShader::readScalarOp2(cgoShaderInstr::cgoShaderInstrEncoding &shInstrDec, bmoUnifiedShader &bmUnifiedShader, F32 &op)
{
    cgoShaderInstr *shInstr = shInstrDec.getShaderInstruction();
    Vec4FP32 reg;

    //  Get pointer to second Vec4FP32 operand register.
    READOPERAND(2, shInstrDec, shInstr, bmUnifiedShader, reg, F32)

    //  Get component from the operand Vec4FP32 register.
    op = reg[shInstr->getOp2SwizzleMode() & 0x03];

    //  Check if absolute value flag is enable.
    if(shInstr->getOp2AbsoluteFlag())
        op = GPUMath::ABS(op);      //  Get absolute value of the first operand.

    //  Check if negate flag is enabled.
    if(shInstr->getOp2NegateFlag())
        op = -op;                   //  Negate.

//printf("reading scalar op2 = %f\n", op);
}

//  Reads and returns the first float scalar operand.
inline void bmoUnifiedShader::readScalarOp1(cgoShaderInstr::cgoShaderInstrEncoding &shInstrDec, bmoUnifiedShader &bmUnifiedShader, S32 &op)
{
    cgoShaderInstr *shInstr = shInstrDec.getShaderInstruction();
    Vec4Int reg;

    //  Get pointer to first Vec4FP32 operand register.
    READOPERAND(1, shInstrDec, shInstr, bmUnifiedShader, reg, S32)

    //  Get component from the operand Vec4FP32 register.
    op = reg[shInstr->getOp1SwizzleMode() & 0x03];

    //  Check if absolute value flag is enable.
    if(shInstr->getOp1AbsoluteFlag())
        op = GPUMath::ABS(op);      //  Get absolute value of the first operand.

    //  Check if negate flag is enabled.
    if(shInstr->getOp1NegateFlag())
        op = -op;                   //  Negate.

//printf("reading scalar op1 = %f\n", op);
}

//  Reads and returns the second float scalar operand.
inline void bmoUnifiedShader::readScalarOp2(cgoShaderInstr::cgoShaderInstrEncoding &shInstrDec, bmoUnifiedShader &bmUnifiedShader, S32 &op)
{
    cgoShaderInstr *shInstr = shInstrDec.getShaderInstruction();
    Vec4Int reg;

    //  Get pointer to second Vec4FP32 operand register.
    READOPERAND(2, shInstrDec, shInstr, bmUnifiedShader, reg, S32)

    //  Get component from the operand Vec4FP32 register.
    op = reg[shInstr->getOp2SwizzleMode() & 0x03];

    //  Check if absolute value flag is enable.
    if(shInstr->getOp2AbsoluteFlag())
        op = GPUMath::ABS(op);      //  Get absolute value of the first operand.

    //  Check if negate flag is enabled.
    if(shInstr->getOp2NegateFlag())
        op = -op;                   //  Negate.

//printf("reading scalar op2 = %f\n", op);
}

//  Reads and returns the first float scalar operand.
inline void bmoUnifiedShader::readScalarOp1(cgoShaderInstr::cgoShaderInstrEncoding &shInstrDec, bmoUnifiedShader &bmUnifiedShader, bool &op)
{
    cgoShaderInstr *shInstr = shInstrDec.getShaderInstruction();

    //  Check if the operand is an implicit boolean operand (aliased to absolute flag).
    if (shInstr->getOp1AbsoluteFlag())
    {
        // Get the implicit boolean operand (aliased to negate flag).
        op = shInstr->getOp1NegateFlag();
    }
    else
    {    
        //  Check if the operand comes from a constant register.
        if (shInstr->getBankOp1() == PARAM)
        {
            Vec4Int reg;

            //  Get pointer to first Vec4FP32 operand register.
            READOPERAND(1, shInstrDec, shInstr, bmUnifiedShader, reg, S32)

            //  Get component from the operand Vec4Int register.
            op = (reg[shInstr->getOp1SwizzleMode() & 0x03] != 0);
        }
        else
        {
            //  Read the operand value from a predicate register.
            op = *((bool *) shInstrDec.getShEmulOp1());
        }
        
        //  Check if the operands is inverted (aliased to negate flag)
        if(shInstr->getOp1NegateFlag())
            op = !op;
    }
    
    
//printf("reading scalar op1 = %s\n", op ? "true" : "false");
}

//  Reads and returns the first float scalar operand.
inline void bmoUnifiedShader::readScalarOp2(cgoShaderInstr::cgoShaderInstrEncoding &shInstrDec, bmoUnifiedShader &bmUnifiedShader, bool &op)
{
    cgoShaderInstr *shInstr = shInstrDec.getShaderInstruction();

    //  Check if the operand is an implicit boolean operand (aliased to absolute flag).
    if (shInstr->getOp2AbsoluteFlag())
    {
        // Get the implicit boolean operand (aliased to negate flag).
        op = shInstr->getOp2NegateFlag();
    }
    else
    {    
        //  Check if the operand comes from a constant register.
        if (shInstr->getBankOp2() == PARAM)
        {
            Vec4Int reg;
            
            //  Get pointer to first Vec4FP32 operand register.
            READOPERAND(2, shInstrDec, shInstr, bmUnifiedShader, reg, S32)

            //  Get component from the operand Vec4Int register.
            op = (reg[shInstr->getOp2SwizzleMode() & 0x03] != 0);
        }
        else
        {
            //  Read the operand value from a predicate register.
            op = *((bool *) shInstrDec.getShEmulOp2());
        }

        //  Check if the operands is inverted (aliased to negate flag)
        if(shInstr->getOp2NegateFlag())
            op = !op;
    }
    
//printf("reading scalar op1 = %s\n", op ? "true" : "false");
}

//  Reads operand from a 1-operand shader instruction.
inline void bmoUnifiedShader::read1Operands(cgoShaderInstr::cgoShaderInstrEncoding &shInstrDec, bmoUnifiedShader &bmUnifiedShader, Vec4FP32 &op)
{
    cgoShaderInstr *shInstr = shInstrDec.getShaderInstruction();

    //  Get first operand.
    bmUnifiedShader.readOperand1(shInstrDec, bmUnifiedShader, op);

    //  Swizzle first operand.
    op = bmUnifiedShader.swizzle(shInstr->getOp1SwizzleMode(), op);

    //  If the first operand absolute flag enabled.
    if(shInstr->getOp1AbsoluteFlag())
        op = bmUnifiedShader.absolute(op);     //  Get absolute value of the first operand.

    //  Check if the first operand negate flag is enabled.
    if(shInstr->getOp1NegateFlag())
        op = bmUnifiedShader.negate(op);

//printf("reading op1 = {%f, %f, %f, %f}\n", op[0], op[1], op[2], op[3]);
}

//  Reads operands from a 2-operands shader instruction.
inline void bmoUnifiedShader::read2Operands(cgoShaderInstr::cgoShaderInstrEncoding &shInstrDec, bmoUnifiedShader &bmUnifiedShader,
                                          Vec4FP32 &op1, Vec4FP32 &op2)
{
    cgoShaderInstr *shInstr = shInstrDec.getShaderInstruction();

    //  Get first operand.
    bmUnifiedShader.readOperand1(shInstrDec, bmUnifiedShader, op1);

    //  Swizzle first operand.
    op1 = bmUnifiedShader.swizzle(shInstr->getOp1SwizzleMode(), op1);

    //  If the first operand absolute flag enabled.
    if(shInstr->getOp1AbsoluteFlag())
        op1 = bmUnifiedShader.absolute(op1);     //  Get absolute value of the first operand.

    //  Check if the first operand negate flag is enabled.
    if(shInstr->getOp1NegateFlag())
        op1 = bmUnifiedShader.negate(op1);       //  Negate.

    //  Get second operand.
    bmUnifiedShader.readOperand2(shInstrDec, bmUnifiedShader, op2);

    //  Swizzle second operand.
    op2 = bmUnifiedShader.swizzle(shInstr->getOp2SwizzleMode(), op2);

    //  If the second operand absolute flag enabled.
    if(shInstr->getOp2AbsoluteFlag())
        op2 = bmUnifiedShader.absolute(op2);     //  Get absolute value of the second operand.

    //  Check if the second operand negate flag is enabled.
    if(shInstr->getOp2NegateFlag())
        op2 = bmUnifiedShader.negate(op2);       //  Negate.

//printf("reading op1 = {%f, %f, %f, %f}\n", op1[0], op1[1], op1[2], op1[3]);
//printf("reading op2 = {%f, %f, %f, %f}\n", op2[0], op2[1], op2[2], op2[3]);
}

//  Reads operands from a 3-operands shader instruction.
inline void bmoUnifiedShader::read3Operands(cgoShaderInstr::cgoShaderInstrEncoding &shInstrDec, bmoUnifiedShader &bmUnifiedShader,
                                          Vec4FP32 &op1, Vec4FP32 &op2, Vec4FP32 &op3)
{
    cgoShaderInstr *shInstr = shInstrDec.getShaderInstruction();

    //  Get first operand.
    bmUnifiedShader.readOperand1(shInstrDec, bmUnifiedShader, op1);

    //  Swizzle first operand.
    op1 = bmUnifiedShader.swizzle(shInstr->getOp1SwizzleMode(), op1);

    //  If the first operand absolute flag enabled.
    if(shInstr->getOp1AbsoluteFlag())
        op1 = bmUnifiedShader.absolute(op1);     //  Get absolute value of the first operand.

    //  Check if the first operand negate flag is enabled.
    if(shInstr->getOp1NegateFlag())
        op1 = bmUnifiedShader.negate(op1);       //  Negate.

    //  Get second operand.
    bmUnifiedShader.readOperand2(shInstrDec, bmUnifiedShader, op2);

    //  Swizzle second operand.
    op2 = bmUnifiedShader.swizzle(shInstr->getOp2SwizzleMode(), op2);

    //  If the second operand absolute flag enabled.
    if(shInstr->getOp2AbsoluteFlag())
        op2 = bmUnifiedShader.absolute(op2);     //  Get absolute value of the second operand.

    //  Check if the second operand negate flag is enabled.
    if(shInstr->getOp2NegateFlag())
        op2 = bmUnifiedShader.negate(op2);       //  Negate.

    //  Get third operand.
    bmUnifiedShader.readOperand3(shInstrDec, bmUnifiedShader, op3);

    //  Swizzle third operand.
    op3 = bmUnifiedShader.swizzle(shInstr->getOp3SwizzleMode(), op3);

    //  If the third operand absolute flag enabled.
    if(shInstr->getOp3AbsoluteFlag())
        op3 = bmUnifiedShader.absolute(op3);     //  Get absolute value of the third operand.

    //  Check if the third operand negate flag is enabled.
    if(shInstr->getOp3NegateFlag())
        op3 = bmUnifiedShader.negate(op3);       //  Negate.

//printf("reading op1 = {%f, %f, %f, %f}\n", op1[0], op1[1], op1[2], op1[3]);
//printf("reading op2 = {%f, %f, %f, %f}\n", op2[0], op2[1], op2[2], op2[3]);
//printf("reading op3 = {%f, %f, %f, %f}\n", op3[0], op3[1], op3[2], op3[3]);
}

//  Reads operands from a 2-operands shader instruction.
inline void bmoUnifiedShader::read2Operands(cgoShaderInstr::cgoShaderInstrEncoding &shInstrDec, bmoUnifiedShader &bmUnifiedShader,
                                          Vec4Int &op1, Vec4Int &op2)
{
    cgoShaderInstr *shInstr = shInstrDec.getShaderInstruction();

    //  Get first operand.
    bmUnifiedShader.readOperand1(shInstrDec, bmUnifiedShader, op1);

    //  Swizzle first operand.
    op1 = bmUnifiedShader.swizzle(shInstr->getOp1SwizzleMode(), op1);

    //  If the first operand absolute flag enabled.
    if(shInstr->getOp1AbsoluteFlag())
        op1 = bmUnifiedShader.absolute(op1);     //  Get absolute value of the first operand.

    //  Check if the first operand negate flag is enabled.
    if(shInstr->getOp1NegateFlag())
        op1 = bmUnifiedShader.negate(op1);       //  Negate.

    //  Get second operand.
    bmUnifiedShader.readOperand2(shInstrDec, bmUnifiedShader, op2);

    //  Swizzle second operand.
    op2 = bmUnifiedShader.swizzle(shInstr->getOp2SwizzleMode(), op2);

    //  If the second operand absolute flag enabled.
    if(shInstr->getOp2AbsoluteFlag())
        op2 = bmUnifiedShader.absolute(op2);     //  Get absolute value of the second operand.

    //  Check if the second operand negate flag is enabled.
    if(shInstr->getOp2NegateFlag())
        op2 = bmUnifiedShader.negate(op2);       //  Negate.

//printf("reading op1 = {%f, %f, %f, %f}\n", op1[0], op1[1], op1[2], op1[3]);
//printf("reading op2 = {%f, %f, %f, %f}\n", op2[0], op2[1], op2[2], op2[3]);
}


//  Writes the quadfloat result of a shader instruction.
inline void bmoUnifiedShader::writeResult(cgoShaderInstr::cgoShaderInstrEncoding &shInstrDec, bmoUnifiedShader &bmUnifiedShader, F32 *result)
{
    F32 *res;
    F32 aux[4];
    cgoShaderInstr *shInstr = shInstrDec.getShaderInstruction();
   
    //  Get address for the result register.
    res = (F32 *) shInstrDec.getShEmulResult();

    //  Get predication for the instruction.
    bool *predicateReg = (bool *) shInstrDec.getShEmulPredicate();
    bool predicateValue = (predicateReg == NULL) ? true : (( (*predicateReg) && !shInstr->getNegatePredicateFlag()) ||
                                                           (!(*predicateReg) &&  shInstr->getNegatePredicateFlag()));
    
    //  Check saturated result flag.
    if (shInstr->getSaturatedRes())
    {
        //  Clamp the result vector components to [0, 1].
        GPUMath::SAT(result, aux);


        //  Write Mask.  Only write in the result register the selected components.
        bmUnifiedShader.writeResReg(*shInstr, aux, res, predicateValue);
    }
    else
    {
        //  Write Mask.  Only write in the result register the selected components.
        bmUnifiedShader.writeResReg(*shInstr, result, res, predicateValue);
    }
}

//  Writes the quadint result of a shader instruction.
inline void bmoUnifiedShader::writeResult(cgoShaderInstr::cgoShaderInstrEncoding &shInstrDec, bmoUnifiedShader &bmUnifiedShader, S32 *result)
{
    S32 *res;
    S32 aux[4];
    cgoShaderInstr *shInstr = shInstrDec.getShaderInstruction();
   
    //  Get address for the result register.
    res = (S32 *) shInstrDec.getShEmulResult();

    //  Get predication for the instruction.
    bool *predicateReg = (bool *) shInstrDec.getShEmulPredicate();
    bool predicateValue = (predicateReg == NULL) ? true : (( (*predicateReg) && !shInstr->getNegatePredicateFlag()) ||
                                                           (!(*predicateReg) &&  shInstr->getNegatePredicateFlag()));
    
    //  Write Mask.  Only write in the result register the selected components.
    bmUnifiedShader.writeResReg(*shInstr, result, res, predicateValue);
}


//  Writes the fixed point quad result to the special fixed point accumulator.
inline void bmoUnifiedShader::writeResult(cgoShaderInstr::cgoShaderInstrEncoding &shInstrDec, bmoUnifiedShader &bmUnifiedShader,
    FixedPoint *result)
{
    FixedPoint *res;         //  GPU fixed point quad accumulator.

    //  Get the shader instruction.
    cgoShaderInstr *shInstr = shInstrDec.getShaderInstruction();

    //  Get the fixed point quad accumulator associated with the thread.
    res = accumFXPBank[shInstrDec.getNumThread()];

    //  NOTE:  Saturation not supported for fixed point quad accumulator.

    //  Get predication for the instruction.
    bool *predicateReg = (bool *) shInstrDec.getShEmulPredicate();
    bool predicateValue = (predicateReg == NULL) ? true : (( (*predicateReg) && !shInstr->getNegatePredicateFlag()) ||
                                                           (!(*predicateReg) &&  shInstr->getNegatePredicateFlag()));

    //  Write Mask.  Only write in the result register the selected components.
    bmUnifiedShader.writeResReg(*shInstr, result, res, predicateValue);
}

inline void bmoUnifiedShader::writeResult(cgoShaderInstr::cgoShaderInstrEncoding &shInstrDec, bmoUnifiedShader &bmUnifiedShader, bool result)
{
    bool *res;
    cgoShaderInstr *shInstr = shInstrDec.getShaderInstruction();
   
    //  Get address for the result predicate register.
    res = (bool *) shInstrDec.getShEmulResult();

    //  Get predication for the instruction.
    bool *predicateReg = (bool *) shInstrDec.getShEmulPredicate();
    bool predicateValue = (predicateReg == NULL) ? true : (( (*predicateReg) && !shInstr->getNegatePredicateFlag()) ||
                                                           (!(*predicateReg) &&  shInstr->getNegatePredicateFlag()));

    //  Check if predication allows for the instruction to write a result.
    if (predicateValue)
    {
        //  Check if the result has to be negated (aliased to sature flag).
        if (shInstr->getSaturatedRes())
            result = !result;
    
        //  Write the result into the predicate result register.
        *res = result;
    }    
}

bool bmoUnifiedShader::checkJump(cg1gpu::cgoShaderInstr::cgoShaderInstrEncoding *shInstrDec, U32 vectorLength, U32 &destPC)
{
    //  Get the first thread corresponding to the vector.
    U32 startThread = shInstrDec->getNumThread() - GPU_MOD(shInstrDec->getNumThread(), vectorLength);

    bool jump;
    
    //  Check if the operand is an implicit boolean operand (aliased to absolute flag).
    if (shInstrDec->getShaderInstruction()->getOp1AbsoluteFlag())
    {
        // Get the implicit boolean operand (aliased to negate flag).
        jump = shInstrDec->getShaderInstruction()->getOp1NegateFlag();
    }
    else
    {    
        //  Get the register identifer for the first operand.
        U32 op1 = shInstrDec->getShaderInstruction()->getOp1();
        
        //  Get if the first operand is inverted (aliased to negate flag).
        bool negated = shInstrDec->getShaderInstruction()->getOp1NegateFlag();
        
        //  Check if the operand comes from a constant register.
        if (shInstrDec->getShaderInstruction()->getBankOp1() == PARAM)
        {
            //  Get the component of the constant register that is used as operand.
            U32 component = shInstrDec->getShaderInstruction()->getOp1SwizzleMode() & 0x03;
            
            //  Initialize jump condition for the whole vector.
            jump = true;
            
            //  Check all threads/elements in the vector.
            for(U32 t = 0; t < vectorLength; t++)
            {
                //  Get the condition for the current thread.
                bool jumpThread = ((U32 *) &constantBank)[op1 * (UNIFIED_CONST_REG_SIZE/4) + component];

                //  Invert if required.
                if (negated)
                    jumpThread = !jumpThread;
                                    
                //  Update the jump condition for the whole vector.
                jump = jump && jumpThread;
            }
            
        }
        else
        {
            //  Initialize jump condition for the whole vector.
            jump = true;
            
            //  Check all threads/elements in the vector.
            for(U32 t = 0; t < vectorLength; t++)
            {
                //  Get the condition for the current thread.
                bool jumpThread = predicateBank[startThread + t][op1];

                //  Invert if required.
                if (negated)
                    jumpThread = !jumpThread;
                                    
                //  Update the jump condition for the whole vector.
                jump = jump && jumpThread;
            }
        }
    }
    
    //  If the jump condition is true update the PCs and return the destination PC.
    if (jump)
    {
        //  Compute the target PC.
        destPC = PCTable[startThread] + shInstrDec->getShaderInstruction()->getJumpOffset();
    }
    else
    {
        //  The destination PC is the next instruction.
        destPC = PCTable[startThread] + 1;
    }
    
    //  Update the PCs for all the threads/elements in the vector.
    for(U32 t = 0; t < vectorLength; t++)
        PCTable[startThread + t] = destPC;
    
    return jump;
}


//
//  Shader instruction implementation.
//

void bmoUnifiedShader::shNOP(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shADD(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    Vec4FP32 op1, op2;
    F32 result[4];

    //  Read instruction operands. 
    bmUnifiedShader.read2Operands(shInstr, bmUnifiedShader, op1, op2);

    //  Perform instruction operation.  
    GPUMath::ADD(op1.getVector(), op2.getVector(), result);

    //  Write instruction result.  
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);
//PRINT_OPERATION_RES_2
    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shARL(cgoShaderInstr::cgoShaderInstrEncoding &shInstrDec, bmoUnifiedShader &bmUnifiedShader)
{
    Vec4FP32 op;
    Vec4Int *res;
    S32 *result;

    cgoShaderInstr *shInstr = shInstrDec.getShaderInstruction();
    
    //  Get predication for the instruction.
    bool *predicateReg = (bool *) shInstrDec.getShEmulPredicate();
    bool predicateValue = (predicateReg == NULL) ? true : (( (*predicateReg) && !shInstr->getNegatePredicateFlag()) ||
                                                           (!(*predicateReg) &&  shInstr->getNegatePredicateFlag()));

    //  Check if the predicate value allows the instruction to write a result.
    if (predicateValue)
    {
        //  Read instruction operands.
        bmUnifiedShader.read1Operands(shInstrDec, bmUnifiedShader, op);

        //  Get the result address register.
        res = (Vec4Int *) shInstrDec.getShEmulResult();

        //  Get pointer to the address register values.
        result = res->getVector();

        //  Perform instruction operation and write the result.
        GPUMath::ARL(result, op.getVector());
    }
    
    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstrDec.getNumThread()]++;

}

void bmoUnifiedShader::shCOS(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    F32 op;
    F32 result[4];

    //  Read instruction operands. 
    bmUnifiedShader.readScalarOp1(shInstr, bmUnifiedShader, op);

    //  Perform instruction operation.  
    GPUMath::COS(op, result);

    //  Write instruction result.  
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);

    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shSIN(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    F32 op;
    F32 result[4];

    //  Read instruction operands. 
    bmUnifiedShader.readScalarOp1(shInstr, bmUnifiedShader, op);

    //  Perform instruction operation.  
    GPUMath::SIN(op, result);

    //  Write instruction result.  
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);

    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shDP3(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    Vec4FP32 op1, op2;
    F32 result[4];

    //  Read instruction operands. 
    bmUnifiedShader.read2Operands(shInstr, bmUnifiedShader, op1, op2);

    //  Perform instruction operation.  
    GPUMath::DP3(op1.getVector(), op2.getVector(), result);

    //  Write instruction result.  
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);

    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shDP4(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    Vec4FP32 op1, op2;
    F32 result[4];

    //  Read instruction operands. 
    bmUnifiedShader.read2Operands(shInstr, bmUnifiedShader, op1, op2);

    //  Perform instruction operation.  
    GPUMath::DP4(op1.getVector(), op2.getVector(), result);

    //  Write instruction result.  
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);

    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;

}

void bmoUnifiedShader::shDPH(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    Vec4FP32 op1, op2;
    F32 result[4];

    //  Read instruction operands. 
    bmUnifiedShader.read2Operands(shInstr, bmUnifiedShader, op1, op2);

    //  Perform instruction operation.  
    GPUMath::DPH(op1.getVector(), op2.getVector(), result);

    //  Write instruction result.  
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);

    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shDST(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    Vec4FP32 op1, op2;
    F32 result[4];

    //  Read instruction operands. 
    bmUnifiedShader.read2Operands(shInstr, bmUnifiedShader, op1, op2);

    //  Perform instruction operation.  
    GPUMath::DST(op1.getVector(), op2.getVector(), result);

    //  Write instruction result.  
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);

    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shEX2(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    F32 op;
    F32 result[4];

    //  Read instruction operands. 
    bmUnifiedShader.readScalarOp1(shInstr, bmUnifiedShader, op);

    //  Perform instruction operation.  
    GPUMath::EX2(op, result);

    //  Write instruction result.  
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);

    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shEXP(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    F32 op;
    F32 result[4];

    //  Read instruction operands. 
    bmUnifiedShader.readScalarOp1(shInstr, bmUnifiedShader, op);

    //  Perform instruction operation.  
    GPUMath::EXP(op, result);

    //  Write instruction result.  
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);

    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shFLR(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    CG_ASSERT("Instruction not implemented.");
}

void bmoUnifiedShader::shFRC(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    Vec4FP32 op1;
    F32 result[4];

    //  Read instruction operands. 
    bmUnifiedShader.read1Operands(shInstr, bmUnifiedShader, op1);

    //  Perform instruction operation.  
    GPUMath::FRC(op1.getVector(), result);

    //  Write instruction result.  
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);

    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shLG2(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    F32 op;
    F32 result[4];

    //  Read instruction operands. 
    bmUnifiedShader.readScalarOp1(shInstr, bmUnifiedShader, op);

    //  Perform instruction operation.  
    GPUMath::LG2(op, result);

    //  Write instruction result.  
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);

    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
    //CG_ASSERT("Instruction not implemented.");
}

void bmoUnifiedShader::shLIT(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    Vec4FP32 op1;
    F32 result[4];

    //  Read instruction operands. 
    bmUnifiedShader.read1Operands(shInstr, bmUnifiedShader, op1);

    //  Perform instruction operation.  
    GPUMath::LIT(op1.getVector(), result);

    //  Write instruction result.  
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);

    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shLOG(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    F32 op;
    F32 result[4];

    //  Read instruction operands. 
    bmUnifiedShader.readScalarOp1(shInstr, bmUnifiedShader, op);

    //  Perform instruction operation.  
    GPUMath::LOG(op, result);

    //  Write instruction result.  
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);

    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shMAD(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    Vec4FP32 op1, op2, op3;
    F32 result[4];

    //  Read instruction operands. 
    bmUnifiedShader.read3Operands(shInstr, bmUnifiedShader, op1, op2, op3);

    //  Perform instruction operation.  
    GPUMath::MAD(op1.getVector(), op2.getVector(),op3.getVector(), result);

    //  Write instruction result.  
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);
//PRINT_OPERATION_RES_3
    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shMAX(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    Vec4FP32 op1, op2;
    F32 result[4];

    //  Read instruction operands. 
    bmUnifiedShader.read2Operands(shInstr, bmUnifiedShader, op1, op2);

    //  Perform instruction operation.  
    GPUMath::MAX(op1.getVector(), op2.getVector(), result);

    //  Write instruction result.  
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);

    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shMIN(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    Vec4FP32 op1, op2;
    F32 result[4];

    //  Read instruction operands. 
    bmUnifiedShader.read2Operands(shInstr, bmUnifiedShader, op1, op2);

    //  Perform instruction operation.  
    GPUMath::MIN(op1.getVector(), op2.getVector(), result);

    //  Write instruction result.  
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);

    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shMOV(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    Vec4FP32 op1;
    F32 result[4];

    //  Read instruction operands. 
    bmUnifiedShader.read1Operands(shInstr, bmUnifiedShader, op1);

    //  Perform instruction operation.  
    GPUMath::MOV(op1.getVector(), result);

    //  Write instruction result.  
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);

    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shMUL(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    Vec4FP32 op1, op2;
    F32 result[4];

    //  Read instruction operands. 
    bmUnifiedShader.read2Operands(shInstr, bmUnifiedShader, op1, op2);

    //  Perform instruction operation.  
    GPUMath::MUL(op1.getVector(), op2.getVector(), result);

    //  Write instruction result.  
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);
//PRINT_OPERATION_RES_2
    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shRCP(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    F32 op;
    F32 result[4];

    //  Read instruction operands. 
    bmUnifiedShader.readScalarOp1(shInstr, bmUnifiedShader, op);

    //  Perform instruction operation.  
    GPUMath::RCP(op, result);

    //  Write instruction result.  
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);

//PRINT_OPERATION_RES_1

    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shRSQ(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    F32 op;
    F32 result[4];

    //  Read instruction operands. 
    bmUnifiedShader.readScalarOp1(shInstr, bmUnifiedShader, op);

    //  Perform instruction operation.  
    GPUMath::RSQ(op, result);

    //  Write instruction result.  
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);

    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shSGE(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    Vec4FP32 op1, op2;
    F32 result[4];

    //  Read instruction operands. 
    bmUnifiedShader.read2Operands(shInstr, bmUnifiedShader, op1, op2);

    //  Perform instruction operation.  
    GPUMath::SGE(op1.getVector(), op2.getVector(), result);

    //  Write instruction result.  
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);

    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shSLT(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    Vec4FP32 op1, op2;
    F32 result[4];

    //  Read instruction operands. 
    bmUnifiedShader.read2Operands(shInstr, bmUnifiedShader, op1, op2);

    //  Perform instruction operation.  
    GPUMath::SLT(op1.getVector(), op2.getVector(), result);

    //  Write instruction result.  
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);

    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}


void bmoUnifiedShader::shTEX(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    Vec4FP32 op1;

    //  Read instruction operands. 
    bmUnifiedShader.read1Operands(shInstr, bmUnifiedShader, op1);

    //  Add the texture access to the queue.  
    bmUnifiedShader.textureOperation(shInstr, TEXTURE_READ, op1, 0.0f);

    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shTXB(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    Vec4FP32 op1;
    bmUnifiedShader.read1Operands(shInstr, bmUnifiedShader, op1); //  Read instruction operands. 
    bmUnifiedShader.textureOperation(shInstr, TEXTURE_READ, op1, op1[3]); //  Add the texture access to the queue.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++; //  Update bmoUnifiedShader PC.  
}

void bmoUnifiedShader::shTXP(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    Vec4FP32 op1;

    //  Read instruction operands. 
    bmUnifiedShader.read1Operands(shInstr, bmUnifiedShader, op1);

    //  Project texture coordinate.  
    op1[0] = op1[0]/op1[3];
    op1[1] = op1[1]/op1[3];
    op1[2] = op1[2]/op1[3];

    //  Add the texture access to the queue.  
    bmUnifiedShader.textureOperation(shInstr, TEXTURE_READ, op1, 0.0f);

    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shTXL(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    Vec4FP32 op1;

    //  Read instruction operands. 
    bmUnifiedShader.read1Operands(shInstr, bmUnifiedShader, op1);

    //  Add the texture access to the queue.  
    bmUnifiedShader.textureOperation(shInstr, TEXTURE_READ_WITH_LOD, op1, op1[3]);

    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shKIL(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    Vec4FP32 op1;

    //  Read instruction operands.  
    bmUnifiedShader.read1Operands(shInstr, bmUnifiedShader, op1);

    //  Check kill condition.  
    if ((op1[0] < 0.0f) || (op1[1] < 0.0f) || (op1[2] < 0.0f) || (op1[3] < 0.0f))
    {
        //  Set the thread kill flag.  
        bmUnifiedShader.kill[shInstr.getNumThread()][bmUnifiedShader.sampleIdx[shInstr.getNumThread()]] = TRUE;
    }

    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shKLS(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    Vec4FP32 op1;
    U32 sampleId;

    //  Read instruction operands.  
    bmUnifiedShader.read1Operands(shInstr, bmUnifiedShader, op1);

    //  Read KLS's sample identifier.  
    sampleId = shInstr.getShaderInstruction()->getOp2();

    //  Check kill condition.  
    if ((op1[0] < 0.0f) || (op1[1] < 0.0f) || (op1[2] < 0.0f) || (op1[3] < 0.0f))
    {
        //  Set the thread kill flag.  
        bmUnifiedShader.kill[shInstr.getNumThread()][sampleId] = TRUE;
    }

    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shZXP(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    Vec4FP32 op1;

    //  Read instruction operands.  
    bmUnifiedShader.read1Operands(shInstr, bmUnifiedShader, op1);

    //  Export Z value  
    bmUnifiedShader.zexport[shInstr.getNumThread()][0] = (F32) op1[0];

    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shZXS(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    Vec4FP32 op1;
    U32 sampleId;

    //  Read instruction operands.  
    bmUnifiedShader.read1Operands(shInstr, bmUnifiedShader, op1);

    //  Read ZXS's sample identifier.  
    sampleId = shInstr.getShaderInstruction()->getOp2();

    //  Export Z values  
    for (U32 i = 0; i < 4; i++)
        bmUnifiedShader.zexport[shInstr.getNumThread()][(bmUnifiedShader.sampleIdx[shInstr.getNumThread()]/4) + i] = (F32) op1[i];

    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shCMP(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    Vec4FP32 op1, op2, op3;
    F32 result[4];

    //  Read instruction operands. 
    bmUnifiedShader.read3Operands(shInstr, bmUnifiedShader, op1, op2, op3);

    //  Perform instruction operation.  
    GPUMath::CMP(op1.getVector(), op2.getVector(),op3.getVector(), result);

    //  Write instruction result.  
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);
//PRINT_OPERATION_RES_3
    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shCMPKIL(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    Vec4FP32 op1, op2, op3;
    F32 result[4];

    //  Read instruction operands. 
    bmUnifiedShader.read3Operands(shInstr, bmUnifiedShader, op1, op2, op3);

    //  Perform instruction operation.  
    GPUMath::CMP(op1.getVector(), op2.getVector(),op3.getVector(), result);

    //  Write instruction result.  
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);
//PRINT_OPERATION_RES_3
    //  Check kill condition.  
    if (((shInstr.getShaderInstruction()->getResultMaskMode() & 0x08) && (result[0] < 0.0f)) ||
        ((shInstr.getShaderInstruction()->getResultMaskMode() & 0x04) && (result[1] < 0.0f)) ||
        ((shInstr.getShaderInstruction()->getResultMaskMode() & 0x02) && (result[2] < 0.0f)) ||
        ((shInstr.getShaderInstruction()->getResultMaskMode() & 0x01) && (result[3] < 0.0f)))
    {
        //  Set the thread kill flag.  
        bmUnifiedShader.kill[shInstr.getNumThread()][bmUnifiedShader.sampleIdx[shInstr.getNumThread()]] = TRUE;
    }

    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shCHS(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    //  NO INSTRUCTION OPERANDS TO READ.  

    //  Point sample index to the next.  
    bmUnifiedShader.sampleIdx[shInstr.getNumThread()]++;

    //  Update bmoUnifiedShader PC.  
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shLDA(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    Vec4FP32 op1;

    //  Read instruction operands.
    bmUnifiedShader.read1Operands(shInstr, bmUnifiedShader, op1);

    //  Add the texture access to the queue.
    bmUnifiedShader.textureOperation(shInstr, ATTRIBUTE_READ, op1, 0.0f);

    //  Update bmoUnifiedShader PC.
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}


void bmoUnifiedShader::shFXMUL(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    Vec4FP32 op1, op2;
    FixedPoint fxpOp1[4];
    FixedPoint fxpOp2[4];
    FixedPoint fxpResult[4];

    //  Read instruction operands (fp 32-bit).
    bmUnifiedShader.read2Operands(shInstr, bmUnifiedShader, op1, op2);

    //  For all components of the registers.
    for(U32 c = 0; c < 4; c++)
    {
        //  Convert first operand to fixed point.
        fxpOp1[c] = FixedPoint(op1[c], 16, bmUnifiedShader.fxpDecBits);

        //  Convert second operand to fixed point.
        fxpOp2[c] = FixedPoint(op2[c], 16, bmUnifiedShader.fxpDecBits);

        //  Initialize the result with 2x precision.
        fxpResult[c] = FixedPoint(1.0f, 32, 2 * bmUnifiedShader.fxpDecBits);

        //  Perform multiplication with 2x precision.
        fxpResult[c] = fxpResult[c] * fxpOp1[c] * fxpOp2[c];
    }

    //  Write instruction result.
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, fxpResult);
//PRINT_FXMUL
    //  Update bmoUnifiedShader PC.
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shFXMAD(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    Vec4FP32 op1, op2;
    FixedPoint fxpOp1[4];
    FixedPoint fxpOp2[4];
    FixedPoint fxpOp3[4];
    FixedPoint fxpResult[4];
    F32 result[4];

    //  NOTE:  No swizzle or modifiers for third operand supported.

    //  Read instruction operands (fp 32-bit).
    bmUnifiedShader.read2Operands(shInstr, bmUnifiedShader, op1, op2);

    //  For all components of the registers.
    for(U32 c = 0; c < 4; c++)
    {
        //  Convert first operand to fixed point.
        fxpOp1[c] = FixedPoint(op1[c], 16, bmUnifiedShader.fxpDecBits);

        //  Convert second operand to fixed point.
        fxpOp2[c] = FixedPoint(op2[c], 16, bmUnifiedShader.fxpDecBits);

        //  Get third operand from the fixed point quad accumulator per thread.
        fxpOp3[c] = bmUnifiedShader.accumFXPBank[shInstr.getNumThread()][c];

        //  Initialize the result with 2x precision.
        fxpResult[c] = FixedPoint(1.0f, 32, 2 * bmUnifiedShader.fxpDecBits);

        //  Perform multiplication and addition with 32.16 precision.
        fxpResult[c] = fxpResult[c] * fxpOp1[c] * fxpOp2[c] + fxpOp3[c];

        //  Convert to fp 32-bit.
        result[c] = fxpResult[c].toFloat32();
    }

    //  Write instruction result.
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);
//PRINT_FXMAD
    //  Update bmoUnifiedShader PC.
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shFXMAD2(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    Vec4FP32 op1, op2, op3;
    FixedPoint fxpOp1[4];
    FixedPoint fxpOp2[4];
    FixedPoint fxpOp3[4];
    FixedPoint fxpResult[4];

    //  Read instruction operands (fp 32-bit).
    bmUnifiedShader.read3Operands(shInstr, bmUnifiedShader, op1, op2, op3);

    //  For all components of the registers.
    for(U32 c = 0; c < 4; c++)
    {
        //  Convert first operand to fixed point.
        fxpOp1[c] = FixedPoint(op1[c], 16, bmUnifiedShader.fxpDecBits);

        //  Convert second operand to fixed point.
        fxpOp2[c] = FixedPoint(op2[c], 16, bmUnifiedShader.fxpDecBits);

        //  Convert third operand to fixed point.
        fxpOp3[c] = FixedPoint(op3[c], 16, bmUnifiedShader.fxpDecBits);

        //  Initialize the result with 2x precision.
        fxpResult[c] = FixedPoint(1.0f, 32, 2 * bmUnifiedShader.fxpDecBits);

        //  Perform multiplication and addition with 32.16 precision.
        fxpResult[c] = fxpResult[c] * fxpOp1[c] * fxpOp2[c] + fxpOp3[c];
    }

    //  Write instruction result.
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, fxpResult);
//PRINT_FXMAD2
    //  Update bmoUnifiedShader PC.
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shEND(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    //  This instruction does nothing in the behaviorModel side.  
}

void bmoUnifiedShader::shSETPEQ(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    F32 op1;
    F32 op2;
    bool result;

    //  Read instruction first scalar operand.
    bmUnifiedShader.readScalarOp1(shInstr, bmUnifiedShader, op1);

    //  Read instruction second scalar operand.
    bmUnifiedShader.readScalarOp2(shInstr, bmUnifiedShader, op2);

    //  Perform instruction operation.
    GPUMath::SETPEQ(op1, op2, result);

    //  Write instruction result.
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);

    //  Update bmoUnifiedShader PC.
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shSETPGT(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    F32 op1;
    F32 op2;
    bool result;

    //  Read instruction first scalar operand.
    bmUnifiedShader.readScalarOp1(shInstr, bmUnifiedShader, op1);

    //  Read instruction second scalar operand.
    bmUnifiedShader.readScalarOp2(shInstr, bmUnifiedShader, op2);

    //  Perform instruction operation.
    GPUMath::SETPGT(op1, op2, result);

    //  Write instruction result.
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);

    //  Update bmoUnifiedShader PC.
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shSETPLT(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    F32 op1;
    F32 op2;
    bool result;

    //  Read instruction first scalar operand.
    bmUnifiedShader.readScalarOp1(shInstr, bmUnifiedShader, op1);

    //  Read instruction second scalar operand.
    bmUnifiedShader.readScalarOp2(shInstr, bmUnifiedShader, op2);

    //  Perform instruction operation.
    GPUMath::SETPLT(op1, op2, result);

    //  Write instruction result.
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);

    //  Update bmoUnifiedShader PC.
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shANDP(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    bool op1;
    bool op2;
    bool result;

    //  Read instruction first scalar operand.
    bmUnifiedShader.readScalarOp1(shInstr, bmUnifiedShader, op1);

    //  Read instruction second scalar operand.
    bmUnifiedShader.readScalarOp2(shInstr, bmUnifiedShader, op2);

    //  Perform instruction operation.
    GPUMath::ANDP(op1, op2, result);

    //  Write instruction result.
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);

    //  Update bmoUnifiedShader PC.
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shJMP(cg1gpu::cgoShaderInstr::cgoShaderInstrEncoding &shInstr, cg1gpu::bmoUnifiedShader &bmUnifiedShader)
{
    //  This instruction affects control and multiple 'threads' so it has to be implemented
    //  outside the behaviorModel.
}

void bmoUnifiedShader::shDDX(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    Vec4FP32 op1;

    //  Read instruction operands.
    bmUnifiedShader.read1Operands(shInstr, bmUnifiedShader, op1);

    //  Add the derivation operation to the current derivation structure.
    bmUnifiedShader.derivOperation(shInstr, op1);

    //  Update bmoUnifiedShader PC.
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shDDY(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    Vec4FP32 op1;

    //  Read instruction operands.
    bmUnifiedShader.read1Operands(shInstr, bmUnifiedShader, op1);

    //  Add the derivation operation to the current derivation structure.
    bmUnifiedShader.derivOperation(shInstr, op1);

    //  Update bmoUnifiedShader PC.
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shADDI(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    Vec4Int op1;
    Vec4Int op2;
    S32 result[4];
    
    //  Read instruction operands.
    bmUnifiedShader.read2Operands(shInstr, bmUnifiedShader, op1, op2);

    //  Perform instruction operation.
    GPUMath::ADDI(op1.getVector(), op2.getVector(), result);

    //  Write instruction result.
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);

    //  Update bmoUnifiedShader PC.
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shMULI(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    Vec4Int op1;
    Vec4Int op2;
    S32 result[4];
        
    //  Read instruction operands.
    bmUnifiedShader.read2Operands(shInstr, bmUnifiedShader, op1, op2);
    
    //  Perform instruction operation.
    GPUMath::MULI(op1.getVector(), op2.getVector(), result);

    //  Write instruction result.
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);

    //  Update bmoUnifiedShader PC.
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shSTPEQI(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    S32 op1;
    S32 op2;
    bool result;

    //  Read instruction first scalar operand.
    bmUnifiedShader.readScalarOp1(shInstr, bmUnifiedShader, op1);

    //  Read instruction second scalar operand.
    bmUnifiedShader.readScalarOp2(shInstr, bmUnifiedShader, op2);

    //  Perform instruction operation.
    GPUMath::STPEQI(op1, op2, result);

    //  Write instruction result.
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);

    //  Update bmoUnifiedShader PC.
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shSTPGTI(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    S32 op1;
    S32 op2;
    bool result;

    //  Read instruction first scalar operand.
    bmUnifiedShader.readScalarOp1(shInstr, bmUnifiedShader, op1);

    //  Read instruction second scalar operand.
    bmUnifiedShader.readScalarOp2(shInstr, bmUnifiedShader, op2);

    //  Perform instruction operation.
    GPUMath::STPGTI(op1, op2, result);

    //  Write instruction result.
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);

    //  Update bmoUnifiedShader PC.
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}

void bmoUnifiedShader::shSTPLTI(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    S32 op1;
    S32 op2;
    bool result;

    //  Read instruction first scalar operand.
    bmUnifiedShader.readScalarOp1(shInstr, bmUnifiedShader, op1);

    //  Read instruction second scalar operand.
    bmUnifiedShader.readScalarOp2(shInstr, bmUnifiedShader, op2);

    //  Perform instruction operation.
    GPUMath::STPEQI(op1, op2, result);

    //  Write instruction result.
    bmUnifiedShader.writeResult(shInstr, bmUnifiedShader, result);

    //  Update bmoUnifiedShader PC.
    bmUnifiedShader.PCTable[shInstr.getNumThread()]++;
}


void bmoUnifiedShader::shIllegal(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader)
{
    CG_ASSERT("Instruction not implemented.");
}





//    shIllegal                                       /*  Opcodes 2Ch - 2Fh  


/*
 *  Shader Instruction Emulation functions table.
 *
 */

void (*bmoUnifiedShader::shInstrEmulationTable[])(cgoShaderInstr::cgoShaderInstrEncoding &, bmoUnifiedShader &) =
{
    shNOP,    shADD,     shADDI,   shARL,       //  Opcodes 00h - 03h
    shANDP,   shIllegal, shNOP,    shCOS,       //  Opcodes 04h - 07h
    shDP3,    shDP4,     shDPH,    shDST,       //  Opcodes 08h - 0Bh
    shEX2,    shEXP,     shFLR,    shFRC,       //  Opcodes 0Ch - 0Fh

    shLG2,    shLIT,     shLOG,    shMAD,       //  Opcodes 10h - 13h
    shMAX,    shMIN,     shMOV,    shMUL,       //  Opcodes 14h - 17h
    shMULI,   shRCP,     shNOP,    shRSQ,       //  Opcodes 18h - 1Bh
    shSETPEQ, shSETPGT,  shSGE,    shSETPLT,    //  Opcodes 1Ch - 1Fh

    shSIN,    shSTPEQI,  shSLT,    shSTPGTI,    //  Opcodes 20h - 23h
    shSTPLTI, shTXL,     shTEX,    shTXB,       //  Opcoded 24h - 27h
    shTXP,    shKIL,     shKLS,    shZXP,       //  Opcodes 28h - 2Bh
    shZXS,    shCMP,     shCMPKIL, shCHS,       //  Opcodes 2Ch - 2Fh
    shLDA,    shFXMUL,   shFXMAD,  shFXMAD2,    //  Opcodes 30h - 33h
    shDDX,    shDDY,     shJMP,    shEND        //  Opcodes 34h - 37h
};

