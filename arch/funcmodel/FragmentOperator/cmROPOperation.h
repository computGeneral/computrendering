/**************************************************************************
 *
 * ROP Operation definition file.
 *
 */

/**
 *
 *  @file cmROPOperation.h
 *
 *  This file defines the ROP Operation class.
 *
 *  This class defines the ROP operations started in a Generic ROP mdu that
 *  sent through the ROP operation unit (latency signal).
 *
 */

#include "DynamicObject.h"
#include "cmGenericROP.h"

#ifndef _ROPOPERATION_

#define _ROPOPERATION_

namespace cg1gpu
{

/**
 *
 *  This class stores the information about ROP operations issued to the operation unit
 *  of a Generic ROP mdu.  The objects of this class circulate through the operation
 *  latency signal to simulate the operation latency.
 *
 *  This class inherits from the DynamicObject class that offers
 *  basic dynamic memory management and statistic gathering capabilities.
 *
 */

class ROPOperation: public DynamicObject
{
private:

    ROPQueue *operatedStamp;        //  Pointer to an operated stamp object.  

public:

    /**
     *
     *  ROP Operation constructor.
     *
     *  @param opStamp A pointer to the stamp object that is being operated.
     *
     *  @return A new ROP Operation object.
     *
     */

    ROPOperation(ROPQueue *opStamp);

    /**
     *
     *  Gets the pointer to the stamp object being operated
     *
     *  @return The pointer to the stamp object being operated.
     *
     */

    ROPQueue *getROPStamp();

};

} // namespace cg1gpu

#endif
