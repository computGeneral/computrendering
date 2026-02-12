/**************************************************************************
 * Shader Unit Behavior Model.
 * Defines the bmoUnifiedShader class.  This class provides services
 * to emulate the functional behaviour of a Shader unit.
 */

#include <functional>
#include "bmMduBase.h"
#include "GPUType.h"
#include "support.h"
#include "Vec4FP32.h"
#include "Vec4Int.h"
#include "GPUMath.h"
#include "bmTextureProcessor.h"
#include "GPUReg.h"
#include "ShaderInstr.h"
#include "DynamicObject.h"
#include "FixedPoint.h"

#ifndef __BM_SHADER_H__
#define __BM_SHADER_H__

namespace arch
{

//  Defines the shader models supported.  
enum bmeShaderModel
{
    UNIFIED,        //  Unified shader model.  
    UNIFIED_MICRO   //  Unified shader model with wider (3x) shader input vector.  
};

//*  Defines the maximum number of texture accesses supported on fly.  
static const U32 TEXT_QUEUE_SIZE = 8192;

/**
 *  This class defines a Shader Behavior Model.
 *
 *  A Shader Behavior Model contains the functions and structures
 *  to emulate the functional behaviour of a Shader (Vertex
 *  or Pixel Shader) following a stream based programming
 *  model.
 *
 */

class cgoShaderInstr;    //  For cross declaration problems ... 


/**
 *  Defines a queued texture access for a stamp.
 */
struct TextureQueue
{
    TextureOperation texOp;         //  Defines the operation requested to the Texture Unit.  
    Vec4FP32 *coordinates;         //  Pointer to the parameters (coordinates, index) for the operation.  
    F32 *parameter;              //  Pointer to the per fragment parameter (lod/bias) for the operation (only for texture reads).  
    U32 textUnit;                //  Identifier of the texture sampler (texture reads) or attribute (attribute read) to be used for the operation.  
    U32 requested;               //  Number of requests received for the stamp.  
    bool vertexTextureAccess;       //  The texture access comes from a vertex shader (only for the CG1 behaviorModel!!!).  
    cgoShaderInstr::cgoShaderInstrEncoding **shInstrD; //  Pointer to the shader instructions that produced the texture access.  
    DynamicObject *accessCookies;                           //  Cookies for the texture access.  
};


/**
 *  Stores information about an on-going derivative computation operation (DDX and DDY).
 */
struct DerivativeInfo
{
    Vec4FP32 input[4];     //  Stores the four input values for the derivation operation.  
    U32 baseThread;      //  Stores the base thread/element for the derivation operation.  
    U32 derived;         //  Stores the number of derivation operations already received.  
    cgoShaderInstr::cgoShaderInstrEncoding **shInstrD;     //  Pointer to the shader instructions corresponding with the derivation operation.  
};

/*  Defines the state partitions (instruction memory and constants) for
    unified shader model.  */
enum
{
    VERTEX_PARTITION = 0,       //  Identifies the partition for vertices.  
    FRAGMENT_PARTITION = 1,     //  Identifies the partition for fragments.  
    TRIANGLE_PARTITION = 2,     //  Identifies the partition for triangles.  
    MICROTRIANGLE_PARTITION = 3 //  Identifies the partition for microtriangle fragments.  
};

class bmoUnifiedShader : public bmoMduBase
{
private:
    static const U32 UNIFIED_TEMP_REG_SIZE = 16;         //  Defines the size in bytes of a temporary register.  
    static const U32 UNIFIED_CONST_REG_SIZE = 16;        //  Defines the size in bytes of a constant register.     
    
    //  Shader parameters.
    char*           name;               //  Shader name. 
    U32             numThreads;         //  Number of active threads supported by the shader.  
    U32             stampFragments;     //  Number of fragments per stamp for mipmaped texture access.  
    U32             fxpDecBits;         //  Decimal bits for the FixedPoint Accumulator register.  
    bool            storeDecodedInstr;  //  Stores if the decoded shader instructions are stored in an array for future use.  
    bmeShaderModel  model;              //  Shader model. 
    //bmoTextureProcessor*     bmTextureProcessor; //  Pointer to the texture behaviorModel attached to the shader.  

