/**************************************************************************
 *
 * Fragment FIFO State Info implementation file.
 *
 */

/**
 *
 *  @file FFIFOStateInfo.cpp
 *
 *  This file implements the FFIFOStateInfo class.
 *
 *  The FFIFOStateInfo class carries state information from Fragment FIFO to Hierarchical/Early Z.
 *
 */


#include "cmFFIFOStateInfo.h"

using namespace arch;


//  Creates a new FFIFOStateInfo object.  
FFIFOStateInfo::FFIFOStateInfo(FFIFOState newState) :

    state(newState)
{
    //  Set color for tracing.  
    setColor(state);

    setTag("FFStIn");
}


//  Returns the Fragment FIFO state carried by the object.  
FFIFOState FFIFOStateInfo::getState()
{
    return state;
}
