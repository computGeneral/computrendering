/**************************************************************************
 *
 * Z Cache class definition file.
 *
 */

/**
 *
 *  @file cmZCacheV2.h
 *
 *  Defines the Z Cache class.  This class defines the cache used
 *  to access the depth and stencil buffer in a GPU.
 *
 */


#ifndef _ZCACHEV2_

#define _ZCACHEV2_

#include "GPUType.h"
#include "cmFetchCache.h"
#include "cmROPCache.h"
#include "cmMemoryTransaction.h"
#include "DepthCompressor.h"

namespace cg1gpu
{

/**
 *
 *  This class describes and implements the behaviour of the cache
 *  used to access the Z buffer in a GPU.
 *  The Z cache is used in the Z Write GPU unit.
 *
 *  This classes derives from the ROP Cache interface class.
 *
 *  This class uses the Fetch Cache.
 *
 */

class ZCacheV2 : public ROPCache
{

private:

    //  Z cache identification.
    static U32 cacheCounter;     //  Class variable used to count and create identifiers for the created Z Caches.  

    //  Z cache registers.
    U32 clearDepth;      //  Depth clear value.  
    U08 clearStencil;     //  Stencil clear value.  
    
    // Z cache state
    U32 wrBlockMaxVal;   //  Maximum value the elements of ROP data buffer block that was written to memory .  
    
    //static const int compressorBlockSizes[] = {64, 128, 192};

protected:
    virtual void processNextWrittenBlock(U08* outputBuffer, U32 size);
    
    virtual bmoCompressor& getCompressor() {
        return bmoDepthCompressor::getInstance();
    }
    
public:

    /**
     *
     *  Z Cache constructor.
     *
     *  Creates and initializes a Z Cache object.
     *
     *  @param numWays Number of ways in the cache.
     *  @param numLines Number of lines in the cache.
     *  @param lineSize Size of the cache line in bytes.
     *  @param readPorts Number of read ports.
     *  @param writePorts Number of write ports.
     *  @param portWidth Width in bytes of the write and read ports.
     *  @param reqQueueSize Z cache request to memory queue size.
     *  @param inputRequests Number of read requests and input buffers.
     *  @param outputRequests Number of write requests and output buffers.
     *  @param disableCompr Disables Z block compression.
     *  @param numStampUnits Number of stamp units in the GPU.
     *  @param stampUnitStride Stride in blocks for the blocks that are assigned to a stamp unit.
     *  @param maxBlocks Maximum number of sopported color blocks in the Z buffer.
     *  @param blocksCycle Number of state block entries that can be modified (cleared) per cycle.
     *  @param compCycles Compression cycles.
     *  @param decompCycles Decompression cycles.
     *  @param postfix The postfix for the z cache statistics.
     *
     *  @return A new initialized cache object.
     *
     */

    ZCacheV2(U32 numWays, U32 numLines, U32 lineSize,
        U32 readPorts, U32 writePorts, U32 portWidth, U32 reqQueueSize,
        U32 inputRequests, U32 outputRequests, bool disableCompr, U32 numStampUnits,
        U32 stampUnitStride, U32 maxBlocks,
        U32 blocksCycle, U32 compCycles, U32 decompCycles, char *postfix);

    /**
     *
     *  Clears the Z buffer.
     *
     *  @param clearDepth The depth value with which to clear the depth buffer.
     *  @param clearStencil The stencil value with which to clear the stencil buffer.
     *
     *  @return If the clear process has finished.
     *
     */

    bool clear(U32 clearDepth, U08 clearStencil);

    /**
     *
     *  Checks if there is an update for the Hierarchical Z ready
     *  and returns it.
     *
     *  @param block A reference to a variable were to store the Hierarchical Z
     *  block to be updated.
     *  @param z A reference to a variable where to store the new Z for the updated
     *  Hierarchical Z block.
     *
     *  @return If there is an update to the Hierarchical Z ready.
     *
     */

    bool updateHZ(U32 &block, U32 &z);

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
