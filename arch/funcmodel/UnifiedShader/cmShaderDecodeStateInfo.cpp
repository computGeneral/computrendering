/**************************************************************************
 *
 * Shader Decode State Info implementation file.
 *
 */

#include "cmShaderDecodeStateInfo.h"

using namespace cg1gpu;

//  Creates a new Shader State Info object.  
ShaderDecodeStateInfo::ShaderDecodeStateInfo(ShaderDecodeState newState) : state(newState)
{
    //  Set color for tracing.  
    setColor(state);

    setTag("ShDStIn");
}


//  Returns the shader decode state carried by the object.  
ShaderDecodeState ShaderDecodeStateInfo::getState()
{
    return state;
}
