/**************************************************************************
 *
 * Clipper State Info definition file.
 *
 */


#ifndef _CLIPPERSTATEINFO_
#define _CLIPPERSTATEINFO_

#include "DynamicObject.h"
#include "cmClipper.h"

namespace cg1gpu
{

/**
 *
 *  @file cmClipperStateInfo.h
 *
 *  This file defines the Clipper State Info class.
 *
 */
 
/**
 *
 *  This class defines a container for the state signals
 *  that the Clipper sends to the Command Processor
 *
 *  Inherits from Dynamic Object that offers dynamic memory
 *  support and tracing capabilities.
 *
 */


class ClipperStateInfo : public DynamicObject
{
private:
    
    ClipperState state;     //  The clipper state.  

public:

    /**
     *
     *  Creates a new ClipperStateInfo object.
     *
     *  @param state The clipper state carried by this clipper state info
     *  object.
     *
     *  @return A new initialized ClipperStateInfo object.
     *
     */
     
    ClipperStateInfo(ClipperState state);
    
    /**
     *
     *  Returns the clipper state carried by the clipper state info object.
     *
     *  @return The clipper state carried in the object.
     *
     */
     
    ClipperState getState();
};

} // namespace cg1gpu

#endif
