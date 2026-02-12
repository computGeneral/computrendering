/**************************************************************************
 *
 * Clipper Status Info implementation file.
 *
 */

/**
 *
 *  @file ClipperStatusInfo.cpp
 *
 *  This file implements the CliperStatusInfo class.
 *
 *  This class objects carries status information from
 *  the Clipper to Primitive Assembly
 *
 */

#include "cmClipperStatusInfo.h"

using namespace arch;

//  Creates a new ClipperStatusInfo object.  
ClipperStatusInfo::ClipperStatusInfo(ClipperStatus newStatus) :
    status(newStatus)
{
    //  Set color for tracing.  
    setColor(status);

    setTag("ClStuIn");
}


//  Returns the clipper status carried by the object.  
ClipperStatus ClipperStatusInfo::getStatus()
{
    return status;
}
