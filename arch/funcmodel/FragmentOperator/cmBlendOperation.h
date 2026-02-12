/**************************************************************************
 *
 * Blend Operation definition file. 
 *
 */
 
/**
 *
 *  @file cmBlendOperation.h 
 *
 *  This file defines the Blend Operation class.
 *
 *  This class defines the blend operations started in the
 *  ColorWrite mdu that are sent through the blend unit (signal).
 *
 */

#include "DynamicObject.h"
 
#ifndef _BLENDOPERATION_

#define _BLENDOPERATION_

namespace arch
{

/**
 *
 *  This class stores the information about blend operations
 *  issued to the blend unit in the Color Write mdu.  The objects
 *  of this class circulate through the Blend signal to simulate
 *  the blend operation latency.
 *
 *  This class inherits from the DynamicObject class that offers
 *  basic dynamic memory management and statistic gathering capabilities.
 *
 */

class BlendOperation: public DynamicObject
{
private:

    U32 id;          //  Operation identifier.  
    
public:

    /**
     *
     *  Blend Operation constructor.
     *
     *  @param id The operation identifier.
     *
     *  @return A new Blend Operation object.
     *
     */
     
    BlendOperation(U32 id);
    
    /**
     *
     *  Gets the blend operation identifier.
     *
     *  @return The blend operation identifier.
     *
     */
     
    U32 getID();
    
};

} // namespace arch

#endif
