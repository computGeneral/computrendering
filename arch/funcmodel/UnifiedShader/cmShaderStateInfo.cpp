/**************************************************************************
 *
 * Shader State Info implementation file.
 *
 */

#include "cmShaderStateInfo.h"


using namespace arch;

//  Creates a new Shader State Info object.  
ShaderStateInfo::ShaderStateInfo(ShaderState newState) : state(newState)
{
    //  Set object color for tracing.  
    setColor(state);

    setTag("ShStIn");
}

//  Returns the shader state carried by the object.  
ShaderState ShaderStateInfo::getState()
{
    return state;
}

