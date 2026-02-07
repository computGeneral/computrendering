/**************************************************************************
 *
 * Color Cache class definition file.
 *
 */

/**
 *
 *  @file cmColorCacheV2.h
 *
 *  Defines the Color Cache class.  This class defines the cache used
 *  to access the color buffer in a GPU.
 *
 */


#ifndef _COLORCACHEV2_

#define _COLORCACHEV2_

#include "GPUType.h"
#include "cmFetchCache.h"
#include "cmROPCache.h"
#include "cmMemoryTransaction.h"
#include "ColorCompressor.h"

namespace cg1gpu
{

/**
 *
 *  This class describes and implements the behaviour of the cache
 *  used to access the color buffer in a GPU.
 *  The color cache is used in the Color Write GPU unit.
 *
 *  This classes derives from the ROPCache interface class.
 *
 *  This class uses the Fetch Cache.
 *
 */

class ColorCacheV2 : public ROPCache
{

private:

    static U32 cacheCounter;     //  Class variable used to count and create identifiers for the created Color Caches.  

protected:
    //virtual void processNextWrittenBlock(U08* outputBuffer, U32 size);
    
    virtual bmoCompressor& getCompressor() {
        return bmoColorCompressor::getInstance();
    }

public:

    /**
     *
     *  Color Cache constructor.
     *
     *  Creates and initializes a Color Cache object.
     *
     *  @param numWays Number of ways in the cache.
     *  @param numLines Number of lines in the cache.
     *  @param lineSize Size of the cache line in bytes.
     *  @param bytesStamp Number of bytes per stamp.
     *  @param readPorts Number of read ports.
     *  @param writePorts Number of write ports.
     *  @param portWidth Width in bytes of the write and read ports.
     *  @param reqQueueSize Color cache request to memory queue size.
     *  @param inputRequests Number of read requests and input buffers.
     *  @param outputRequests Number of write requests and output buffers.
     *  @param disableCompr Disables color compression.
     *  @param numStampUnits Number of stamp units in the GPU.
     *  @param stampUnitStride Stride in blocks for the blocks that are assigned to a stamp unit.
     *  @param maxBlocks Maximum number of sopported color blocks in the color buffer.
     *  @param blocksCycle Number of state block entries that can be modified (cleared) per cycle.
     *  @param compCycles Compression cycles.
     *  @param decompCycles Decompression cycles.
     *  @param postfix The postfix for the color cache statistics.
     *
     *  @return A new initialized cache object.
     *
     */

    ColorCacheV2(U32 numWays, U32 numLines, U32 lineSize,
        U32 readPorts, U32 writePorts, U32 portWidth, U32 reqQueueSize,
        U32 inputRequests, U32 outputRequests, bool disableCompr, U32 numStampUnits,
        U32 stampUnitStride, U32 maxBlocks, U32 blocksCycle, U32 compCycles, U32 decompCycles, char *postfix);

    /**
     *
     *  Clears the color buffer.
     *
     *  @param clearColor Color value with which to
     *  clear the color buffer.
     *
     *  @return If the clear process has finished.
     *
     */

    bool clear(U08 *clearColor);

    /**
     *
     *  Copies the block state memory.
     *
     *  @param buffer Pointer to a ROP Block State buffer.
     *  @param blocks Number of blocks to copy.
     *
     */

    void copyBlockStateMemory(ROPBlockState *buffer, U32 blocks);

};

} // namespace cg1gpu

#endif
