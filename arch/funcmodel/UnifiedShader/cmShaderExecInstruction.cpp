/**************************************************************************
 *
 * Shader Execution Instruction.
 *
 */


#include "cmShaderExecInstruction.h"
#include <cstring>
#include <stdio.h>

/**
 *
 *  @file ShaderExecInstruction.cpp
 *
 *  Implements the ShaderExecInstruction class members and
 *  functions.  This class describes the instance of an
 *  instruction while it is being executed through the
 *  shader pipeline.
 *
 */

using namespace cg1gpu;


//  Constructor to be used with original Shader
ShaderExecInstruction::ShaderExecInstruction(cgoShaderInstr::cgoShaderInstrEncoding &shInstrDec, U32 pcInstr,
    U64 startCycle, bool repeat, bool fakedFetch) :

    instr(shInstrDec), pc(pcInstr), cycle(startCycle), repeated(repeat), fake(fakedFetch), trace(false)
{
    cgoShaderInstr *shInstr = instr.getShaderInstruction();

    state = FETCH;

    //  Set color for tracing.
    setColor(shInstr->getOpcode());

    //  Set instruction disassemble as info.
    if (!fake)
        shInstr->disassemble((char *) getInfo(), MAX_INFO_SIZE);
    else
        strcpy((char *) getInfo(), "FAKE INSTRUCTION");

    setTag("ShExIns");
}

//  Constructor to be used by VectorShader
ShaderExecInstruction::ShaderExecInstruction(cgoShaderInstr::cgoShaderInstrEncoding &shInstrDec, U32 thread, U32 elem, 
    U32 pcInstr, U64 startCycle, bool fakedFetch) :
    
    instr(shInstrDec), threadID(thread), element(elem), pc(pcInstr), cycle(startCycle), repeated(false), fake(fakedFetch), trace(false)
{
    cgoShaderInstr *shInstr = instr.getShaderInstruction();

    state = FETCH;

    //  Set color for tracing.
    setColor(shInstr->getOpcode());

    //  Set instruction disassemble as info.
    char disasmInstr[255];

    if (!fake)
    {
        shInstr->disassemble(disasmInstr, MAX_INFO_SIZE);
        sprintf((char *) getInfo(), "ThID %03d Elem %03d PC h%04x -> %s", threadID, element, pc, disasmInstr);
    }
    else
        sprintf((char *) getInfo(), "ThID %03d Elem %03d PC h%04x -> FAKE INSTRUCTION", threadID, element, pc);

    
    setTag("ShExIns");

}

//  Return the Shader Instruction of the dynamic instruction.  
cgoShaderInstr::cgoShaderInstrEncoding &ShaderExecInstruction::getShaderInstruction()
{
    return instr;
}

//  Returns the dynamic shader instruction start cycle (creation/fetch).  
U64 ShaderExecInstruction::getStartCycle()
{
    return cycle;
}

//  Returns the dynamic shader instruction PC.  
U32 ShaderExecInstruction::getPC()
{
    return pc;
}

//  Returns if the instruction is being replayed.  
bool ShaderExecInstruction::getRepeated()
{
    return repeated;
}

//  Returns the dynamic shader instruction state.  
ExecInstrState ShaderExecInstruction::getState()
{
    return state;
}

//  Returns a reference to the result from the instruction.  
Vec4FP32 &ShaderExecInstruction::getResult()
{
    return result;
}

//  Changes the state of the instruction.  
void ShaderExecInstruction::changeState(ExecInstrState newState)
{
    state = newState;
}

//  Sets the instruction result.  
void ShaderExecInstruction::setResult(const Vec4FP32 &res)
{
    result = res;
}

//  Gets if the instruction is faked.  
bool ShaderExecInstruction::getFakeFetch() const
{
    return fake;
}

//  Gets if the instruction execution must be traced.
bool ShaderExecInstruction::getTraceExecution()
{
    return trace;
}

//  Enable tracing the execution of the instruction.
void ShaderExecInstruction::enableTraceExecution()
{
    trace = true;
}
