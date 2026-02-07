/**************************************************************************
 *
 * Triangle Setup State Info definition file.
 *
 */

/**
 *
 *  @file cmTriangleSetupStateInfo.h
 *
 *  This file defines the TriangleSetupStateInfo class.
 *
 *  The Triangle Setup State Info class is used to
 *  carry state information between Triangle Setup
 *  and Primitive Assembly.
 *
 */

#ifndef _TRIANGLESETUPSTATEINFO_
#define _TRIANGLESETUPSTATEINFO_

#include "DynamicObject.h"
#include "cmTriangleSetup.h"

namespace cg1gpu
{

/**
 *
 *  This class defines a container for the state signals
 *  that the Triangle Setup mdu sends to the Primitive Assembly mdu.
 *
 *  Inherits from Dynamic Object that offers dynamic memory
 *  support and tracing capabilities.
 *
 */


class TriangleSetupStateInfo : public DynamicObject
{
private:
    
    TriangleSetupState state;    //  The Triangle Setup state.  

public:

    /**
     *
     *  Creates a new TriangleSetupStateInfo object.
     *
     *  @param state The triangle setup state carried by this
     *  triangle setup state info object.
     *
     *  @return A new initialized TriangleSetupStateInfo object.
     *
     */
     
    TriangleSetupStateInfo(TriangleSetupState state);
    
    /**
     *
     *  Returns the triangle setup state carried by the triangle setup
     *  state info object.
     *
     *  @return The triangle setup state carried in the object.
     *
     */
     
    TriangleSetupState getState();
};

} // namespace cg1gpu

#endif
