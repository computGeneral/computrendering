/**************************************************************************
 *
 * Shader Optimization
 *  Defines the ShaderOptimization class.
 *
 *  This class implements a number of optimization and code transformation passes for shader programs
 *  using the cgoShaderInstr class.
 *
 */


#ifndef _SHADEROPTIMIZATION_
#define _SHADEROPTIMIZATION_

#include "GPUType.h"
#include "GPUReg.h"
#include "ShaderInstr.h"

#include <vector>
#include <set>
#include <map>

using namespace std;

namespace arch
{

class ShaderOptimization
{
private:
    //  Instruction type enumerator
    enum 
    {
        CTRL_INSTR_TYPE,
        ALU_INSTR_TYPE,
        TEX_INSTR_TYPE
    };

    /**
     *  Structure defined to store information about (temporal) register usage in the program.
     *  Used by the dead code elmination optimization pass.
     */
    struct RegisterState
    {
        bool compWasWritten[4];       //  Defines if a component of the register was written/defined.  
        bool compWasRead[4];          //  Defines if a component of a register was read/used.  
        U32  compWriteInstruction[4]; //  Pointer (first instruction uses pointer value 1) to the last instruction in the program that wrote/defined the component.
    };

    /**
     *  Structure that stores what components of the result of a shader instruction can be eliminated from the
     *  write mask due to the component value not being used by the program.
     *  Used by the dead code elimination optimization pass.
     */
    struct InstructionInfo
    {
        bool removeResComp[4];      //  Defines if a component can be removed from the write mask of the shader instruction.  
    };

    /**
     *  Structure that stores the correspondance between a name component (an uniquely identified value created by a shader instruction
     *  as defined by the rename pass) and a temporal register.
     *  Used by the reduce live register usage optimization pass.
     */
    struct NameComponent
    {
        U32 name;                //  The name identifier.  
        U32 component;           //  The component identifier.  
        U32 freeFromInstr;       //  Index to the instruction after the last instruction that uses the name component.  
    };

    /**
     *  Structure that defines a temporal register component.
     *  Used by copy propagation optimization pass.
     */
    struct RegisterComponent
    {
        U32 reg;         //  Regsiter identifier.  
        U32 comp;        //  Register component identifier.  
    };

    /**
     *  Structure that stores information about how a register name and its components are created and used.
     *  Used by the reduce live register usage optimization pass.
     */
    struct NameUsage
    {
        U32 createdByInstr[4];               //  Stores the index (starting from 1) to the instruction that create the value of the name component.  
        vector <bool> usedByInstrOp1[4];     //  Vector that stores the name component usage in the first operand of the shader program instructions.  
        vector <bool> usedByInstrOp2[4];     //  Vector that stores the name component usage in the first operand of the shader program instructions.  
        vector <bool> usedByInstrOp3[4];     //  Vector that stores the name component usage in the first operand of the shader program instructions.  
        vector <U32> instrPackedCompUse;     //  Vector that stores the maximum number of components of the name are used in the same shader program instruction.  
        U32 maxPackedCompUse;                //  Stores the maximum number of components of the name used in any shader program instruction.  
        U32 lastUsedByInstr[4];              //  Stores the index (starting from 1) to the last shader program instruction that used the name component.  
        U32 copiedRegister[4];               //  Stores the register from which the value was copied.  
        U32 copiedComponent[4];              //  Stores the component from which the value was copied.  
        U32 copiedFromInstr[4];              //  Stores the index of the instruction that created the copied value.  
        vector<U32> copiedByInstr[4];        //  Stores a list of indices to instructions that copied the value.  
        vector<RegisterComponent> copies[4]; //  Stores a list with the copies of the value.  
        U32 masterName;                      //  Stores the identifier of the name that is defined as a master of an aggregation of names related by copy.  
        bool allocated;                      //  Stores if the name was assigned to a temporal register.  
        U32 allocReg[4];                     //  Stores the temporal register allocated for each name component.  
        U32 allocComp[4];                    //  Stores the component from the temporal register allocated for each name component.  
    };

