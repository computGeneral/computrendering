/**************************************************************************
 *
 * Shader Input definition file.
 *
 */

#include "cmShaderInput.h"

using namespace arch;

//  Creates a new ShaderInput.  
ShaderInput::ShaderInput(U32 ID, U32 unitID, U32 stEntry, Vec4FP32 *attrib)
{
    //  Set shader input parameters.  
    id = ID;
    unit = unitID;
    entry = stEntry;
    attributes = attrib;
    kill = false;
    isAFragment = false;
    last = false;

    setTag("ShInp");
}

//  Creates a new ShaderInput.  
ShaderInput::ShaderInput(U32 ID, U32 unitID, U32 stEntry, Vec4FP32 *attrib, ShaderInputMode shInMode)
{
    //  Set shader input parameters.  
    id = ID;
    unit = unitID;
    entry = stEntry;
    attributes = attrib;
    kill = false;
    isAFragment = (shInMode == SHIM_FRAGMENT || shInMode == SHIM_MICROTRIANGLE_FRAGMENT);
    mode = shInMode;
    last = false;

    setTag("ShInp");
}

//  Creates a new ShaderInput.  
ShaderInput::ShaderInput(ShaderInputID ID, U32 unitID, U32 stEntry, Vec4FP32 *attrib)
{
    //  Set shader input parameters.  
    id = ID;
    unit = unitID;
    entry = stEntry;
    attributes = attrib;
    kill = false;
    isAFragment = false;
    last = false;

    setTag("ShInp");
}

//  Creates a new ShaderInput.  
ShaderInput::ShaderInput(ShaderInputID ID, U32 unitID, U32 stEntry, Vec4FP32 *attrib, ShaderInputMode shInMode)
{
    //  Set shader input parameters.  
    id = ID;
    unit = unitID;
    entry = stEntry;
    attributes = attrib;
    kill = false;
    isAFragment = (shInMode == SHIM_FRAGMENT || shInMode == SHIM_MICROTRIANGLE_FRAGMENT);
    mode = shInMode;
    last = false;

    setTag("ShInp");
}

//  Gets the shader input identifier.  
U32 ShaderInput::getID()
{
    return id.id.inputID;
}

//  Gets the shader input identifier.  
ShaderInputID ShaderInput::getShaderInputID()
{
    return id;
}

//  Gets the shader input post shading storage entry.  
U32 ShaderInput::getEntry()
{
    return entry;
}

//  Gets the shader input post shading storage unit.  
U32 ShaderInput::getUnit()
{
    return unit;
}

//  Gets the shader input attributes.  
Vec4FP32 *ShaderInput::getAttributes()
{
    return attributes;
}

//  Sets the shader input kill flag.  
void ShaderInput::setKilled()
{
    kill = true;
}

//  Returns is the shader input was killed/aborted.  
bool ShaderInput::getKill()
{
    return kill;
}

//  Set shader input as being a fragment.  
void ShaderInput::setAsFragment()
{
    isAFragment = true;
    mode = SHIM_FRAGMENT;
}

//  Set shader input as being a vertex.  
void ShaderInput::setAsVertex()
{
    isAFragment = false;
    mode = SHIM_VERTEX;
}

//  Returns if the shader input is for a fragment.  
bool ShaderInput::isFragment()
{
    return isAFragment;
}

//  Returns if the shader input is for a vertex.  
bool ShaderInput::isVertex()
{
    return !isAFragment;
}

//  Set last index in batch flag.  
void ShaderInput::setLast(bool lastInBatch)
{
    last = lastInBatch;
}

//  Return if the index is the last in the batch.  
bool ShaderInput::isLast()
{
    return last;
}

//  Return the shader input mode.  
ShaderInputMode ShaderInput::getInputMode()
{
    return mode;
}

//  Sets the start cycle in the shader.  
void ShaderInput::setStartCycleShader(U64 cycle)
{
    startCycleShader = cycle;
}

//  Returns the start cycle in the shader unit.  
U64 ShaderInput::getStartCycleShader() const
{
    return startCycleShader;
}

//  Sets the number of cycles spent in the shader.  
void ShaderInput::setShaderLatency(U32 latency)
{
    shaderLatency = latency;
}

//  Returns the number of cycles spent inside the shader unit .  
U32 ShaderInput::getShaderLatency() const
{
    return shaderLatency;
}
