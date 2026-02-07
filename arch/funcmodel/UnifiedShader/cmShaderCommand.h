/**************************************************************************
 *
 * Shader Command.
 *
 */

/**
 *
 *  @file cmShaderCommand.h
 *
 *  Defines ShaderCommand class.   This class stores commands
 *  sent to the Shader Unit from a Command Processor.
 *
 */

#include "GPUType.h"
#include "GPUReg.h"
#include "DynamicObject.h"

#ifndef _SHADERCOMMAND_

#define _SHADERCOMMAND_

namespace cg1gpu
{

//*  Commands that can recieve the Shader.  
enum Command
{
    RESET,              //  Reset the shader.  
    PARAM_WRITE,        //  Request to write the parameter (constant) bank.  
    LOAD_PROGRAM,       //  Request to load a new shader program.  
    SET_INIT_PC,        //  Request to set initial PC.  
    SET_THREAD_RES,     //  Request to set the per thread resource usage.  
    SET_IN_ATTR,        //  Request to configure shader input attributes.  
    SET_OUT_ATTR,       //  Request to configure shader output attributes.  
    SET_MULTISAMPLING,  //  Request to set the MSAA enabled flag. (used for microtriangle stamps).  
    SET_MSAA_SAMPLES,   //  Request to set the MSAA number of samples. (used for microtriangle stamps).  
    TX_REG_WRITE        //  Texture unit register write.  
};

/**
 *
 *  Defines a Shader Command from the Command Processor to the Shader.
 *
 *  This class stores information about commands sent to a Shader Unit
 *  from a Command Processor.
 *  The class inherits from DynamicObject class that offers dynamic
 *  memory management and trace support.
 *
 */

class ShaderCommand : public DynamicObject
{

private:

    Command command;    //  Type of the command.  
    U08 *code;        //  For NEW_PROGRAM command, program code.  
    U32 sizeCode;    //  For NEW_PROGRAM command, size of program code.  
    Vec4FP32 *values;  //  For PARAM_WRITE array of Vec4FP32s.  
    U32 firstAddr;   //  For PARAM_WRITE address of the first register/position to write.  
    U32 numValues;   //  For PARAM_WRITE number of values in the Vec4FP32 array.  
    U32 iniPC;       //  For SET_INIT_PC, new initial PC for the shader program.  
    U32 thResources; //  For SET_THREAD_RES, per thread resource usage.  
    U32 attribute;   //  For SET_IN_ATTR, SET_OUT_ATTR, which shader input/output is going to be configured.  
    bool attrActive;    //  For SET_IN_ATTR, SET_OUT_ATTR, if the shader input/output is active for the current program.  
    bool multiSampling; //  For SET_MULTISAMPLING, if multisampling is enabled. (used for microtriangle stamps) 
    U32 msaaSamples; //  For SET_MSAA_SAMPLES, the number of MSAA samples. (used for microtriangle stamps) 
    GPURegister txReg;  //  For TX_REG_WRITE, the texture unit register to write.  
    U32 txSubReg;    //  For TX_REG_WRITE, the registe subregister to write.  
    GPURegData txData;  //  For TX_REG_WRITE, the texture unit register data to write.  
    ShaderTarget target;    //  For SET_INIT_PC and SET_THREAD_RES the shader target for which the command is issued.  
    U32 loadPC;          //  Direction in the shader instruction memory where to load the shader program.  
    
public:

    /**
     *
     *  ShaderCommand constructor.
     *
     *  Creates a ShaderCommand without any parameters.
     *
     *  @param comm  Command type for the ShaderCommand.
     *  @return A ShaderCommand object.
     *
     */

    ShaderCommand(Command comm);

    /**
     *
     *  ShaderCommand constructor.
     *
     *  Creates a ShaderCommand of type NEW_PROGRAM.
     *
        @param pc Direction in the shader instruction memory where to load the shader program.
     *  @param progCode Pointer to a shader program in binary format.
     *  @param size Size of the shader program (bytes).
     *  @return A ShaderCommand object of type NEW_PROGRAM.
     *
     */

    ShaderCommand(U32 pc, U08 *progCode, U32 size);

    /**
     *
     *  ShaderCommand constructor.
     *
     *  Creates a ShaderCommand of type PARAM_WRITE.
     *
     *  @param input Pointer to an array of Vec4FP32s that stores the
     *  values to write in the parameter bank/memory.
     *  @param firstAddr Address/position of the first element
     *  to write in the parameter bank/memory.
     *  @param inputSize Number of elements in the array.
     *  @return A ShaderCommand object of type PARAM_WRITE.
     *
     */

    ShaderCommand(Vec4FP32 *input, U32 firstAddr, U32 inputSize);

    /**
     *
     *  Shader Command constructor.
     *
     *  Creates a ShaderCommand for commands with a single unsigned 32 bit parameter.
     *
     *  @param com Shader command (SET_INI_PC, SET_THREAD_RES).
     *  @param target Shader target for which the command is applied.
     *  @param data Parameter of the shader command
     *
     *  @return A ShaderCommand object.
     *
     */

    ShaderCommand(Command com, ShaderTarget target, U32 data);

    /**
     *
     *  Shader Command constructor.
     *
     *  Creates a ShaderCommand of type SET_IN_ATTR or SET_OUT_ATTR.
     *
     *  @param com Shader command (SET_IN_ATTR or SET_OUT_ATTR).
     *  @param attrib The shader output attribute to configure.
     *  @param active If the shader output attribute is going to be written.
     *
     *  @return A ShaderCommand object of type SET_IN_ATTR or SET_OUT_ATTR.
     *
     */

