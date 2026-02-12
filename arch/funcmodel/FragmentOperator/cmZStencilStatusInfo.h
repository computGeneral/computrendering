/**************************************************************************
 *
 * Z Stencil Status Info definition file.
 *
 */

/**
 *
 *  @file cmZStencilStatusInfo.h
 *
 *  This file defines the Z Stencil Status Info class.
 *
 *  This class defines objects that carry state information
 *  from Z Stencil Test to Fragment FIFO (Interpolator).
 *
 */

#ifndef _ZSTENCILSTATUSINFO_

#define _ZSTENCILSTATUSINFO_

#include "DynamicObject.h"
#include "cmZStencilTest.h"

namespace arch
{

/**
 *
 *  This class defines the objects that carry state information from
 *  the Z Stencil Test mdu to Fragment FIFO (Interpolator) mdu.
 *
 *  The class inherits from the DynamicObject class that offers basic
 *  dynamic memory and tracing features.
 *
 */

class ZStencilStatusInfo: public DynamicObject
{
private:

    ZStencilTestState state;    //  The state information carried by the object.  

public:

    /**
     *
     *  ZStencilStatusInfo constructor.
     *
     *  @param state The state information the object will carry.
     *
     *  @return A new ZStencilStatusInfo object.
     *
     */

    ZStencilStatusInfo(ZStencilTestState state);


    /**
     *
     *  Get the state information carried by the object.
     *
     *  @return The state information carried by the object.
     *
     */

    ZStencilTestState getState();

};

} // namespace arch

#endif
