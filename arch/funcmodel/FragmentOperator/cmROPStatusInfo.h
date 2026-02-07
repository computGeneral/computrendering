/**************************************************************************
 *
 * ROP Status Info definition file.
 *
 */

/**
 *
 *  @file cmROPStatusInfo.h
 *
 *  This file defines the ROP Status Info class.
 *
 *  This class defines objects that carry state information
 *  from a Generic ROP mdu to a producer stage that sends fragments
 *  to the Generic ROP mdu.
 *
 */

#ifndef _ROPSTATUSINFO_

#define _ROPSTATUSINFO_

#include "DynamicObject.h"
#include "cmGenericROP.h"

namespace cg1gpu
{

/**
 *
 *  This class defines the objects that carry state information from
 *  a Generic ROP mdu to a producer stage that sends fragments to the
 *  Geneneric ROP mdu.
 *
 *  The class inherits from the DynamicObject class that offers basic
 *  dynamic memory and tracing features.
 *
 */

class ROPStatusInfo: public DynamicObject
{
private:

    ROPState state;     //  The state information carried by the object.  

public:

    /**
     *
     *  ROPStatusInfo constructor.
     *
     *  @param state The state information the object will carry.
     *
     *  @return A new ROPStatusInfo object.
     *
     */

    ROPStatusInfo(ROPState state);


    /**
     *
     *  Get the state information carried by the object.
     *
     *  @return The state information carried by the object.
     *
     */

    ROPState getState();

};

} // namespace cg1gpu

#endif
