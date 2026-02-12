/**************************************************************************
 *
 * Shader State Info definition file.
 *
 */


#ifndef _SHADERSTATEINFO_
#define _SHADERSTATEINFO_

#include "DynamicObject.h"
#include "cmShaderFetch.h"

/**
 *
 *  This class defines a container for the state signals
 *  that the shader sends to the cmoStreamController.
 *  Inherits from Dynamic Object that offers dynamic memory
 *  support and tracing capabilities.
 *
 */

namespace arch
{

class ShaderStateInfo : public DynamicObject
{
private:
    
    ShaderState state;    //  The Shader state.  
    
public:

    /**
     *
     *  Creates a new ShaderStateInfo object.
     *
     *  @param state The Shader state carried by this
     *  shader state info object.
     *
     *  @return A new initialized ShaderStateInfo object.
     *
     */
     
    ShaderStateInfo(ShaderState state);

    /**
     *
     *  Returns the shader state carried by the shader
     *  state info object.
     *
     *  @return The shader state carried in the object.
     *
     */
     
    ShaderState getState();

};

} // namespace arch

#endif
