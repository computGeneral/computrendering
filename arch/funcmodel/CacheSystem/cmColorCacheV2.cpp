/**************************************************************************
 *
 * Color Cache class implementation file.
 *
 */

/**
 *
 * @file ColorCacheV2.cpp
 *
 * Implements the Color Cache class.  This class the cache used for
 * access to the color buffer in a GPU.
 *
 */

#include "cmColorCacheV2.h"
#include "GPUMath.h"
#include "bmFragmentOperator.h"
#include <cstring>

using namespace cg1gpu;

//  Color Cache class counter.  Used to create identifiers for the created Color Caches
//  that are then used to access the Memory Controller.
U32 ColorCacheV2::cacheCounter = 0;

//  Color cache constructor.
ColorCacheV2::ColorCacheV2(U32 ways, U32 lines, U32 lineSz,
        U32 readP, U32 writeP, U32 pWidth, U32 reqQSize, U32 inReqs,
        U32 outReqs, bool comprDisabled, U32 numStampUnits, U32 stampUnitStride,
        U32 maxColorBlocks, U32 blocksPerCycle,
        U32 compCycles, U32 decompCycles, char *postfix) :

        ROPCache(ways, lines, lineSz, readP, writeP, pWidth, reqQSize,
            inReqs, outReqs, comprDisabled, numStampUnits, stampUnitStride, maxColorBlocks, blocksPerCycle, compCycles,
            decompCycles, COLORWRITE, "ColorCache", postfix)

{
    //  Get the ROP Cache identifier.
    cacheID = cacheCounter;

    //  Update the number of created Color Caches.
    cacheCounter++;

    //  Set reset value for clear.
    for(U32 i = 0; i < MAX_BYTES_PER_COLOR; i++)
        clearResetValue[i] = 0x00;

 }


//  Clears the color cache.
bool ColorCacheV2::clear(U08 *color)
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
                //  Set the ROP clear value register.
                for(U32 i = 0; i < MAX_BYTES_PER_PIXEL; i++)
                    clearROPValue[i] = color[i];

                //  Unset reset mode.
                clearMode = FALSE;
            }
        }
    }
    else
    {
        //  NOTE:  SHOULD TAKE INTO ACCOUNT THE RESOLUTION SO NOT ALL
        //  BLOCKS HAD TO BE CLEARED EVEN IF UNUSED AT CURRENT RESOLUTION.  */

        //  Set clear cycles.
        clearCycles = (U32) ceil((F32) maxBlocks / (F32) blocksCycle);

        //  Set clear mode.
        clearMode = TRUE;

        //  Reset the cache.
        resetMode = TRUE;
    }

    return clearMode;
}


//  Copies the block state memory.
void ColorCacheV2::copyBlockStateMemory(ROPBlockState *buffer, U32 blocks)
{
    CG_ASSERT_COND(!(blocks > maxBlocks), "More blocks to copy than blocks in the state memory.");
    //  Copy the block states.
    memcpy(buffer, blockState, sizeof(ROPBlockState) * blocks);
}

