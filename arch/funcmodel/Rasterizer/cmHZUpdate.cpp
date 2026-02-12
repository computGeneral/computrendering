/**************************************************************************
 *
 * Hierarchical Z Update implementation file.
 *
 */

/**
 *
 *  @file HZUpdate.cpp
 *
 *  This file implements the HZUpdate class.
 *
 *  The HZUpdate class carries block updates from Z Test
 *  to the Hierarchical Z buffer.
 *
 */


#include "cmHZUpdate.h"

using namespace arch;



//  Creates a new HZUpdate object.  
HZUpdate::HZUpdate(U32 address, U32 z) :
blockAddress(address), blockZ(z)
{
    setTag("HZUpd");
}

//  Returns the address of the HZ Buffer block to be updated.  
U32 HZUpdate::getBlockAddress()
{
    return blockAddress;
}

//  Returns the new z for the HZ Buffer block to be updated.  
U32 HZUpdate::getBlockZ()
{
    return blockZ;
}