    //  Shader Model parameters
    U32 instructionMemorySize;   //  Size (in instructions) of the Instruction Memory.  
    U32 numInputRegs;            //  Number of registers in the Input Bank.  
    U32 numOutputRegs;           //  Number of registers in the Output Bank.  
    U32 numTemporaryRegs;        //  Number of registers in the Temporary Register Bank.  
    U32 numConstantRegs;         //  Number of constants in the Constant Register Bank.  
    U32 numAddressRegs;          //  Number of registers in the Address Register Bank.  
    U32 numPredicateRegs;        //  Number of registers in the Predicate Register bank.  

    //  Shader State.
    cgoShaderInstr **InstrMemory;      // Shader Instruction Memory  
    cgoShaderInstr::cgoShaderInstrEncoding ***InstrDecoded; //  Decoded, per thread shader instructions.  
    Vec4FP32 **inputBank;                      //  Shader Input Register Bank.  
    Vec4FP32 **outputBank;                     //  Shader Output Register Bank. 
    U08 *temporaryBank;                       //  Shader Temporary Register Bank.  
    U08 *constantBank;                        //  Shader Constant Register Bank.  
    Vec4Int **addressBank;                      //  Shader Address Register Bank (integer).  
    FixedPoint **accumFXPBank;                  //  FixedPoint Accumulator (implicit) bank.  
    bool **predicateBank;                       //  Shader Predicate Register bank.  
    U32 *PCTable;                            //  Thread PC table.  
    bool **kill;                                //  Stores the thread (per-sample) kill flag that marks the thread for premature ending.  
    F32 **zexport;                           //  Stores the thread (per-sample) z export value (MicroPolygon rasterizer).  
    U32 *sampleIdx;                          //  Stores the thread current sample identifier pointer (changed/incremented by the CHS instruction).  
    TextureQueue textQueue[TEXT_QUEUE_SIZE];    //  Texture access queue.  
    U32 freeTexture[TEXT_QUEUE_SIZE];        //  List of free entries in the texture queue.  
    U32 waitTexture[TEXT_QUEUE_SIZE];        //  List of entries in the texture queue waiting to be processed.  
    U32 firstFree;                           //  Pointer to the first entry in the free texture list.  
    U32 lastFree;                            //  Pointer to the last entry in the free texture list.  
    U32 numFree;                             //  Number of free entries in the texture queue.  
    U32 firstWait;                           //  Pointer to the first entry in the wait texture list.  
    U32 lastWait;                            //  Pointer to the last entry in the wait texture list.  
    U32 numWait;                             //  Number of entries waiting to be processed in the texture queue.  

    DerivativeInfo  currentDerivation;          //  Stores the information related with the current derivation operation.  

    std::function<void(void)> ArchModelSMKick;

    /**
     *  Shader Instruction Emulation functions table.
     *
     *  This table stores pointers to the functions that
     *  emulate the different shader instructions.  They
     *  are ordered by opcode so they can be accessed
     *  using the decoded cgoShaderInstr opcode field.
     *
     */

    static void (*shInstrEmulationTable[])(cgoShaderInstr::cgoShaderInstrEncoding &, bmoUnifiedShader &);

    /**
     *  Shader Instruction Emulation functions table.  For instructions that set
     *  the condition codes.
     *
     *  This table stores pointer to the functions that emulate the different
     *  shader instructions.  They are ordered by opcode so they can be accessed
     *  using the decoded cgoShaderInstr opcode field.
     *  This table contains functions for emulating the version of
     *  the instruction that updates the condition code register.
     */

    static void (*shInstrEmulationTableCC[])(cgoShaderInstr::cgoShaderInstrEncoding &, bmoUnifiedShader &);

    //                     
    //  PRIVATE FUNCTIONS  
    //                     

    /**
     *  Sets the pointer to the emulated function for the instruction
     *  in the table for predecoded instructions.
     *
     *  \param shInstr  Shader Instruction for which select an emulation
     *  function.
     *
     */

    void setEmulFunction(cgoShaderInstr::cgoShaderInstrEncoding *shInstr);

    /*
     *  Decodes and returns the address to the register that the instruction is
     *  going to access when executed (per thread!).
     *
     *  @param numThread  Thread in which the instruction is going to be executed.
     *  @param bank Bank of the register to be decoded.
     *  @param reg Register to in the bank to be decoded.
     *  @param partition Shader partition in unified shader model to be used.
     *
     *  @return A pointer (the address) to the register.
     *
     */

