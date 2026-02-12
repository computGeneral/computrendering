/**************************************************************************
 *
 * ROP Status Info implementation file.
 *
 */

/**
 *
 *  @file RPStatusInfo.cpp
 *
 *  This file implements the ROP Status Info class.
 *
 *  This class implements an object that carries state information
 *  from a Generic ROP mdu to a producer stage that generates fragments for the
 *  Generic ROP mdu.
 *
 */

#include "cmROPStatusInfo.h"

using namespace arch;

//  ROP Status Info constructor.  
ROPStatusInfo::ROPStatusInfo(ROPState stat)
{
    //  Set carried state.  
    state = stat;

    //  Set object color.  
    setColor(state);

    setTag("ZSTStIn");
}


//  Get state info carried by the object.  
ROPState ROPStatusInfo::getState()
{
    return state;
}
