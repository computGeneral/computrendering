/**************************************************************************
 *
 * Shader Execution Instruction.
 *
 */

/**
 *
 *  @file cmShaderExecInstruction.h
 *
 *  Defines the ShaderExecInstruction class members and
 *  functions.  This class describes the instance of an
 *  instruction while it is being executed through the
 *  shader pipeline.
 *
 */

#include "GPUType.h"
#include "DynamicObject.h"
#include "ShaderInstr.h"


#ifndef _SHADEREXECINSTRUCTION_

#define _SHADEREXECINSTRUCTION_

namespace arch
{

enum ExecInstrState { FETCH, WAIT, RUN };

/**
 *
 *  Defines a dynamic (execution time) instruction.
 *
 *  This class defines the dynamic instance of a shader instruction
 *  while it is being executed through the shader pipeline.  It
 *  carries information about the instruction that is executed and
 *  its state in the shader pipeline.
 *  This class inherits from the DynamicObject class which offers
 *  dynamic memory management and tracing support.
 *
 *
 */


class ShaderExecInstruction : public DynamicObject
{

private:

    cgoShaderInstr::cgoShaderInstrEncoding &instr; // Pointer to the decoded shader instruction carried by the object.  
    U32 pc;              //  PC associated with this dynamic instruction.  
    U64 cycle;           //  Cycle in which this dynamic instruction was created (started?).  
    U32 threadID;        //  Shader thread that fetched this the shader instruction.  
    U32 element;         //  Vector thread element for which this shader instruction was fetched.  
    Vec4FP32 result;       //  Result from the instruction.    
    ExecInstrState state;   //  State of the dynamic instruction.  
    bool repeated;          //  The instruction is a repeated instruction.  
    bool fake;              //  The instruction is a fake fetch.  
    bool trace;             //  Flag that stores if the execution of this instruction must be traced.  

public:

    /**
     *
     *  Class constructor.
     *
     *  This function creates a dynamic instruction that instancies cgoShaderInstrEncoding shInstr which
     *  is at PC pcInstr in the cycle startCycle.
     *
     *  @param shInstr The cgoShaderInstrEncoding from which this dynamic instruction is instance.
     *  @param pcInstr The PC when/where this instruction was fetched/created.
     *  @param startCycle  The cycle in which this dynamic instruction was created.
     *  @param repeated The instruction is being replayed.
     *  @param fakedFetch The dynamic instruction is a fake fetch and doesn't carries a real
     *  instruction to decode and execute.
     *
     *  @return A new ShaderExecInstruction object.
     *
     */

    ShaderExecInstruction(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, U32 pcInstr,
        U64 startCycle, bool repeated, bool fakedFetch);

    /**
     *
     *  Class constructor.
     *
     *  This function creates a dynamic instruction that instancies cgoShaderInstrEncoding shInstr which
     *  is at PC pcInstr in the cycle startCycle.
     *
     *  @param shInstr The cgoShaderInstrEncoding from which this dynamic instruction is instance.
     *  @param thread Identifier of the shader thread for which the shader instruction was fetched.
     *  @param element Index of the vector thread element for which the shader instruction was fetched.
     *  @param pcInstr The PC when/where this instruction was fetched/created.
     *  @param startCycle The cycle in which this dynamic instruction was created.
     *  @param fakedFetch The dynamic instruction is a fake fetch and doesn't carries a real
     *  instruction to decode and execute.
     *
     *  @return A new ShaderExecInstruction object.
     *
     */

    ShaderExecInstruction(cgoShaderInstr::cgoShaderInstrEncoding &shInstr, U32 threadID, U32 element, 
        U32 pcInstr, U64 startCycle, bool fakedFetch);

    /**
     *
     *  Returns a reference to the Shader Instruction Decoded.
     *
     *  @return A reference to the Shader Instruction Decoded from which this dynamic
     *  instruction is an instance.
     *
     */

    cgoShaderInstr::cgoShaderInstrEncoding &getShaderInstruction();

    /**
     *
     *  Gets the dynamic instruction start cycle.
     *
     *  @return The cycle in which this instruction was created.
     *
     */

    U64 getStartCycle();

    /**
     *
     *  Returns the PC from where this instruction was fetched.
     *
     *  @return The PC from where this instruction was fetched.
     *
     */

    U32 getPC();

    /**
     *
     *  Returns the instruction current state.
     *
     *
     *  @return The state of the instruction.
     */

    ExecInstrState getState();

    /**
     *
     *  Returns the result produced by the instruction.
     *
     *  @return The result produced by the instruction.
     *
     */

    Vec4FP32& getResult();

    /**
     *
     *  Changes the state of the instruction.
     *
     *  @param state New state of the instruction.
     *
     */

    void changeState(ExecInstrState state);

    /**
     *
     *  Sets the result value produced by the instruction.
     *
     *  @param res The result value for the instruction.
     *
     */

    void setResult(const Vec4FP32& res);

    /**
     *
     *  Get repeated instruction flag.
     *
     *  @return Returns if the instruction is being repeated.
     *
     */

    bool getRepeated();

    /**
     *
     *  Gets if the instruction is a fake fetch.
     *
     *  @return If the instruction is a fake fetch.
     *
     */

    bool getFakeFetch() const;

    /**
     *
     *  Enables tracing the instruction execution.
     *
     */
     
    void enableTraceExecution();
    
    /**
     *
     *  Gets if the instruction execution must be traced.
     *
     *  @return If the instruction execution must be traced.
     *
     */
     
    bool getTraceExecution();


};

} // namespace arch

#endif
