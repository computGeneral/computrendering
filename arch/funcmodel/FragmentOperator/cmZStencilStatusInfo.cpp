/**************************************************************************
 *
 * Z Stencil Status Info implementation file.
 *
 */

/**
 *
 *  @file ZStencilStatusInfo.cpp
 *
 *  This file implements the Z Stencil Status Info class.
 *
 *  This class implements an object that carries state information
 *  from Z Stencil Test mdu to Fragment FIFO (Interpolator) mdu.
 *
 */

#include "cmZStencilStatusInfo.h"

using namespace cg1gpu;

//  Z Stencil Status Info constructor.  
ZStencilStatusInfo::ZStencilStatusInfo(ZStencilTestState stat)
{
    //  Set carried state.  
    state = stat;

    //  Set object color.  
    setColor(state);

    setTag("ZSTStIn");
}


//  Get state info carried by the object.  
ZStencilTestState ZStencilStatusInfo::getState()
{
    return state;
}
