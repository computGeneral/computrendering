/**************************************************************************
 *
 * Hierarchical Z State Info implementation file.
 *
 */

/**
 *
 *  @file HZStateInfo.cpp
 *
 *  This file implements the HZStateInfo class.
 *
 *  The HZStateInfo class carries state information
 *  between Hierarchical Z early test and Triangle Traversal.
 *
 */


#include "cmHZStateInfo.h"

using namespace arch;


//  Creates a new HZStateInfo object.  
HZStateInfo::HZStateInfo(HZState newState) :

    state(newState)
{
    //  Set color for tracing.  
    setColor(state);

    setTag("HZStIn");
}


//  Returns the Hierarchical Z early test state carried by the object.  
HZState HZStateInfo::getState()
{
    return state;
}