    /**
     *  Structure that stores information about values created by the shader program and the copies (a copy is created by
     *  a CG1_ISA_OPCODE_MOV instruction) of such values. Used by the copy propagation optimization pass.
     */
    struct ValueCopy
    {
        U32 createdByInstr[4];               //  Stores the index (starting at 1) of the instruction that created the component value.  
        U32 copiedRegister[4];               //  Stores the register from which the value was copied.  
        U32 copiedComponent[4];              //  Stores the component from which the value was copied.  
        U32 copiedFromInstr[4];              //  Stores the index of the instruction that created the copied value.  
        vector<U32> copiedByInstr[4];        //  Stores a list of indices to instructions that copied the value.  
        vector<RegisterComponent> copies[4]; //  Stores a list with the copies of the value.  
    };

    /**
     *
     *  Defines the maximum number of temporal registers in the shader programming model.
     *
     */
    static const U32 MAX_TEMPORAL_REGISTERS = 32;

    /**
     *
     *  Defines the maximum number of input attributes in the shader programming model.
     */
    static const U32 MAX_INPUT_ATTRIBUTES = 48;

    /**
     *
     *  Support tables used to allocate names to registers.
     *  Used by the reduce live register usage optimization pass.
     */

    static U32 mappingTableComp0[];
    static U32 mappingTableComp1[];
    static U32 mappingTableComp2[];
    static U32 mappingTableComp3[];

    //
    //  Functions that provide information about the instruction types.
    //

    /**
     *
     *  Returns if the shader instruction opcode corresponds to a instruction that produces scalar
     *  results (broadcasted to all the result components).
     *
     *  @param opc The Shader Instruction opcode to evaluate.
     *
     *  @return If the shader instruction opcode corresponds to a instruction that produces scalar
     *  results (broadcasted to all the result components).
     *
     */

    static bool hasScalarResult(ShOpcode opc);

    /**
     *
     *  Returns if the shader instruction opcode corresponds to a instruction that is a vector
     *  operation (same operation performed for all the result components).
     *
     *  @param opc The Shader Instruction opcode to evaluate.
     *
     *  @return If the shader instruction opcode corresponds to a instruction that is a vector
     *  operation (same operation performed for all the result components).
     *
     */

    static bool isVectorOperation(ShOpcode opc);

    /**
     *
     *  Returns if the shader instruction opcode corresponds to a instruction that produces a SIMD4
     *  result (result components can not be swizzled/renamed).
     *
     *  @param opc The Shader Instruction opcode to evaluate.
     *
     *  @return If the shader instruction opcode corresponds to a instruction that produces a SIMD4
     *  result (result components can not be swizzled/renamed).
     *
     */

    static bool hasSIMD4Result(ShOpcode opc);

    /**
     *
     *  Returns if the shader instruction opcode corresponds to a instruction that produces no result.
     *
     *  @param opc The Shader Instruction opcode to evaluate.
     *
     *  @return If the shader instruction opcode corresponds to a instruction that produces no result.
     *
     */

    static bool hasNoResult(ShOpcode opc);

    /**
     *
     *  Returns if the shader instruction opcode corresponds to a instruction that writes the result
     *  to an address register.
     *
     *  @param opc The Shader Instruction opcode to evaluate.
     *
     *  @return If the shader instruction opcode corresponds to a instruction that writes the result
     *  to an address register.
     *
     */

    static bool hasAddrRegResult(ShOpcode opc);

    /**
     *
     *  Returns if the shader instruction opcode corresponds to a instruction that has not been implemented.
     *
     *  @param opc The Shader Instruction opcode to evaluate.
     *
     *  @return If the shader instruction opcode corresponds to a instruction that has not been implemented.
     *
     */

    static bool notImplemented(ShOpcode opc);


    /**
     *
     *  Returns if the presence of shader instruction in the shader program implies that the
     *  early z optimization must be disabled.
     *
     *  @param opc The shader instruction opcode to evaluate.
     *
     *  @return If the shader instruction implies that early z must be disabled.
     *
     */
     
