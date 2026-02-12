/**************************************************************************
 *
 * Z Operation definition file.
 *
 */

/**
 *
 *  @file cmZOperation.h
 *
 *  This file defines the Z Operation class.
 *
 *  This class defines the Z and Stencil tests started in the
 *  Z Stencil Test mdu that are sent through the test unit (signal).
 *
 */

#include "DynamicObject.h"

#ifndef _ZOPERATION_

#define _ZOPERATION_

namespace arch
{

/**
 *
 *  This class stores the information about Z and stencil tests
 *  issued to the test unit in the Z Stencil Test mdu.  The objects
 *  of this class circulate through the test signal to simulate
 *  the test operation latency.
 *
 *  This class inherits from the DynamicObject class that offers
 *  basic dynamic memory management and statistic gathering capabilities.
 *
 */

class ZOperation: public DynamicObject
{
private:

    U32 id;          //  Operation identifier.  

public:

    /**
     *
     *  Z Operation constructor.
     *
     *  @param id The operation identifier.
     *
     *  @return A new Z Operation object.
     *
     */

    ZOperation(U32 id);

    /**
     *
     *  Gets the Z operation identifier.
     *
     *  @return The Z operation identifier.
     *
     */

    U32 getID();

};

} // namespace arch

#endif
