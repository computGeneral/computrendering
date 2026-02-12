/**************************************************************************
 *
 * Blend Operation implementation file.
 *
 */

/**
 *
 *  @file BlendOperation.cpp
 *
 *  This file implements the Blend Operation class.
 *
 *  This class implements the blend operations issued to the
 *  blend unit in the ColorWrite mdu.
 *
 */

#include "cmBlendOperation.h"

using namespace arch;


//  Blend Operation constructor.  
BlendOperation::BlendOperation(U32 opID)
{
    //  Set operation parameters.  
    id = opID;

    //  Set object color.  
    setColor(0);

    setTag("BlOp");
}

//  Return blend operation id.  
U32 BlendOperation::getID()
{
    return id;
}