    static bool mustDisableEarlyZ(ShOpcode opc);

    //
    //  Support functions for the optimization/transformation passes used to create or modify shader instructions.
    //

    /**
     *
     *  Creates a clone of the input shader instruction.
     *
     *  @param origInstr Pointer to the shader instruction to clone.
     *
     *  @return A pointer to shader instruction that is a clone of the input instruction.
     *
     */

     static cgoShaderInstr *copyInstruction(cgoShaderInstr *origInstr);

    /**
     *
     *  Patches the input operands of the input shader instruction.
     *  Used by the transformation pass that replaces input attributes with LDA instructions.
     *
     *  @param origInstr Pointer to the input shader instruction.
     *  @param patchRegOp1 Patched register identifier for the first operand of the instruction.
     *  @param patchBankOp1 Patched bank identifier for the first operand of the instruction.
     *  @param patchRegOp2 Patched register identifier for the second operand of the instruction.
     *  @param patchBankOp2 Patched bank identifier for the second operand of the instruction.
     *  @param patchRegOp3 Patched register identifier for the third operand of the instruction.
     *  @param patchBankOp3 Patched bank identifier for the third operand of the instruction.
     *
     *  @return A pointer to the patched shader instruction.
     *
     */

    static cgoShaderInstr *patchedOpsInstruction(cgoShaderInstr *origInstr,
                                             U32 patchRegOp1, Bank patchBankOp1,
                                             U32 patchRegOp2, Bank patchBankOp2,
                                             U32 patchRegOp3, Bank patchBankOp3);

    /**
     *
     *  Patches the result write mask of the input shader instruction.
     *  Used by the dead code elimination optimization pass.
     *
     *  @param origInstr Pointer to the input shader instruction.
     *  @param resultMask Patched result write mask.
     *
     *  @return A pointer to the patched shader instruction.
     *
     */

    static cgoShaderInstr *patchResMaskInstruction(cgoShaderInstr *origInstr, MaskMode resultMask);

    /**
     *
     *  Renames the result and operand registers used by the input shader instruction.
     *  Used by the register rename pass.
     *
     *  @param origInstr Pointer to the input shader instruction.
     *  @param resName New name (register identifier) for the result register.
     *  @param op1Name New name (register identifier) for the first operand register.
     *  @param op2Name New name (register identifier) for the second operand register.
     *  @param op3Name New name (register identifier) for the third operand register.
     *
     *  @return A pointer to the patched shader instruction.
     *
     */

    static cgoShaderInstr *renameRegsInstruction(cgoShaderInstr *origInstr, U32 resName, U32 op1Name, U32 op2Name, U32 op3Name);

    /**
     *
     *  Patches the result and operand registers used by the input shader instruction.
     *  Patches the result write mask and the operand component swizzle modes used by the input shader instruction.
     *  Used by the live register reduction pass.
     *  Used by the copy propagation pass.
     *
     *  @param origInstr Pointer to the input shader instruction.
     *  @param resReg Patched result register identifier.
     *  @param resMask Patched result write mask.
     *  @param op1Reg Patched first operand register identifier.
     *  @param op1Swz Patched first operand component swizzle mode.
     *  @param op2Reg Patched second operand register identifier.
     *  @param op2Swz Patched second operand component swizzle mode.
     *  @param op3Reg Patched third operand register identifier.
     *  @param op3Swz Patched third operand component swizzle mode.
     *
     *  @return A pointer to the patched shader instruction.
     *
     */

    static cgoShaderInstr *setRegsAndCompsInstruction(cgoShaderInstr *origInstr,
                                                         U32 resReg, MaskMode resMask,
                                                         U32 op1Reg, SwizzleMode op1Swz,
                                                         U32 op2Reg, SwizzleMode op2Swz,
                                                         U32 op3Reg, SwizzleMode op3Swz);