    void *decodeOpReg(U32 numThread, Bank bank, U32 reg, U32 partition = VERTEX_PARTITION);

    /**
     *
     *  Adds a texture operation for a shader processing element to the texture queue.
     *
     *  @param shInstr Reference to the instruction producing the texture operation.
     *  @param operation Type of operation requested to the texture unit.
     *  @param coord Coordinates of the texture access/Index for the attribute load.
     *  @param parameter Per fragment parameter (lod/bias) for the texture access.
     *
     */

    void textureOperation(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, TextureOperation operation, Vec4FP32 coord, F32 parameter);

    /**
     *
     *  Adds the information about a derivation operation to the current derivation structure.
     *
     *  @param shInstr Reference to the shader instruction corresponding with the derivation operation.
     *  @param input Input values for the derivation operation.
     *
     */
    
    void derivOperation(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, Vec4FP32 input);     
     
    /**
     *  Swizzle function.  Reorders and replicates the components of a 4D
     *  float point vector.
     *
     */

    Vec4FP32 swizzle(SwizzleMode mode, Vec4FP32 qf);

    /**
     *  Swizzle function.  Reorders and replicates the components of a 4D
     *  integer vector.
     *
     */

    Vec4Int swizzle(SwizzleMode mode, Vec4Int qi);

    /**
     *  Negate function.  Negates the 4 components in a 4D float point vector.
     *
     */

    Vec4FP32 negate(Vec4FP32 qf);

    /**
     *  Negate function.  Negates the 4 components in a 4D integer vector.
     *
     */

    Vec4Int negate(Vec4Int qi);

    /**
     *  Absolute function.  Returns the absolute value for the 4 components
     *  in a 4D float point vector.
     *
     */

    Vec4FP32 absolute(Vec4FP32 qf);

    /**
     *  Absolute function.  Returns the absolute value for the 4 components
     *  in a 4D float point vector.
     *
     */

    Vec4Int absolute(Vec4Int qi);
    
    /**
     *  Write Mask for VS2 Shader Model.  Components in the outpyt 4D
     *  float point register are written only if the write mask is
     *  enabled for that component and the condition mask is true for the
     *  corresponding swizzled component of the condition code register.
     *
     */

    void writeResReg(cgoShaderInstr &shInstr, F32 *f, F32 *res, bool predicate = true);

    /**
     *  Write Mask for VS2 Shader Model.  Components in the outpyt 4D
     *  integer register are written only if the write mask is
     *  enabled for that component and the condition mask is true for the
     *  corresponding swizzled component of the condition code register.
     *
     */

    void writeResReg(cgoShaderInstr &shInstr, S32 *f, S32 *res, bool predicate = true);

    /**
     *  Write Mask for VS2 Shader Model.  Components in the outpyt 4D
     *  float point register are written only if the write mask is
     *  enabled for that component and the condition mask is true for the
     *  corresponding swizzled component of the condition code register.
     *
     */

    void writeResReg(cgoShaderInstr &shInstr, FixedPoint *f, FixedPoint *res, bool predicate = true);

    /**
     *  Read the first operand value of the instruction.
     *
     *  @param shInstr  Referente to the shader instruction.
     *  @param numThread  Thread in which the instruction is executed.
     *  @param PC PC of the executed instruction.
     *  @param op Reference to the Vec4FP32 where to store the first operand.
     *
     */

    void readOperand1(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader, Vec4FP32 &op);

    /**
     *  Read the second operand value of the instruction.
     *
     *  @param shInstr  Referente to the shader instruction.
     *  @param numThread  Thread in which the instruction is executed.
     *  @param PC PC of the executed instruction.
     *  @param op Reference to the Vec4FP32 where to store the second operand.
     *
     */

    void readOperand2(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader, Vec4FP32 &op);

    /**
     *  Read the third operand value of the instruction.
     *
     *  @param shInstr  Referente to the shader instruction.
     *  @param numThread  Thread in which the instruction is executed.
     *  @param PC PC of the executed instruction.
     *  @param op Reference to the Vec4FP32 where to store the third operand.
     *
     */

    void readOperand3(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader, Vec4FP32 &op);

    /**
     *  Read the first operand value of the instruction.
     *
     *  @param shInstr  Referente to the shader instruction.
     *  @param numThread  Thread in which the instruction is executed.
     *  @param PC PC of the executed instruction.
     *  @param op Reference to the Vec4Int where to store the first operand.
     *
     */

