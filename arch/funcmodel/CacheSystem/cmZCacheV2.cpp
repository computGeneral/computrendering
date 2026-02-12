/**************************************************************************
 *
 * Z Cache class implementation file.
 *
 */

/**
 *
 * @file ZCacheV2.cpp
 *
 * Implements the Z Cache class.  This class the cache used for access to the Z buffer in a GPU.
 *
 */

#include "cmZCacheV2.h"
#include "GPUMath.h"
#include "bmFragmentOperator.h"

using namespace arch;

//  Z Cache class counter.  Used to create identifiers for the created Z Caches
//  that are then used to access the Memory Controller.
U32 ZCacheV2::cacheCounter = 0;


//  Z cache constructor.
ZCacheV2::ZCacheV2(U32 ways, U32 lines, U32 lineSz,
        U32 readP, U32 writeP, U32 pWidth, U32 reqQSize, U32 inReqs,
        U32 outReqs, bool zComprDisabled, U32 numStampUnits, U32 stampUnitStride, U32 maxZBlocks, U32 blocksPerCycle,
        U32 compCycles, U32 decompCycles, char *postfix) :

        ROPCache(ways, lines, lineSz, readP, writeP, pWidth, reqQSize,
            inReqs, outReqs, zComprDisabled, numStampUnits, stampUnitStride, maxZBlocks, blocksPerCycle, compCycles,
            decompCycles, ZSTENCILTEST, "ZCache", postfix)

{
    //  Get the Z Cache identifier.
    cacheID = cacheCounter;

    //  Update the number of created Z Caches.
    cacheCounter++;

    //  Set reset value for clear.
    for(U32 i = 0; i < (MAX_BYTES_PER_PIXEL >> 2); i++)
        ((U32 *) clearResetValue)[i] = 0x00ffffff;

}

//  Clears the Z cache.
bool ZCacheV2::clear(U32 depth, U08 stencil)
{
    //  Reset the cache.
    if (clearMode)
    {
        //  Check clear cycles remaining.
        if (clearCycles > 0)
        {
            //  Update clear cycles.
            clearCycles--;

            //  Check if end of clear.
            if (clearCycles == 0)
            {
                //  Set the clear value registers.
                clearDepth = depth;
                clearStencil = stencil;

                //  Set the ROP data clear value
                ((U32 *) clearROPValue)[0] = (clearStencil << 24) | (clearDepth & 0x00ffffff);

                //  Unset reset mode.  
                clearMode = FALSE;
            }
        }
    }
    else
    {
        //  NOTE:  SHOULD TAKE INTO ACCOUNT THE RESOLUTION SO NOT ALL
        //  BLOCKS HAD TO BE CLEARED EVEN IF UNUSED AT CURRENT RESOLUTION.

        //  Set clear cycles.
        clearCycles = (U32) ceil((F32) maxBlocks / (F32) blocksCycle);

        //  Set clear mode.
        clearMode = TRUE;

        //  Reset the cache.
        resetMode = TRUE;
    }

    return clearMode;
}

//  Check HZ updates.
bool ZCacheV2::updateHZ(U32 &block, U32 &z)
{
    //  Check if there is an updated block.
    if (blockWasWritten)
    {
        //  Return block identifier and block Z.
        block = writtenBlock;
        z = wrBlockMaxVal;

        //  Reset updated HZ block flag.
        blockWasWritten = false;

        return true;
    }
    else
        return false;
}

void ZCacheV2::processNextWrittenBlock(U08* outputBuffer, U32 size)
{
    U32* data = (U32*) outputBuffer;
    U32 dataSize = size / sizeof(U32);
    
    U32 maxZ;
    
    //  Calculate the maximum depth/Z.
    bmoFragmentOperator::blockMaxZ(data, dataSize, maxZ);

    // Store for later use
    wrBlockMaxVal = maxZ;
}

//  Copies the block state memory.
void ZCacheV2::copyBlockStateMemory(ROPBlockState *buffer, U32 blocks)
{
    CG_ASSERT_COND(!(blocks > maxBlocks), "More blocks to copy than blocks in the state memory.");
    //  Copy the block states.
    memcpy(buffer, blockState, sizeof(ROPBlockState) * blocks);
}

