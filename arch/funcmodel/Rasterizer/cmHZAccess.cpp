/**************************************************************************
 *
 * Hierarchical Z Access implementation file.
 *
 */

/**
 *
 *  @file HZAccess.cpp
 *
 *  This file implements the HZAccess class.
 *
 *  The HZAccess class is used to simulate the access
 *  over the Hierarchical Z Buffer.
 *
 */


#include "cmHZAccess.h"

using namespace cg1gpu;

//  Creates a new HZAccess object.  
HZAccess::HZAccess(HZOperation op, U32 addr, U32 d) :
operation(op), address(addr)
{
    //  Select access operation.  
    switch(operation)
    {
        case HZ_READ:

            cacheEntry = d;

            break;

        case HZ_WRITE:

            data = d;

            break;

        default:
            CG_ASSERT("Unsupported access operation.");
            break;
    }

    setTag("HZAcc");
}

//  Returns the address of the HZ Buffer block to be accessed.  
U32 HZAccess::getAddress()
{
    return address;
}

//  Returns the Z for HZ Buffer block to be written.  
U32 HZAccess::getData()
{
    CG_ASSERT_COND(!(operation != HZ_WRITE), "Not a write operation.");
    return data;
}

//  Returns the HZ Cache entry where to perform the read access to the HZ Buffer.  
U32 HZAccess::getCacheEntry()
{
    CG_ASSERT_COND(!(operation != HZ_READ), "Not a read operation.");
    return cacheEntry;
}

//  Returns the access operation to perform.  
HZOperation HZAccess::getOperation()
{
    return operation;
}