    void readOperand1(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader, Vec4Int &op);

    /**
     *  Read the second operand value of the instruction.
     *
     *  @param shInstr  Referente to the shader instruction.
     *  @param numThread  Thread in which the instruction is executed.
     *  @param PC PC of the executed instruction.
     *  @param op Reference to the Vec4Int where to store the second operand.
     *
     */

    void readOperand2(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader, Vec4Int &op);

    /**
     *
     *  This function reads and returns the quadfloat operand of a single operand shader instruction.
     *  THIS FUNCTION SHOULD BE INLINED.
     *
     *  @param shInstr Reference to a shader instruction.
     *  @param bmUnifiedShader Reference to the bmoUnifiedShader object where the instruction is executed.
     *  @param op1 A reference to a Vec4FP32 to return the instruction first operand value.
     *
     */

    void read1Operands(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader, Vec4FP32 &op1);

    /**
     *
     *  This function reads and returns the two quadfloat operands of a two operands shader instruction.
     *  THIS FUNCTION SHOULD BE INLINED.
     *
     *  @param shInstr Reference to a shader instruction.
     *  @param bmUnifiedShader Reference to the bmoUnifiedShader object where the instruction is executed.
     *  @param op1 A reference to a Vec4FP32 to return the instruction first operand value.
     *  @param op2 A reference to a Vec4FP32 to return the instruction second operand value.
     *
     */

    void read2Operands(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader, Vec4FP32 &op1, Vec4FP32 &op2);

    /**
     *
     *  This function reads and returns the three quadfloat operands of a three operands shader instruction.
     *  THIS FUNCTION SHOULD BE INLINED.
     *
     *  @param shInstr Reference to a shader instruction.
     *  @param bmUnifiedShader Reference to the bmoUnifiedShader object where the instruction is executed.
     *  @param op1 A reference to a Vec4FP32 to return the instruction first operand value.
     *  @param op2 A reference to a Vec4FP32 to return the instruction second operand value.
     *  @param op3 A reference to a Vec4FP32 to return the instruction third operand value.
     *
     */

    void read3Operands(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader,
                       Vec4FP32 &op1, Vec4FP32 &op2, Vec4FP32 &op3);

    /**
     *
     *  This function reads and returns the two quadint operands of a two operands shader instruction.
     *  THIS FUNCTION SHOULD BE INLINED.
     *
     *  @param shInstr Reference to a shader instruction.
     *  @param bmUnifiedShader Reference to the bmoUnifiedShader object where the instruction is executed.
     *  @param op1 A reference to a Vec4Int to return the instruction first operand value.
     *  @param op2 A reference to a Vec4Int to return the instruction second operand value.
     *
     */

    void read2Operands(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader, Vec4Int &op1, Vec4Int &op2);

    /**
     *
     *  This function reads and returns the first float point scalar operand for a shader instruction.
     *  THIS FUNCTION SHOULD BE INLINED.
     *
     *  @param shInstr Reference to a shader instruction.
     *  @param bmUnifiedShader Reference to the bmoUnifiedShader object where the instruction is executed.
     *  @param op A reference to a 32 bit float variable where to return the instruction scalar operand value.
     *
     */

    void readScalarOp1(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader, F32 &op);

    /**
     *
     *  This function reads and returns the second float point scalar operand for a shader instruction.
     *  THIS FUNCTION SHOULD BE INLINED.
     *
     *  @param shInstr Reference to a shader instruction.
     *  @param bmUnifiedShader Reference to the bmoUnifiedShader object where the instruction is executed.
     *  @param op A reference to a 32 bit float variable where to return the instruction scalar operand value.
     *
     */

    void readScalarOp2(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader, F32 &op);


    /**
     *
     *  This function reads and returns the first integer scalar operand for a shader instruction.
     *  THIS FUNCTION SHOULD BE INLINED.
     *
     *  @param shInstr Reference to a shader instruction.
     *  @param bmUnifiedShader Reference to the bmoUnifiedShader object where the instruction is executed.
     *  @param op A reference to a 32 bit integer variable where to return the instruction scalar operand value.
     *
     */

    void readScalarOp1(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader, S32 &op);