    ShaderCommand(Command com, U32 attr, bool active);

    /**
     *
     *  Shader Command constructor.
     *
     *  Creates a ShaderCommand of type TX_REG_WRITE.
     *
     *  @param reg Register of the Texture Unit to write to.
     *  @param subReg Subregister of the register to write to.
     *  @param data Data to write in the Texture Unit register.
     *
     *  @return A ShaderCommand object of type TX_REG_WRITE.
     *
     */

    ShaderCommand(GPURegister reg, U32 subReg, GPURegData data);

    /**
     *
     *  Shader Command constructor.
     *
     *  Creates a ShaderCommand of the type SET_MULTISAMPLING. (used for microtriangle stamps).
     *
     *  @param enabled Multisampling is enabled.
     *
     *  @return A ShaderCommand object of type SET_MULTISAMPLING.
     *
     */

    ShaderCommand(bool enabled);

    /**
     *
     *  Shader Command constructor.
     *
     *  Creates a ShaderCommand of the type SET_MSAA_SAMPLES. (used for microtriangle stamps).
     *
     *  @param samples The number of MSAA samples.
     *
     *  @return A ShaderCommand object of type SET_MSAA_SAMPLES.
     *
     */

    ShaderCommand(U32 samples);


    /**
     *
     *  ShaderComamnd destructor.
     *
     */
     
    ~ShaderCommand();
    
    /**
     *
     *  Returns the type of the command.
     *
     *  @return  Type of the command.
     *
     */

    Command getCommandType() const;

    /**
     *
     *  Returns the pointer to the shader program code.
     *
     *  @return A pointer to the shader program code.
     *
     */

    U08 *getProgramCode();

    /**
     *
     *  Returns the size of the shader program code in bytes.
     *
     *  @return Shader program code size.
     *
     */

    U32 getProgramCodeSize() const;

    /**
     *
     *  Returns a pointer to an array of Vec4FP32 values.
     *
     *  @return A pointer to an array of Vec4FP32s.
     *
     */

    Vec4FP32 *getValues();

    /**
     *
     *  Returns the number of elements in the Vec4FP32 array.
     *
     *  @return Number of values in the Vec4FP32 array.
     *
     */

    U32 getNumValues() const;

    /**
     *
     *  Returns the address of the first element to write
     *  in parameter memory.
     *
     *  @return Address of the first element to write.
     *
     */

    U32 getParamFirstAddress() const;

    /**
     *
     *  Returns the new initial PC for the shader program.
     *
     *  @return Initial shader program PC.
     *
     */

    U32 getInitPC() const;

    /**
     *
     *  Returns the per thread resource usage to set.
     *
     *  @return Per shader thread resource usage.
     *
     */

    U32 getThreadResources() const;

    /**
     *
     *  Returns the address in the shader instruction memory where to load the shader program.
     *
     *  @return Address in the shader instruction memory where to load the shader program.
     *
     */
    U32 getLoadPC() const;
    
    /**
     *
     *  Returns the shader target for the command.
     *
     *  @return The shader target for which the command was issued.
     *
     */
     
     ShaderTarget getShaderTarget() const;
    
    /**
     *
     *  Returns the shader input/output attribute to be configured.
     *
     *  @return The shader input/output to configure.
     *
     */

    U32 getAttribute() const;

    /**
     *
     *  Returns if the shader input/output is active in the current shader program.
     *
     *  @return If the shader input/output is active
     *
     */

    bool isAttributeActive() const;

    /**
     *
     *  Returns the texture unit register to write to.
     *
     *  @return The register identifier to write to.
     *
     */

    GPURegister getRegister() const;

    /**
     *
     *  Returns the texture unit register subregister to write.
     *
     *  @return The subregister to write to.
     *
     */

    U32 getSubRegister() const;

    /**
     *
     *  Returns the data to write in the texture unit register.
     *
     *  @return Data to write into the texture unit register.
     *
     */

    GPURegData getRegisterData() const;

    /**
     *
     *  Returns if multisampling enabled/disabled for SET_MULTISAMPLING commands
     *
     *  @return true if multisampling enabled.
     *
     */

    bool multisamplingEnabled() const;

    /**
     *
     *  Returns the number of MSAA sampels for SET_MSAA_SAMPLES commands
     *
     *  @return true if multisampling enabled.
     *
     */

    U32 samplesMSAA() const;

    /**
     *
     *  Stores the pointer a shader program code and its size.
     *
     *  @param code  Pointer to a shader program code in binary format.
     *  @param size  Size of the shader program code in bytes.
     *
     */

    void setProgramCode(U08 *code, U32 size);

    /**
     *
     *  Stores a pointer to an array of Vec4FP32 values and
     *  the size of the array.
     *
     *  @param values  Pointer to an array of Vec4FP32s.
     *  @param size  Size of the array of Vec4FP32s.
     *
     */

     void setValues(Vec4FP32 *values, U32 size);

    /**
     *
     *  Sets the address for the first element to write in the
     *  parameter memory.
     *
     *  @param firstAddress  Address of the first element to write
     *  in the parameter memory.
     *
     */

     void setParamFirstAddress(U32 firstAddress);

};

} // namespace cg1gpu

#endif
