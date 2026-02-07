/**************************************************************************
 *
 * Rasterizer State Info implementation file.
 *
 */

#include "cmRasterizerStateInfo.h"

using namespace cg1gpu;

//  Creates a new RasterizerStateInfo object.  
RasterizerStateInfo::RasterizerStateInfo(RasterizerState newState) : state(newState)
{
    //  Set color for tracing.  
    setColor(state);

    setTag("RasStIn");
}


//  Returns the rasterizer state carried by the object.  
RasterizerState RasterizerStateInfo::getState()
{
    return state;
}
