/**************************************************************************
 *
 * Primitive Assembly State Info implementation file.
 *
 */


/**
 *
 *  @file PrimitiveAssemblyStateInfo.h
 *
 *  This file implements the Primitive Assembly State Info class.
 *
 *  This class carries state information from Primitive Assembly to
 *  the Command Processor.
 *
 */

#include "PrimitiveAssemblyStateInfo.h"

using namespace arch;

//  Creates a new PrimitiveAssemblyStateInfo object.  
PrimitiveAssemblyStateInfo::PrimitiveAssemblyStateInfo(AssemblyState newState) :
    state(newState)
{
    //  Set color for tracing.  
    setColor(state);

    setTag("PAsStIn");
}


//  Returns the primitive assembly state carried by the object.  
AssemblyState PrimitiveAssemblyStateInfo::getState()
{
    return state;
}