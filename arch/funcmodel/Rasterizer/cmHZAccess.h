/**************************************************************************
 *
 * Hierarchical Z Buffer Access definition file.
 *
 */

/**
 *
 *  @file cmHZAccess.h
 *
 *  This file defines the HZAccess class.
 *
 *  The Hierarchical Z Access class is used to simulate
 *  the latency of access operations in the Hierarchical
 *  Z Buffer.
 *
 */

#ifndef _HZACCESS_
#define _HZACCESS_

#include "DynamicObject.h"

namespace cg1gpu
{

/**
 *
 *  Defines the Hierarchical Z buffer access operations.
 *
 */
enum HZOperation
{
    HZ_WRITE,   //  A write operation over the Hierarchical Z Buffer.  
    HZ_READ     //  A read operation over the Hierarchical Z Buffer.  
};


/**
 *
 *  This class is used to simulate the operation latency of
 *  accesses to the Hierarchical Z Buffer.
 *
 *  Inherits from Dynamic Object that offers dynamic memory
 *  support and tracing capabilities.
 *
 */


class HZAccess : public DynamicObject
{
private:

    HZOperation operation;  //  The operation to be performed over the HZ Buffer.  
    U32 address;         //  The HZ buffer block to be accessed.  
    U32 data;            //  Data to write over the HZ Buffer.  
    U32 cacheEntry;      //  The HZ Cache entry where to store the HZ Buffer data.  

public:

    /**
     *
     *  Creates a new HZAccess object.
     *
     *  @param op Operation to perform.
     *  @param address The block address to be updated.
     *  @param data The z to write into the block Z (for
     *  HZ_WRITE) or the cache entry where to store the HZ
     *  block data (HZ_READ).
     *
     *  @return A new initialized HZAccess object.
     *
     */

    HZAccess(HZOperation op, U32 address, U32 data);

    /**
     *
     *  Returns the access operation to perform.
     *
     *  @return Access operation.
     *
     */

    HZOperation getOperation();

    /**
     *
     *  Returns the address of the HZ block to be accessed.
     *
     *  @return The block address of the HZ buffer block to
     *  be accessed.
     *
     */

    U32 getAddress();

    /**
     *
     *  Returns the new z for the HZ buffer block to be written.
     *
     *  @return The new Z for the block to be written.
     *
     */

    U32 getData();

    /**
     *
     *  Returns the HZ Cache entry where to store the read HZ
     *  block Z.
     *
     *  @param HZ Cache entry where to perform the read access
     *  to the HZ buffer.
     *
     */

    U32 getCacheEntry();

};

} // namespace cg1gpu

#endif
