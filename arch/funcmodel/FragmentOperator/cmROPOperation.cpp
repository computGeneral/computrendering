/**************************************************************************
 *
 * ROP Operation implementation file.
 *
 */

/**
 *
 *  @file ROPOperation.cpp
 *
 *  This file implements the ROP Operation class.
 *
 *  This class implements the ROP operations started in a Generic ROP mdu
 *  that are sent through an operation signal simulating the latency of
 *  the ROP operation unit.
 *
 */

#include "cmROPOperation.h"

using namespace cg1gpu;

//  ROP Operation constructor.  
ROPOperation::ROPOperation(ROPQueue *opStamp)
{
    //  Set operation parameters.  
    operatedStamp = opStamp;

    //  Set object color.  
    setColor(0);

    setTag("ROPOp");
}

//  Return Z operation id.  
ROPQueue *ROPOperation::getROPStamp()
{
    return operatedStamp;
}


