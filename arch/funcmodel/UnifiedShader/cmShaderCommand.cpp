/**************************************************************************
 *
 * Shader Command.
 *
 */


/**
 *
 *  @file ShaderCommand.cpp
 *
 *  This file store the implementation for the ShaderCommand class.
 *  The ShaderCommand class stores information about commands sent
 *  to a Shader Unit from a Command Processor.
 *
 */

#include "cmShaderCommand.h"
#include <cstring>

using namespace arch;

//  Creates the ShaderCommand and sets the command type.  
ShaderCommand::ShaderCommand(Command com)
{
    command = com;     //  Set the command type.  

    //  Default (empty) values for the command parameters.
    code = NULL;
    sizeCode = 0;
    values = NULL;
    numValues = 0;
    firstAddr = 0;
    iniPC = 0;

    //  Set color for tracing.  
    setColor(command);

    setTag("ShCom");
}

//  Craete an initialiced ShaderCommand of type NEW_PROGRAM.  
ShaderCommand::ShaderCommand(U32 pc, U08 *progCode, U32 size)
{
    command = LOAD_PROGRAM;
    code = new U08[size];
    memcpy(code, progCode, size);
    sizeCode = size;
    loadPC = pc;

    //  Default (empty) values for the unused parameters.  
    values = NULL;
    numValues = 0;
    firstAddr = 0;

    //  Set color for tracing.  
    setColor(command);

    setTag("ShCom");
}

//  Creates an initialized ShaderCommand of  type PARAM_WRITE.  
ShaderCommand::ShaderCommand(Vec4FP32 *buffer, U32 first, U32 size)
{
    command = PARAM_WRITE;
    values = buffer;
    numValues = size;
    firstAddr = first;

    //  Default (empty) values for the unused parameters.  
    code = NULL;
    sizeCode = 0;
    iniPC = 0;

    //  Set color for tracing.  
    setColor(command);

    setTag("ShCom");
}

//  Creates an initialized ShaderCommand of  type SET_INIT_PC.  
ShaderCommand::ShaderCommand(Command com, ShaderTarget shTarget, U32 data)
{
    //  Default (empty) values for the unused parameters.  
    code = NULL;
    sizeCode = 0;
    values = NULL;
    numValues = 0;
    firstAddr = 0;
    iniPC = 0;
    thResources = 0;

    //  Select command.  
    switch (com)
    {
        case SET_INIT_PC:

            //  Configure command for setting the shader program initial PC.  
            command = SET_INIT_PC;
            iniPC = data;
            target = shTarget;

            break;

        case SET_THREAD_RES:

            //  Configure command for setting the per thread resource usage.  
            command = SET_THREAD_RES;
            thResources = data;
            target = shTarget;

            break;

        default:
            CG_ASSERT("Unsupported command for single parameter 32 bit unsigned constructor.");
            break;
    }

    //  Set color for tracing.  
    setColor(command);

    setTag("ShCom");
}

//  Creates an initialized ShaderCommand of type SET_IN_ATTR or SET_OUT_ATTR.  
ShaderCommand::ShaderCommand(Command com, U32 attr, bool active)
{
    CG_ASSERT_COND(!((com != SET_IN_ATTR) && (com != SET_OUT_ATTR)), "Unsupported command for configuring input/output shader attributes constructor.");
    //  Default (empty) values for the unused parameters.  
    code = NULL;
    sizeCode = 0;
    values = NULL;
    numValues = 0;
    firstAddr = 0;
    iniPC = 0;
    thResources = 0;
    
    //  Set command.  
    command = com;

    //  Shader input/output attribute to configure.  
    attribute = attr;

    //  Set if the input/output attribute is enabled.  
    attrActive = active;

    //  Set color for signal tracing.  
    setColor(command);

    setTag("ShCom");
}

//  Creates an initialized ShaderCommand of type TX_REG_WRITE.  
ShaderCommand::ShaderCommand(GPURegister reg, U32 subReg, GPURegData data)
{

    //  Default (empty) values for the unused parameters.  
    code = NULL;
    sizeCode = 0;
    values = NULL;
    numValues = 0;
    firstAddr = 0;
    iniPC = 0;
    thResources = 0;

    //  Command for texture unit register write.  
    command = TX_REG_WRITE;

    //  Set register and data to write.  
    txReg = reg;
    txSubReg = subReg;
    txData = data;

    //  Set color for signal tracing.  
    setColor(command);

    setTag("ShCom");
}

//  Creates an initialized ShaderCommand of the type SET_MULTISAMPLING.  
ShaderCommand::ShaderCommand(bool enabled)
{
    //  Default (empty) values for the unused parameters.  
    code = NULL;
    sizeCode = 0;
    values = NULL;
    numValues = 0;
    firstAddr = 0;
    iniPC = 0;
    thResources = 0;
    txSubReg = 0;

    //  Command for multisampling register write.  
    command = SET_MULTISAMPLING;

    //  Set register and data to write.  
    multiSampling = enabled;

    //  Set color for signal tracing.  
    setColor(command);

    setTag("ShCom");
}

//  Creates an initialized ShaderCommand of the type SET_MSAA_SAMPLES.  
ShaderCommand::ShaderCommand(U32 samples)
{
    //  Default (empty) values for the unused parameters.  
    code = NULL;
    sizeCode = 0;
    values = NULL;
    numValues = 0;
    firstAddr = 0;
    iniPC = 0;
    thResources = 0;
    txSubReg = 0;

    //  Command for multisampling register write.  
    command = SET_MSAA_SAMPLES;

    //  Set register and data to write.  
    msaaSamples = samples;

    //  Set color for signal tracing.  
    setColor(command);

    setTag("ShCom");
}

