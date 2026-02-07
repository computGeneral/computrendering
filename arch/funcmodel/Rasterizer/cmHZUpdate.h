/**************************************************************************
 *
 * Hierarchical Z Update definition file.
 *
 */

/**
 *
 *  @file cmHZUpdate.h
 *
 *  This file defines the HZUpdate class.
 *
 *  The Hieararchical Z Update class is used to block updates
 *  from Z Test to the Hierarchical Z buffer.
 *
 */

#ifndef _HZUPDATE_
#define _HZUPDATE_

#include "DynamicObject.h"

namespace cg1gpu
{

/**
 *
 *  This class defines a container for the block updates
 *  that the Z Test mdu sends to the Hierarchical Z buffer.
 *
 *  Inherits from Dynamic Object that offers dynamic memory
 *  support and tracing capabilities.
 *
 */


class HZUpdate : public DynamicObject
{
private:

    U32 blockAddress;    //  The HZ buffer block to be updated.  
    U32 blockZ;          //  The new z for the HZ buffer block to be updated.  

public:

    /**
     *
     *  Creates a new HZUpdate object.
     *
     *  @param address The block address to be updated.
     *  @param z The block Z value.
     *
     *  @return A new initialized HZUpdate object.
     *
     */

    HZUpdate(U32 address, U32 z);

    /**
     *
     *  Returns the address of the HZ block to be updated.
     *
     *  @return The block address of the HZ buffer block to
     *  be updated.
     *
     */

    U32 getBlockAddress();

    /**
     *
     *  Returns the new z for the HZ buffer block to be updated.
     *
     *  @return The new Z for the block to be updated.
     *
     */

    U32 getBlockZ();

};

} // namespace cg1gpu

#endif
