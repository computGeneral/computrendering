/**************************************************************************
 *
 * Shader State.
 *
 */

/**
 *
 *  @file cmShaderState.h
 *
 *  Defines enumeration types for the shader external and internal states.
 *
 */

#ifndef _SHADERSTATE_

#define _SHADERSTATE_

namespace arch
{

/**
 *
 *  Shader States.
 *
 */


enum ShaderState
{
    SH_RESET,   //  The shader is in reset state.  
    SH_READY,   //  The Shader accepts new input data.  
    SH_EMPTY,   //  The Shader has all its thread and input buffers empty.  Accepts programs and parameters.  
    SH_BUSY     //  The Shader has all the input buffers filled.  Does not accepts commands.  
};

/**
 *
 *  This defines the different states in which the Shader Decode stage
 *  can be.
 *
 */

enum ShaderDecodeState
{
    SHDEC_READY,    //  The Shader Decode stage can receive instructions.  
    SHDEC_BUSY      //  The Shader Decode stage can not receive instructions.  
};

}  // namespace arch

#endif