    /**
     *
     *  This function reads and returns the second integer scalar operand for a shader instruction.
     *  THIS FUNCTION SHOULD BE INLINED.
     *
     *  @param shInstr Reference to a shader instruction.
     *  @param bmUnifiedShader Reference to the bmoUnifiedShader object where the instruction is executed.
     *  @param op A reference to a 32 bit integer variable where to return the instruction scalar operand value.
     *
     */

    void readScalarOp2(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader, S32 &op);

    /**
     *
     *  This function reads and returns the first boolean scalar operand for a shader instruction.
     *
     *  @param shInstr Reference to a shader instruction.
     *  @param bmUnifiedShader Reference to the bmoUnifiedShader object where the instruction is executed.
     *  @param op A reference to a boolean varable where to return the instruction first boolean scalar operand value.
     *
     */
    void readScalarOp1(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader, bool &op);

    /**
     *
     *  This function reads and returns the second boolean scalar operand for a shader instruction.
     *
     *  @param shInstr Reference to a shader instruction.
     *  @param bmUnifiedShader Reference to the bmoUnifiedShader object where the instruction is executed.
     *  @param op A reference to a boolean varable where to return the instruction second boolean scalar operand value.
     *
     */
    void readScalarOp2(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader, bool &op);

    /**
     *
     *  This function writes a shader instruction quadfloat result.
     *  THIS FUNCTION SHOULD BE INLINED.
     *
     *  @param shInstr Reference to the shader instruction.
     *  @param bmUnifiedShader Reference to the bmoUnifiedShader object where the instruction is executed.
     *  @param result A pointer to a four component float point
     *  result.
     *
     */

    void writeResult(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader, F32 *result);

    /**
     *
     *  This function writes a shader instruction quadint result.
     *  THIS FUNCTION SHOULD BE INLINED.
     *
     *  @param shInstr Reference to the shader instruction.
     *  @param bmUnifiedShader Reference to the bmoUnifiedShader object where the instruction is executed.
     *  @param result A pointer to a four component float point
     *  result.
     *
     */

    void writeResult(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader, S32 *result);

    /**
     *
     *  This function writes a shader instruction quad fixed point result.
     *  THIS FUNCTION SHOULD BE INLINED.
     *
     *  @param shInstr Reference to the shader instruction.
     *  @param bmUnifiedShader Reference to the bmoUnifiedShader object where the instruction is executed.
     *  @param result A pointer to a four component fixed point result.
     *
     */

    void writeResult(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader, FixedPoint *result);

    /**
     *
     *  This function writes a shader instruction boolean result (predicate register).
     *  THIS FUNCTION SHOULD BE INLINED.
     *
     *  @param shInstr Reference to the shader instruction.
     *  @param bmUnifiedShader Reference to the bmoUnifiedShader object where the instruction is executed.
     *  @param result The result boolean value to write.
     *
     */

    void writeResult(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader, bool result);

    //  Shader Emulation Functions.  