    /**
     *
     *
     *  Patches an input shader instruction to convert the shader instruction to Scalar (single component result) format.
     *  Used by the SIMD4 to Scalar transformation pass.
     *
     *  @param origInstr Pointer to the input shader instruction.
     *  @param op1Comp Patched single component to be read for the first operand.
     *  @param op2Comp Patched single component to be read for the second operand.
     *  @param op3Comp Patched single component to be read for the third operand.
     *  @param resComp Patched single component to write for the result.
     *
     *  @return A pointer to the patched shader instruction.
     *
     */

    static cgoShaderInstr *patchSOAInstruction(cgoShaderInstr *origInstr,
                                                  SwizzleMode op1Comp, 
                                                  SwizzleMode op2Comp,
                                                  SwizzleMode op3Comp,
                                                  MaskMode resComp);

    /**
     *
     *  Updates the table of temporal registers in use with the temporal register usage of the input shader instruction.
     *  Used by the transformation pass that replaces input attributes with LDA instructions.
     *  Used by the SIMD4 to Scalar transformation pass.
     *
     *  @param instr Pointer to a shader instruction.
     *  @param tempInUse Pointer to an array of boolean values defining if a temporal register is used by the program.
     *
     */

    static void updateTempRegsUsed(cgoShaderInstr *instr, bool *tempInUse);


    /**
     *
     *  Fills a table with information about what temporal registers are used by the shader program.
     *  Used by the transformation pass that replaces input attributes with LDA instructions.
     *
     *  @param inProgram The input shader program defined as a vector of pointers to shader instructions.
     *  @param tempInUse Pointer to an array of boolean values defining if a temporal register is used by the program.
     *
     */

    static void getTempRegsUsed(vector<cgoShaderInstr *> inProgram, bool *tempInUse);

    /**
     *
     *  Updates the instruction type count with the input instruction.
     *
     *  @param instr Pointer to a shader instruction.
     *  @param instrType Pointer to an array of counts storing the instruction types.
     *
     */

    static void updateInstructionType(cgoShaderInstr *instr, unsigned int *instrType);

    /**
     *
     *  Tests the swizzle mode for included components.
     *
     *  @param swizzle The input swizzle mode.
     *  @return if the swizzle mode includes z in any of the components.
     *
     */

    static void testSwizzle(SwizzleMode swizzle, bool& includesX, bool& includesY, bool& includesZ, bool& includesW);

    /**
     *
     *  Creates a list (stored in a vector) with the operand components accessed based on the component
     *  swizzle mask.
     *
     *  @param opSwizzle The input operand component swizzle mode.
     *  @param components Reference to a vector that will store the actual components accessed by the operand.
     *
     */

    static void extractOperandComponents(SwizzleMode opSwizzle, vector<SwizzleMode> &components);


    /**
     *
     *  Creates two lists (stored as vectors) of the components written by the result based on the
     *  result write mask.  The first list uses write mask identifiers and the second component swizzle
     *  identifier for the components.
     *
     *  @param resMask The input result write mask.
     *  @param componentsMask Reference to a vector that will store the written result components as result mask
     *  identifiers.
     *  @param componentsSwizzle Reference to a vector that will store the written result components
     *  component swizzle identifiers.
     *
     */

    static void extractResultComponents(MaskMode resMask, vector<MaskMode> &componentsMask, vector<SwizzleMode> &componentsSwizzle);


    /**
     *
     *  Creates the list (stored as vectors) of the actual operand components that are read by the shader instruction
     *  to produce the corresponding result component value.  It takes into account the operand component swizzle mode,
     *  the result write mask (uses decoded component list created by the extractResultComponents() function) and the
     *  instruction opcode to generate the lists.
     *  The lists store the components post swizzle that are actually read.  To get the identifiers of the components
     *  read from the operand registers the swizzle translation must be applied.
     *  As the only three operand instruction is CG1_ISA_OPCODE_MAD and the component usage is the same for all three operands
     *  (same for all normal SIMD instructions) the first or second operand lists can be used for the third operand.
     *
     *  @param instr Pointer to a shader instruction.
     *  @param resComponentsSwizzle A vector storing the components written by the instruction.
     *  @param readComponentsOp1 A reference to a vector that will store the actual first operand components accessed by the
     *  instruction to generate the corresponding result component.
     *  @param readComponentsOp2 A reference to a vector that will store the actual second operand components accessed by the
     *  instruction to generate the corresponding result component.
     *
     */

