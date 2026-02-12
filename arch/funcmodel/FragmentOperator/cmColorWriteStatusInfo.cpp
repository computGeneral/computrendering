/**************************************************************************
 *
 * Color Write Status Info implementation file.
 *
 */

/**
 *
 *  @file ColorWriteStatusInfo.cpp
 *
 *  This file implements the Color Write Status Info class.
 *
 *  This class implements carries state information between Color Write
 *  and Z Test boxes.
 *
 */

#include "cmColorWriteStatusInfo.h"

using namespace arch;

//  Color Write Status Info constructor.  
ColorWriteStatusInfo::ColorWriteStatusInfo(ColorWriteState stat)
{
    //  Set carried state.  
    state = stat;

    //  Set object color.  
    setColor(state);

    setTag("CWStIn");
}


//  Get state info carried by the object.  
ColorWriteState ColorWriteStatusInfo::getState()
{
    return state;
}