    static void shNOP(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shADD(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shARL(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shCOS(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);

    static void shDP3(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shDP4(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shDPH(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shDST(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);

    static void shEX2(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shEXP(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shFLR(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shFRC(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);

    static void shLG2(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shLIT(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shLOG(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shMAD(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);

    static void shMAX(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shMIN(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shMOV(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shMUL(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);

    static void shRCP(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shRSQ(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shSGE(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shSIN(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);

    static void shSLT(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    
    static void shTEX(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shTXB(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shTXP(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shTXL(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    
    static void shKIL(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shKLS(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
   
    static void shZXP(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shZXS(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);    
    static void shCHS(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shCMP(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);

    static void shCMPKIL(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shLDA(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shFXMUL(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shFXMAD(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);

    static void shFXMAD2(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shEND(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);

    static void shSETPEQ(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shSETPGT(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shSETPLT(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shANDP(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shJMP(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    
    static void shDDX(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shDDY(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    
    static void shADDI(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shMULI(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shSTPEQI(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shSTPGTI(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    static void shSTPLTI(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);
    
    static void shIllegal(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, bmoUnifiedShader &bmUnifiedShader);

public:
    void ArchModelFunc(std::function<void(void)> ArchModelFunc) { ArchModelSMKick = ArchModelFunc; }

    /**
     *
     *   Constructor function for the class.
     *
     *   This function creates a multithreaded Shader Behavior Model for a type
     *   of Shader Behavior Model.  Allocates space for the input, output and other
     *   register banks of the Shader, and for the Shader Instruction Memory.
     *   All the resources that aren't shared between the threads are replicated.
     *
     *   @param name Name of the Shader.  Identifies the shader accessed.
     *   @param model Defines the shader model to use.
     *   @param numThreads Number of threads supported by a multithreaded shader.
     *   @param storeDecodedInstr Flag that defines if decoded instructions are stored for future use (may require a lot of memory space).
     *   @param textEmul Pointer to a texture behaviorModel attached to the shader.
     *   @param stampFrags Fragments per stamp for texture accesses.
     *   @param fxpDecBits Decimal bits for the FixedPoint Accumulator register.
     *
     *   @return Returns a new bmoUnifiedShader object.
     *
     */

    bmoUnifiedShader(char *name,
             bmeShaderModel model, 
             cgeModelAbstractLevel modelType = CG_BEHV_MODEL,
             U32 numThreads = 1, 
             bool storeDecodedInstr = false, 
             //bmoTextureProcessor *textEmul = NULL,
             U32 stampFrags = 4,
             U32 fxpDecBits = 16);

    /**
     *
     *  This function resets the Shader state (for example when a new vertex/pixel is
     *  loaded).  Temporal registers are loaded with default values.
     *
     *  \param numThread Identifies the thread which state should be reseted.
     *  \return Returns Nothing.
     *
     */

    void resetShaderState(U32 numThread);

    /**
     *
     *  This function loads register or parameters values into the Shader state.
     *
     *  \param numThread Identifies the thread which state should be modified.
     *  \param bank Bank identifier where the data is going to be written.
     *  \param reg  Register identifier where the data is going to be written.
     *  \param data Data to write (4D Float).
     *  \return Returns Nothing.
     *
     */

    void loadShaderState(U32 numThread, Bank bank, U32 reg, Vec4FP32 data);

    /**
     *
     *  This function loads register or parameters values into the Shader state.
     *
     *  \param numThread Identifies the thread which state should be modified.
     *  \param bank Bank identifier where the data is going to be written.
     *  \param reg  Register identifier where the data is going to be written.
     *  \param data Data to write (4D Integer).
     *  \return Returns Nothing.
     *
     */

    void loadShaderState(U32 numThread, Bank bank, U32 reg, Vec4Int data);

    /**
     *
     *  This function loads register or parameters values into the Shader state.
     *
     *  \param numThread Identifies the thread which state should be modified.
     *  \param bank Bank identifier where the data is going to be written.
     *  \param reg  Register identifier where the data is going to be written.
     *  \param data Data to write (Boolean).
     *  \return Returns Nothing.
     *
     */

    void loadShaderState(U32 numThread, Bank bank, U32 reg, bool data);

    /**
     *
     *  This function partially loads a register bank into the Shader state.
     *
     *  \param numThread Identifies the thread which state should be modified.
     *  \param bank Bank identifier where the data is going to be written.
     *  \param data Pointer to an array of Vec4FP32 values to write into the register.
     *  \param startReg First register to write.
     *  \param nRegs Number of data float vectors to copy in the register bank.
     *  \return Returns Nothing.
     *
     */

    void loadShaderState(U32 numThread, Bank bank, Vec4FP32 *data, U32 startReg, U32 nRegs);

    /**
     *
     *  This function loads a full register bank into the Shader state.
     *
     *  \param numThread Identifies the thread which state should be modified.
     *  \param bank Bank identifier where the data is going to be written.
     *  \param data Data to write (4D float).
     *  \return Returns Nothing.
     *
     */

    void loadShaderState(U32 numThread, Bank bank, Vec4FP32 *data);

    /**
     *
     *  This function reads register or parameters values from the Shader state.
     *
     *  \param numThread Identifies the thread which state should be read.
     *  \param bank Bank identifier from where the data is going to be read.
     *  \param reg  Register identifier from where the data is going to be read.
     *  \param value Reference to the Vec4FP32 where the value is going to be written.
     *  \return Returns Nothing.
     *
     */

    void readShaderState(U32 numThread, Bank bank, U32 reg, Vec4FP32 &value);

    /**
     *
     *  This function reads register or parameters values from the Shader state.
     *
     *  \param numThread Identifies the thread which state should be read.
     *  \param bank Bank identifier from where the data is going to be read.
     *  \param reg  Register identifier from where the data is going to be read.
     *  \param value  Reference to the Vec4Int where the value is going to be written.
     *  \return Returns Nothing.
     *
     */

    void readShaderState(U32 numThread, Bank bank, U32 reg, Vec4Int &value);


    /**
     *
     *  This function reads register or parameters values from the Shader state.
     *
     *  \param numThread Identifies the thread which state should be read.
     *  \param bank Bank identifier from where the data is going to be read.
     *  \param reg  Register identifier from where the data is going to be read.
     *  \param value Reference to the bool variable where the value is going to
     *  be written.
     *  \return Returns The value of the register (Boolean).
     *
     */

    void readShaderState(U32 numThread, Bank bank, U32 reg, bool &value);

    /**
     *
     *  This function reads a full register bank from the Shader state (4D float).
     *
     *  \param numThread Identifies the thread which state should be read.
     *  \param bank Bank identifier from where the data is going to be read.
     *  \param data Pointer to the array where the data is going to be returned.
     *  \return Returns Nothing.
     *
     */

    void readShaderState(U32 numThread, Bank bank, Vec4FP32 *data);

    /**
     *
     *  This function reads a full register bank from the Shader state (4D Integer).
     *
     *  \param numThread Identifies the thread which state should be read.
     *  \param bank Bank identifier from where the data is going to be read.
     *  \param data Pointer to the variable where the data is going to be returned.
     *  \return Returns Nothing.
     *
     */

    void readShaderState(U32 numThread, Bank bank, Vec4Int *data);

    /**
     *
     *  This function reads a full register bank from the Shader state (Boolean).
     *
     *  \param numThread Identifies the thread which state should be read.
     *  \param bank Bank identifier from where the data is going to be read.
     *  \param data Pointer to the variable where the data is going to be returned.
     *  \return Returns Nothing.
     *
     */

    void readShaderState(U32 numThread, Bank bank, bool *data);

    /**
     *
     *  This function loads a new Shader Program into the Shader.
     *
     *  @param code Pointer to a buffer with the shader program in binary format.
     *  @param address Address inside the shader program memory where to store the program.
     *  @param sizeCode Size of the Shader Program code in binary format (bytes).
     *  @param partition Shader partition for which to load the shader program.
     *
     *  @return Returns Nothing.
     *
     */

    void loadShaderProgram(U08 *code, U32 address, U32 sizeCode, U32 partition);

    /**
     *
     *  This function loads a new Shader Program into the Shader (instruction is not decoded).
     *
     *  @param code Pointer to a buffer with the shader program in binary format.
     *  @param address Address inside the shader program memory where to store the program.
     *  @param sizeCode Size of the Shader Program code in binary format (bytes).
     *
     *  @return Returns Nothing.
     *
     */

    void loadShaderProgramLight(U08 *code, U32 address, U32 sizeCode);

    /**
     *
     *  This functions decodes a shader instruction for a shader thread and returns the
     *  corresponding decoded shader instruction object.
     *
     *  @param shInstr The shader instruction to decode.
     *  @param address The address in shader instruction memory of the shader instruction to decode.
     *  @param nThread The shader thread for which the instruction is decoded.
     *  @param partition Defines the shader partition (vertex or fragment) to use for a given
     *  program for unified shader model.
     *
     *  @return A cgoShaderInstrEncoding object with the shader instruction decoded
     *  information.
     *
     */

    cgoShaderInstr::cgoShaderInstrEncoding *DecodeInstr(cgoShaderInstr *shInstr,
        U32 address, U32 nThread, U32 partition);

    /**
     *
     *  This function returns a Shader Instruction from the current
     *  Shader Instruction Memory.
     *
     *  \param threadId Identifier of the thread from which to fetch the instruction.
     *  \param PC Instruction Memory address from where to fetch the instruction.
     *  \return Returns a pointer to a cgoShaderInstr object with the decoded
     *  instruction that had to be read.
     *
     */

    cgoShaderInstr::cgoShaderInstrEncoding *FetchInstr(U32 threadID, U32 PC);

    /**
     *
     *  This function returns a Shader Instruction from the current
     *  Shader Instruction Memory (decode instruction at fetch version).
     *
     *  @param threadId Identifier of the thread from which to fetch the instruction.
     *  @param PC Instruction Memory address from where to fetch the instruction.
     *  @param partition Defines the shader partition (vertex or fragment) to use for a given
     *  program for unified shader model.
     *
     *  @return Returns a pointer to a cgoShaderInstr object with the decoded
     *  instruction that had to be read.
     *
     */

    cgoShaderInstr::cgoShaderInstrEncoding *fetchShaderInstructionLight(U32 threadID, U32 PC, U32 partition);

    /**
     *
     *  Read the shader instruction at the given instruction memory address for the defined partition.
     *
     *  @param pc Address of the shader instruction in the instruction memory.
     *
     *  @return Returns a pointer to the cgoShaderInstr in that position of the instruction memory.
     *
     */

    cgoShaderInstr *readShaderInstruction(U32 pc);

    /**
     *
     *  This function executes the instruction provided as parameter and
     *  updates the Shader State with the result of the instruction execution.
     *
     *  \param instruction  A cgoShaderInstr object that defines the instruction
     *  to be executed.
     *
     */

    void execShaderInstruction(cgoShaderInstr::cgoShaderInstrEncoding *instruction);

    /**
     *  This function returns the emulated PC stored for a Shader Behavior Model  thread.
     *  @param numThread The shader thread from which to get the PC.
     *  @return The emulated PC of the shader thread.
     */

    U32 GetThreadPC(U32 numThread);

    /**
     *  This functions sets the emulated PC for a thread shader in the behaviorModel.
     *  @param numThread The shader thread for which to set the PC.
     *  @param pc The new thread pc.
     */
    U32 setThreadPC(U32 numThread, U32 pc);

    /**
     *  This function returns the kill flag stored for a Shader Behavior Model thread.
     *  @param The shader thread from which to get the kill flag value.
     *  @return The kill flag of the shader thread.
     */
    bool threadKill(U32 numThread);

    /**
     *  This function returns the sample kill flag stored for a Shader Behavior Model thread.
     *  @param numThread The shader thread from which to get the kill flag value.
     *  @param sample The sample identifier.
     *  @return The sample kill flag of the shader thread.
     */
    bool threadKill(U32 numThread, U32 sample);

    /**
     *
     *  This function returns the sample z export value stored for a Shader Behavior Model thread.
     *
     *  @param numThread The shader thread from which to get the kill flag value.
     *  @param sample The sample identifier.
     *  @return The sample z export value of the shader thread.
     *
     */

    F32 threadZExport(U32 numThread, U32 sample);

    /**
    *
     *  Generates the next complete texture request for a stamp available in the shader behaviorModel.
     *
     *  @return A TextureAccess object with the information about the texture accesses for
     *  a whole stamp.
     *
     */

    TextureAccess *nextTextureAccess();

    /**
    *
     *  Generates the next complete texture request for a vertex texture access available in the shader behaviorModel.
     *  Only the CG1 behaviorModel should use this function.
     *
     *  @return A TextureAccess object with the information about the texture accesses for a vertex texture access.
     *
     */

    TextureAccess *nextVertexTextureAccess();

    /**
     *
     *  Completes a texture access writing the sample in the result register.
     *
     *  @param id Pointer to the texture queue entry to be completed and written
     *  to the thread register bank.
     *  @param sample Pointer to an array with the result of the texture access
     *  for the whole stamp represented by the texture queue entry to complete.
     *  @param threads Pointer to an array where to store the threads that generated
     *  the texture access.
     *  @param removeInstr Boolean value that defines if the shaded decoded instruction
     *  that generated the texture request has to be deleted.  Provided for behaviorModel
     *  support.
     *
     */

    void writeTextureAccess(U32 id, Vec4FP32 *sample, U32 *threads, bool removeInstr = true);

    /**
     *
     *  Checks a jump instruction condition for a vector and performs the jump if required.
     *
     *  @param shInstrDec Pointer to a shader decoded instruction correspoding with the representative
     *  instruction for the vector.
     *  @param vectorLenght Number of elements/threads per vector that must execute in lock-step.
     *  @param targetPC Reference to a variable where to store the destination PC of the jump.
     *
     *  @return Returns if the jump instruction condition was true and the PC changed to the
     *  jump destination PC.
     *
     */
     
    bool checkJump(cgoShaderInstr::cgoShaderInstrEncoding *shInstrDec, U32 vectorLenght, U32 &destPC);


};

} // namespace arch

#endif
