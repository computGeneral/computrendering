/**************************************************************************
 *
 * Triangle Setup State Info implementation file.
 *
 */

/**
 *
 *  @file TriangleSetupStateInfo.cpp
 *
 *  This file implements the TriangleSetupStateInfo class.
 *
 *  The TriangleSetupStateInfo class carries state information
 *  between Triangle Setup and Primitive Assembly.
 *
 */


#include "cmTriangleSetupStateInfo.h"


using namespace arch;

//  Creates a new TriangleSetupStateInfo object.  
TriangleSetupStateInfo::TriangleSetupStateInfo(TriangleSetupState newState) :

    state(newState)
{
    //  Set color for tracing.  
    setColor(state);

    setTag("TrSStIn");
}


//  Returns the triangle setup state carried by the object.  
TriangleSetupState TriangleSetupStateInfo::getState()
{
    return state;
}