//  Destructor.
ShaderCommand::~ShaderCommand()
{
    //  Check if this Shader Command allocated memory for the shader code (LOAD_PROGRAM command).
    if (code != NULL)
        delete [] code;
}


//  Returns the type of the command.  
Command ShaderCommand::getCommandType() const
{
    return command;
}

//  Returns a pointer to the shader program code.  
U08 *ShaderCommand::getProgramCode()
{

    //  Check correct command type.  
    CG_ASSERT_COND(!(command != LOAD_PROGRAM), "Unsuported command parameters for this command type.");
    return code;
}

//  Returns the size of the shader program code in bytes.  
U32 ShaderCommand::getProgramCodeSize() const
{
    return sizeCode;
}

//  Returns the reference to the array of Vec4FP32s.  
Vec4FP32 *ShaderCommand::getValues()
{
    //  Check correct command type.  
    CG_ASSERT_COND(!(command != PARAM_WRITE), "Unsupported command parameter for this command type.");
    return values;
}

//  Returns the size of the array of Vec4FP32s.  
U32 ShaderCommand::getNumValues() const
{
    //  Check correct command type.  
    CG_ASSERT_COND(!(command != PARAM_WRITE), "Unsupported command parameter for this command type.");
    return numValues;
}

//  Returns the first element to write in parameter memory.  
U32 ShaderCommand::getParamFirstAddress() const
{
    //  Check correct command type.  
    CG_ASSERT_COND(!(command != PARAM_WRITE), "Unsupported command parameter for this command type.");
    return firstAddr;
}

//  Return the new shader program initial PC.  
U32 ShaderCommand::getInitPC() const
{
    //  Check correct command type.  
    CG_ASSERT_COND(!(command != SET_INIT_PC), "Unsupported command parameter for this command type.");
    return iniPC;
}

//  Return the per thread resource usage.  
U32 ShaderCommand::getThreadResources() const
{
    //  Check correct command type.  
    CG_ASSERT_COND(!(command != SET_THREAD_RES), "Unsupported command parameter for this command type.");
    return thResources;
}

//  Return the address in shader instruction memory where to load the shader program.
U32 ShaderCommand::getLoadPC() const
{
    CG_ASSERT_COND(!(command != LOAD_PROGRAM), "Unsupported command parameter for this command type.");    
    return loadPC;
}

//  Return the shader target for which the shader command was issued.
ShaderTarget ShaderCommand::getShaderTarget() const
{

    CG_ASSERT_COND(!((command != SET_INIT_PC) && (command != SET_THREAD_RES)), "Unsupported command parameter for this command type.");
    return target;
}


//  Return the shader input/output attribute to configure.  
U32 ShaderCommand::getAttribute() const
{
    //  Check correct command type.  
    CG_ASSERT_COND(!((command != SET_OUT_ATTR) && (command != SET_IN_ATTR)), "Unsupported command parameter for this command type.");
    return attribute;
}

//  Return if the shader input/output is active.  
bool ShaderCommand::isAttributeActive() const
{
    //  Check correct command type.  
    CG_ASSERT_COND(!((command != SET_OUT_ATTR) && (command != SET_IN_ATTR)), "Unsupported command parameter for this command type.");
    return attrActive;
}

//  Returns the texture unit register to write.  
GPURegister ShaderCommand::getRegister() const
{
    CG_ASSERT_COND(!(command != TX_REG_WRITE), "Unsupported command parameter for this command type.");
    return txReg;
}

//  Returns the texture unit register data to write.  
GPURegData ShaderCommand::getRegisterData() const
{
    CG_ASSERT_COND(!(command != TX_REG_WRITE), "Unsupported command parameter for this command type.");
    return txData;
}

//  Returns the texture unit subregister to write.  
U32 ShaderCommand::getSubRegister() const
{
    CG_ASSERT_COND(!(command != TX_REG_WRITE), "Unsupported command parameter for this command type.");
    return txSubReg;
}

//  Returns if multisampling enabled/disabled for SET_MULTISAMPLING commands.  
bool ShaderCommand::multisamplingEnabled() const
{
    return multiSampling;
}

//  Returns the number of MSAA sampels for SET_MSAA_SAMPLES commands.  
U32 ShaderCommand::samplesMSAA() const
{
    return msaaSamples;
}

//  Sets the pointer to a shader program code and the code aize.  
void ShaderCommand::setProgramCode(U08 *progCode, U32 size)
{
    //  Check correct command type.  
    CG_ASSERT_COND(!(command != LOAD_PROGRAM), "Unsuported command parameters for this command type.");
    code = progCode;
    sizeCode = size;
}

//  Sets pointer to the array of values and its size.  
void ShaderCommand::setValues(Vec4FP32 *valueArray, U32 size)
{
    //  Check correct command type.  
    CG_ASSERT_COND(!(command != PARAM_WRITE), "Unsupported command parameters for this command.");
    values = valueArray;
    numValues = size;
}

//  Sets the address for the first element to write in parameter memory.  
void ShaderCommand::setParamFirstAddress(U32 first)
{
    //  Check correct command type.  
    CG_ASSERT_COND(!(command != PARAM_WRITE), "Unsupported command parameters for this command.");
    firstAddr = first;
}

