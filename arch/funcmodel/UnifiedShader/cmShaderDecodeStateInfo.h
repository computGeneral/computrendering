/**************************************************************************
 *
 * Shader Decode State Info definition file.
 *
 */


#ifndef _SHADERDECODESTATEINFO_
#define _SHADERDECODESTATEINFO_

#include "DynamicObject.h"
#include "cmShaderDecodeExecute.h"

namespace cg1gpu
{

/**
 *
 *  This class defines a container for the state signals
 *  that the shader decode mdu sends to the shader fetch mdu.
 *  Inherits from Dynamic Object that offers dynamic memory
 *  support and tracing capabilities.
 *
 */


class ShaderDecodeStateInfo : public DynamicObject
{

private:
    
    ShaderDecodeState state;    //  The Shader decode state.  

public:

    /**
     *
     *  Creates a new ShaderDecodeStateInfo object.
     *
     *  @param state The Shader decode state carried by this
     *  shader decode state info object.
     *
     *  @return A new initialized ShaderDecodeStateInfo object.
     *
     */
     
    ShaderDecodeStateInfo(ShaderDecodeState state);
    
    /**
     *
     *  Returns the shader decode state carried by the shader
     *  decode state info object.
     *
     *  @return The shader decode state carried in the object.
     *
     */
     
    ShaderDecodeState getState();
};

} // namespace cg1gpu

#endif