    static void getReadOperandComponents(cgoShaderInstr *instr, vector<SwizzleMode> resComponentsSwizzle,
                                         vector<U32> &readComponentsOp1, vector<U32> &readComponentsOp2);

    /**
     *
     *  Encodes the result write mask based on an array defining the result components that have to be written.
     *
     *  @param activeResComp Pointer to an array storing boolean values defining if a given component of the result has to be written.
     *
     *  @result The encoded result write mask.
     *
     */

    static MaskMode encodeWriteMask(bool *activeResComp);

    /**
     *
     *  Removes components for the input result write mask based on an array of boolean values defining the
     *  components to remove.
     *
     *  @param inResMask The input result write mask.
     *  @param removeResComp A pointer to an array of boolean values defining the components to remove from
     *  the write maks.
     *
     *  @return The result write mask with removed components.
     *
     */

    static MaskMode removeComponentsFromWriteMask(MaskMode inResMask, bool *removeResComp);

    /**
     *
     *  Encodes the component swizzle based on an array of component identifiers.
     *
     *  @param opSwizzle Pointer to an array of component identifiers (0 to 3).
     *
     *  @return The encoded component swizzle mode.
     *
     */

    static SwizzleMode encodeSwizzle(U32 *opSwizzle);


public:

    /**
     *
     *  Clones the input shader program.
     *
     *  @param inProgram The input shader program defined as a vector of pointers to shader instructions.
     *  @param outProgram Reference to the output shader program (defined as a vector of pointer to
     *  shader instructions) that will store the clone of the shader program.
     *
     */

    static void copyProgram(vector<cgoShaderInstr *> inProgram, vector<cgoShaderInstr *> &outProgram);


    /**
     *
     *  Concatenates the source shader code after the destination shader program.
     *
     *  @param srcProgram  The input shader code to concatenate.
     *  @param destProgram Reference to the result concatenated shader program.
     *
     */

    static void concatenateProgram(vector<cgoShaderInstr *> srcProgram, vector<cgoShaderInstr *>& destProgram);


    /**
     *
     *  Deletes the input shader program.
     *  Deletes the shader instructions pointed by the program and clears the vector.
     *
     *  @param inProgram The input shader program defined as a vector of pointers to shader instructions.
     *
     */

    static void deleteProgram(vector<cgoShaderInstr *> &inProgram);


    /**
     *
     *  Shader program transformation that converts input attributes into attribute loads using the LDA
     *  instruction.
     *
     *  @param inProgram The input shader program defined as a vector of pointers to shader instructions.
     *  @param outProgram Reference to the output shader program (defined as a vector of pointer to
     *  shader instructions) that will store the transformed program.
     */

    static void attribute2lda(vector<cgoShaderInstr *> inProgram, vector<cgoShaderInstr *> &outProgram);

    static float getALUTEXRatio(vector<cgoShaderInstr *> inProgram);

    /**
     *
     *  Shader program transformation that converts instructions from the SIMD4 format (SIMD4) to Scalar format
     *  (scalar).
     *
     *  @param inProgram The input shader program defined as a vector of pointers to shader instructions.
     *  @param outProgram Reference to the output shader program (defined as a vector of pointer to
     *  shader instructions) that will store the transformed program.
     */

    static void aos2soa(vector<cgoShaderInstr *> inProgram, vector<cgoShaderInstr *> &outProgram);


