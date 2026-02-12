/**************************************************************************
 *
 * Clipper State Info implementation file.
 *
 */

/**
 *
 *  @file ClipperStateInfo.cpp
 *
 *  This file implements the Clipper State Info class.
 *
 *  This class carries state information from the Clipper
 *  to the Command Processor.
 *
 */


#include "cmClipperStateInfo.h"

using namespace arch;

//  Creates a new ClipperStateInfo object.  
ClipperStateInfo::ClipperStateInfo(ClipperState newState) :
    state(newState)
{
    //  Set color for tracing.  
    setColor(state);

    setTag("ClSteIn");
}


//  Returns the clipper state carried by the object.  
ClipperState ClipperStateInfo::getState()
{
    return state;
}
