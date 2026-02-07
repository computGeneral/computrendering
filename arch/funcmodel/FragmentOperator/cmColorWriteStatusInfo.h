/**************************************************************************
 *
 * Color Write Status Info definition file.
 *
 */

/**
 *
 *  @file cmColorWriteStatusInfo.h
 *
 *  This file defines the Color Write Status Info class.
 *
 *  This class defines objects that carry state information
 *  from Color Write to Z Test.
 *
 */

#ifndef _COLORWRITESTATUSINFO_

#define _COLORWRITESTATUSINFO_

#include "DynamicObject.h"
#include "cmColorWrite.h"

namespace cg1gpu
{

/**
 *
 *  This class defines the objects that carry state information from
 *  the Color Write mdu to the Z Test mdu.
 *
 *  The class inherits from the DynamicObject class that offers basic
 *  dynamic memory and tracing features.
 *
 */

class ColorWriteStatusInfo: public DynamicObject
{
private:

    ColorWriteState state;      //  The state information carried by the object.  

public:

    /**
     *
     *  ColorWriteStatusInfo constructor.
     *
     *  @param state The state information the object will carry.
     *
     *  @return A new ColorWriteStatusInfo object.
     *
     */

    ColorWriteStatusInfo(ColorWriteState state);


    /**
     *
     *  Get the state information carried by the object.
     *
     *  @return The state information carried by the object.
     *
     */

    ColorWriteState getState();

};

} // namespace cg1gpu

#endif