    /**
     *
     *  Shader program optimization that removes code that doesn't affects the result of the program.
     *  The implementation supports removing write components from the results of SIMD instructions.
     *  The function returns true if the
     *
     *  @param inProgram The input shader program defined as a vector of pointers to shader instructions.
     *  @param namesUsed The number of temporal registers name used by the program.
     *  @param outProgram Reference to the output shader program (defined as a vector of pointer to
     *  shader instructions) that will store the optimized program.
     *
     *  @return TRUE if code was removed.
     *
     */

    static bool deadCodeElimination(vector<cgoShaderInstr *> inProgram, U32 namesUsed, vector<cgoShaderInstr *> &outProgram);

    /**
     *
     *  Shader program transformation that renames every alue created by the shader program with an
     *  unique name.
     *  Used to reduce the complexity of other optimization passes.
     *  The function returns the number of names used.
     *
     *  @param inProgram The input shader program defined as a vector of pointers to shader instructions.
     *  @param outProgram Reference to the output shader program (defined as a vector of pointer to
     *  shader instructions) that will store the transformed program.
     *  @param AOStoSOA Boolean value that informs the rename function if the input program was transformed to
     *  Scalar instruction format.
     *
     *  @return The number of names used by the program.
     */

    static U32 renameRegisters(vector<cgoShaderInstr *> inProgram, vector<cgoShaderInstr *> &outProgram, bool AOStoSOA);

    /**
     *
     *  Shader program optimization that reduces the number of live register used at any point of the shader program.
     *  Implements allocation of names to temporal registers while trying to reduce the number of total registers used
     *  by reutilizing freed temporal registers and components.
     *  The function returns the maximum number of live temporal registers at any point of the program.
     *
     *  @param inProgram The input shader program defined as a vector of pointers to shader instructions.
     *  @param names Number of names used by the input program.
     *  @param outProgram Reference to the output shader program (defined as a vector of pointer to
     *  shader instructions) that will store the optimized program.
     *
     *  @return The maximum number of live temporal registers used at any point of the program.
     *
     */

    static U32 reduceLiveRegisters(vector<cgoShaderInstr *> inProgram, U32 names, vector<cgoShaderInstr *> &outProgram);

    /**
     *
     *  Shader program optimization that removes redundant CG1_ISA_OPCODE_MOV instructions (copies from the input operand to the same output
     *  result).
     *  The pass is used to remove MOVs introduced by the register rename pass that become redundant after register
     *  allocation by the reduce live register usage optimization pass.
     *
     *  @param inProgram The input shader program defined as a vector of pointers to shader instructions.
     *  @param outProgram Reference to the output shader program (defined as a vector of pointer to
     *  shader instructions) that will store the optimized program.
     *
     */
    static void removeRedundantMOVs(vector<cgoShaderInstr *> inProgram, vector<cgoShaderInstr *> &outProgram);

    /**
     *
     *  Shader program optimization that removes copies of values (CG1_ISA_OPCODE_MOV instructions) by propagating the original copy
     *  to other uses of the value.
     *  The pass is used after the register renaming pass to remove extra code generated by pass.
     *
     *  @param inProgram The input shader program defined as a vector of pointers to shader instructions.
     *  @param names Number of names used by the input program.
     *  @param outProgram Reference to the output shader program (defined as a vector of pointer to
     *  shader instructions) that will store the optimized program.
     *
     */

    //  No longer used or mantained.
    //static void copyPropagation(vector<cgoShaderInstr *> inProgram, U32 namesUsed, vector<cgoShaderInstr *> &outProgram);

    /**
     *
     *  Disassembles and sends to the standard output the instruction list of the input program.
     *
     *  @param inProgram The input shader program defined as a vector of pointers to shader instructions.
     *
     */

    static void printProgram(vector<cgoShaderInstr *> inProgram);

