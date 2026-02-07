/**************************************************************************
 *
 * Clipper Status Info definition file.
 *
 */


#ifndef _CLIPPERSTATUSINFO_
#define _CLIPPERSTATUSINFO_

#include "DynamicObject.h"
#include "cmClipper.h"

namespace cg1gpu
{

/**
 *
 *  @file cmClipperStatusInfo.h
 *
 *  This file defines the Clipper Stateus Info class.
 *
 *  Carries the Clipper status to Primitive Assembly
 *
 */
 
/**
 *
 *  This class defines a container for the state signals
 *  that the Clipper sends to Primitive Assembly.
 *
 *  Inherits from Dynamic Object that offers dynamic memory
 *  support and tracing capabilities.
 *
 */


class ClipperStatusInfo : public DynamicObject
{
private:
    
    ClipperStatus status;   //  The clipper status.  

public:

    /**
     *
     *  Creates a new ClipperStatusInfo object.
     *
     *  @param state The clipper status carried by this clipper status
     *  info object.
     *
     *  @return A new initialized ClipperStatusInfo object.
     *
     */
     
    ClipperStatusInfo(ClipperStatus state);
    
    /**
     *
     *  Returns the clipper status carried by the clipper status info object.
     *
     *  @return The clipper status carried in the object.
     *
     */
     
    ClipperStatus getStatus();
};

} // namespace cg1gpu

#endif
