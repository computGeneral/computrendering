/**************************************************************************
 *
 * Z Operation implementation file.
 *
 */

/**
 *
 *  @file ZOperation.cpp
 *
 *  This file implements the Z Operation class.
 *
 *  This class implements the Z and stencil tests issued to the
 *  test unit in the Z Stencil Test mdu.
 *
 */

#include "cmZOperation.h"

using namespace cg1gpu;

//  Z Operation constructor.  
ZOperation::ZOperation(U32 opID)
{
    //  Set operation parameters.  
    id = opID;

    //  Set object color.  
    setColor(0);

    setTag("ZOp");
}

//  Return Z operation id.  
U32 ZOperation::getID()
{
    return id;
}