    /**
     *
     *  Runs a number of optimization passes over the input program.
     *  Passes (in order):
     *
     *      rename                      (if renaming is enabled)
     *      dead code elimination       (run multiple times)
     *      copy propagation
     *      dead code elimination       (run multiple times)
     *      reduce live registers       (if renaming is enabled)
     *      remove redundant moves
     *
     *  @param inProgram The input shader program defined as a vector of pointers to shader instructions.
     *  @param outProgram Reference to the output shader program (defined as a vector of pointer to
     *  shader instructions) that will store the optimized program.
     *  @param maxLiveTempRegs Reference to a variable where to store the maximum number of temporal registers
     *  alive at any point of the optimized shader program.
     *  @param noRename Boolean value that defines if the renaming and live register reduction passes are not run for the
     *  input program.
     *  @param AOStoSOA Boolean value that defines if the input program is in Scalar format.
     *  @param verbose Boolean value that defines if the debug output of the intermediate transformations of the program is
     *  enabled.
     *
     */

    static void optimize(vector<cgoShaderInstr *> inProgram, vector<cgoShaderInstr *> &outProgram, U32 &maxLiveTempRegs,
                         bool noRename, bool AOStoSOA, bool verbose = false);

    /**
     *
     *  Marks instructions in the shader program as wait points.
     *  A wait point is defined as a instruction that marks a shader thread change.  Wait points are set
     *  at the instruction before values loaded from memory (via the TEX, TXB, TXP or LDA instructions)
     *  are actually used by the program.  After a wait point all the values previously loaded from memory
     *  have to be available to the program.
     *
     *  @param inProgram The input shader program defined as a vector of pointers to shader instructions.
     *  @param outProgram Reference to the output shader program (defined as a vector of pointer to
     *  shader instructions) that will store the transformed program.
     *
     */

    static void assignWaitPoints(vector<cgoShaderInstr *> inProgram, vector<cgoShaderInstr *> &outProgram);

    /**
     *
     *  Decodes a shader program in binary format into a Shader Instruction program.
     *
     *  @param code Pointer to a buffer with the shader program in binary format.
     *  @param size Size of the program.
     *  @param outProgram Reference to the output program defined as a vector of pointer to shader instructions.
     *  @param numTemps Reference to a variable where to store the number of temporal registers found
     *  in the decoded program.
     *  @param hasJumps Reference to a boolean variable where to store if the decoded program has jump
     *  instructions.
     *
     */

    static void decodeProgram(U08 *code, U32 size, vector<cgoShaderInstr *> &outProgram, U32 &numTemps, bool &hasJumps);

    /**
     *
     *  Encodes the input shader program into binary format.
     *
     *  @param inProgram The input shader program defined as a vector of pointers to shader instructions.
     *  @param code Pointer to a buffer where to store the shader program in binary format.
     *  @param size Reference to a variable that stores the size of the buffer and that is assigned the size
     *  of the shader program in binary format.
     *
     */

    static void encodeProgram(vector<cgoShaderInstr *> inProgram, U08 *code, U32 &size);

    /**
     *
     *  Assembles a shader program written in CG1 Shader Assembler Language.
     *
     *  @param program Pointer to a byte array with the shader program written in CG1 Shader Assembler Language.
     *  @param code Pointer to a byte array where to store the assembled program code.
     *  @param size Reference to a variable that stores the size of the byte array used to store the assembled
     *              program code.  After the function is executed the variable will store the actual size of
     *              the encoded program.
     *
     *
     */
    
    static void assembleProgram(U08 *program, U08 *code, U32 &size);

    /**
     *
     *  Disassembles an encoded shader program to CG1 Shader Assembler Language.
     *
     *  @param code Pointer to a byte array with shader program code.
     *  @param codeSize Size of the encoded shader program in bytes.
     *  @param program Pointer to a byte array where to store the disassembled shader program in CG1 Shader Assembler Language.
     *  @param size Reference to a variable that stores the size of the byte array used to store the disassembled
     *              program.  After the function is executed the variable will store the actual size of
     *              the disassembled program.
     *  @param disableEarlyZ Reference to a boolean variable where to store if the shader program requires early z to
     *                       be disabled.
     *
     *
     */
    
    static void disassembleProgram(U08 *code, U32 codeSize, U08 *program, U32 &size, bool &disableEarlyZ);
    
}; // class ShaderOptimization

} // namespace arch

#endif
