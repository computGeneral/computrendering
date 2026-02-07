/**************************************************************************
 *
 * Z Cache class implementation file.
 *
 */

/**
 *
 * @file ZCache.cpp
 *
 * Implements the Z Cache class.  This class the cache used for
 * access to the Z buffer in a GPU.
 *
 */

#include "cmZCache.h"
#include "GPUMath.h"
#include "bmFragmentOperator.h"

using namespace cg1gpu;

/*  Z Cache class counter.  Used to create identifiers for the created Z Caches
    that are then used to access the Memory Controller.  */
U32 ZCache::cacheCounter = 0;


//  Z cache constructor.  
ZCache::ZCache(U32 ways, U32 lines, U32 stampsLine,
        U32 bytesStamp, U32 readP, U32 writeP, U32 pWidth, U32 reqQSize, U32 inReqs,
        U32 outReqs, bool zComprDisabled, U32 maxZBlocks, U32 blocksPerCycle,
        U32 compCycles, U32 decompCycles, char *postfix) :

    lineStamps(stampsLine), stampBytes(bytesStamp), readPorts(readP), writePorts(writeP), portWidth(pWidth),
    inputRequests(inReqs), outputRequests(outReqs),
    disableCompr(zComprDisabled), maxBlocks(maxZBlocks),
    blocksCycle(blocksPerCycle),
    comprLatency(compCycles), decomprLatency(decompCycles),
    lineSize(lineStamps * stampBytes)

{
    U32 i;

    //  Check parameters.  
    GPU_ASSERT(
        if (ways == 0)
            CG_ASSERT("At least one ways required.");
        if (lines == 0)
            CG_ASSERT("At least one cache line per ways required.");
        if (lineStamps == 0)
            CG_ASSERT("At least one stamp per cache line required.");
        if (stampBytes < (STAMP_FRAGMENTS * 1))
            CG_ASSERT("At least one byte per fragment in the stamp.");
        if (GPU_MOD(stampBytes, STAMP_FRAGMENTS) != 0)
            CG_ASSERT("Bytes per stamp must be a multiple of the fragments in the stamp.");
        if (readPorts == 0)
            CG_ASSERT("At least one read port required.");
        if (writePorts == 0)
            CG_ASSERT("At least one read port required.");
        if (portWidth == 0)
            CG_ASSERT("Port width must be at least one byte.");
        if (inputRequests == 0)
            CG_ASSERT("Input request queue must have at least one entry.");
        if (outputRequests == 0)
            CG_ASSERT("Output request queue must have at least one entry.");
        if (maxBlocks < ((MAX_DISPLAY_RES_X * MAX_DISPLAY_RES_Y) / (lineStamps * STAMP_FRAGMENTS)))
            CG_ASSERT("Block state memory not enough large for largest supported resolution.");
        if (blocksCycle == 0)
            CG_ASSERT("At lest one block state entry should be cleared per cycle.");
        if (comprLatency == 0)
            CG_ASSERT("Compression unit latency must be at least 1.");
        if (decomprLatency == 0)
            CG_ASSERT("Decompression unit latency must be at least 1.");
    )

    //  Get the Z Cache identifier.  
    cacheID = cacheCounter;

    //  Update the number of created Z Caches.  
    cacheCounter++;

    //  Create the fetch cache object.  
    cache = new FetchCache(ways, lines, lineStamps * bytesStamp, reqQSize, postfix);

    //  Create the input buffer.  
    inputBuffer = new U08*[inputRequests];

    //  Check allocation.  
    CG_ASSERT_COND(!(inputBuffer == NULL), "Error allocating input buffers.");
    //  Create the output buffer.  
    outputBuffer = new U08*[outputRequests];

    //  Check allocation.  
    CG_ASSERT_COND(!(outputBuffer == NULL), "Error allocating output buffers.");
    //  Allocate the mask buffers.  
    maskBuffer = new U32*[outputRequests];

    //  Check allocation.  
    CG_ASSERT_COND(!(maskBuffer == NULL), "Error allocating mask buffers.");
    //  Create read request queue.  
    readQueue = new ZCacheReadRequest[inputRequests];

    //  Check allocation.  
    CG_ASSERT_COND(!(readQueue == NULL), "Error allocating the read queue.");
    //  Create write request queue.  
    writeQueue = new ZCacheWriteRequest[outputRequests];

    //  Check allocation.  
    CG_ASSERT_COND(!(writeQueue == NULL), "Error allocating the write queue.");
    //  Allocate individual buffers.  
    for(i = 0; i < inputRequests; i++)
    {
        //  Allocate input buffer.  
        inputBuffer[i] = new U08[lineStamps * bytesStamp];

        //  Check allocation.  
        CG_ASSERT_COND(!(inputBuffer[i] == NULL), "Error allocating input buffer.");    }

    //  Allocate individual buffers.  
    for(i = 0; i < outputRequests; i++)
    {
        //  Allocate output buffer.  
        outputBuffer[i] = new U08[lineStamps * bytesStamp];

        //  Check allocation.  
        CG_ASSERT_COND(!(outputBuffer[i] == NULL), "Error allocating output buffer.");
        //  Allocate mask buffer.  
        maskBuffer[i] = new U32[(lineStamps * bytesStamp) >> 2];

        //  Check allocation.  
        CG_ASSERT_COND(!(maskBuffer[i] == NULL), "Error allocating mask buffer.");    }

    //  Allocate the Z block state table.  
    blockState = new ZBlockState[maxBlocks];

    //  Check allocation.  
    CG_ASSERT_COND(!(blockState == NULL), "Error allocating the block state information.");
    //  Allocate the write port cycle counters.  
    writeCycles = new U32[writePorts];

    //  Check allocation.  
    CG_ASSERT_COND(!(writeCycles == NULL), "Error allocating write port cycle counters.");
    //  Allocate the read port cycle counters.  
    readCycles = new U32[readPorts];

    //  Check allocation.  
    CG_ASSERT_COND(!(writeCycles == NULL), "Error allocating read port cycle counters.");
    //  Reset ticket counter.  
    nextTicket = 0;

    //  Calculate the shift bits for block addresses.  
    blockShift = GPUMath::calculateShift(lineSize);

    //  Set flush mode to false.  
    flushMode = FALSE;

    //  Set clear mode to false.  
    clearMode = FALSE;

    //  Reset the Z cache.  
    resetMode = TRUE;

    //  No fetch performed at the start.  
    fetchPerformed = false;

    //  Set pointers to the next ports (avoid warnings).  
    nextReadPort = nextWritePort = 0;
}


//  Fetches a cache line.  
bool ZCache::fetch(U32 address, U32 &way, U32 &line, DynamicObject *source)
{
    //  Check if a fetch was already performed.  
    if (fetchPerformed)
        return false;

    //  Set as fetch performed in the current cycle flag.  
    fetchPerformed = true;

    //  Try to fetch the address in the Fetch Cache.  
    return cache->fetch(address, way, line, source);
}

//  Allocates a cache/buffer line.  
bool ZCache::allocate(U32 address, U32 &way, U32 &line, DynamicObject *source)
{
    U32 i;
    U32 e;
    U32 block;
    bool wait;

    //  Check if a fetch/allocate was already performed in the current cycle.  
    if (fetchPerformed)
        return false;

    //  Set as fetch/allocate performed in the current cycle flag.  
    fetchPerformed = true;

    //  Calculate block for allocation address.  
    block = address2block(address);

    //  Wait for the block to be written and compressed before allocate it.  
    wait = FALSE;

    //  Search the block inside the write queue.  
    for(i = 0, e = GPU_MOD(nextFreeWrite + freeWrites, outputRequests);
        ((i < (outputRequests - freeWrites)) && (!wait));
        i++, e = GPU_MOD(e + 1, outputRequests))
    {
        //  Check if it is the same block.  
        if (writeQueue[e].block == block)
        {
            //  Set wait write.  
            wait = TRUE;
            GPU_DEBUG(
                printf("ZCache => Block %d for address %x is on the write queue.\n",
                    block, address);
                printf("ZCache => Wait before allocation.\n");
            )
        }
    }

    //  Check if the allocation must wait.  
    if (wait)
        return FALSE;

    //  Check the state of the block.  
    switch(blockState[block])
    {
        case Z_BLOCK_CLEAR:
        case Z_BLOCK_UNCOMPRESSED:

            /*  The line is not required to be loaded, just
                try to make an allocation in the Fetch Cache.  */
            return cache->allocate(address, way, line, source);

        case Z_BLOCK_COMPRESSED_NORMAL:
        case Z_BLOCK_COMPRESSED_BEST:

            /*  The line must be load from memory,
                try to fetch the address in the Fetch Cache.  */
            return cache->fetch(address, way, line, source);
    }

	return false;
}

//  Reads Z data from a stamp.  
bool ZCache::read(U32 address, U32 way, U32 line, U08 *data)
{
    U32 i;

    //  Search for the next free write port.  
    for(; (nextReadPort < readPorts) && (readCycles[nextReadPort] > 0); nextReadPort++);


    //  Check if cache read port is busy.  
    if ((nextReadPort < readPorts) && (readCycles[nextReadPort] == 0))
    {
        //  Try to read fetched data from the Fetch Cache.  
        if (cache->read(address, way, line, stampBytes, data))
        {
            //  Update read cycles counter.  
            readCycles[nextReadPort] += (U32) ceil((F32) stampBytes / (F32) portWidth);

            //  Update pointer to next read port.  
            nextReadPort++;

            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
}

//  Writes masked Z data from a stamp (write buffer mode).  
bool ZCache::write(U32 address, U32 way, U32 line, U08 *data,
    bool *mask)
{
    U32 i;

    //  Search for the next free write port.  
    for(; (nextWritePort < writePorts) && (writeCycles[nextWritePort] > 0); nextWritePort++);

    //  Check if write port is busy.  
    if ((nextWritePort < writePorts) && (writeCycles[nextWritePort] == 0))
    {
        //  Tries to perform a masked write to the fetch cache.  
        if (cache->write(address, way, line, stampBytes, data, mask))
        {
            //  Update write cycles counter.  
            writeCycles[nextWritePort] += (U32) ceil((F32) stampBytes / (F32) portWidth);

            //  Update pointer to the next write port.  
            nextWritePort++;

            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
}


//  Writes Z data from a stamp.  
bool ZCache::write(U32 address, U32 way, U32 line, U08 *data)
{
    U32 i;

    //  Search for the next free write port.  
    for(; (nextWritePort < writePorts) && (writeCycles[nextWritePort] > 0); nextWritePort++);

    //  Check if the cache write port is busy.  
    if ((nextWritePort < writePorts) && (writeCycles[nextWritePort] == 0))
    {
        //  Tries to perform a write to the fetch cache.  
        if (cache->write(address, way, line, stampBytes, data))
        {
            //  Update write cycles counter.  
            writeCycles[nextWritePort] += (U32) ceil((F32) stampBytes / (F32) portWidth);

            //  Update pointer to the next write port.  
            nextWritePort++;

            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else
    {
        return FALSE;
    }
}

//  Unreserves a cache line after use.  
void ZCache::unreserve(U32 way, U32 line)
{
    //  Unreserve fetch cache line.  
    cache->unreserve(way, line);
}

//  Resets the Z cache structures.  
void ZCache::reset()
{
    //  Unset clear mode.  
    clearMode = FALSE;

    //  Reset the Z cache.  
    resetMode = TRUE;
}

//  Process a memory transaction.  
void ZCache::processMemoryTransaction(MemoryTransaction *memTrans)
{
    //  Process memory transaction.  
    switch(memTrans->getCommand())
    {
        case MT_READ_DATA:

            //  Check if memory bus is in use.  
            CG_ASSERT_COND(!(memoryCycles > 0), "Memory bus still busy.");
            //  Get ticket of the transaction.  
            readTicket = memTrans->getID();

            //  Keep transaction size.  
            lastSize = memTrans->getSize();

            GPU_DEBUG(
                printf("ZCache => MT_READ_DATA ticket %d address %x size %d\n",
                    readTicket, memTrans->getAddress(), lastSize);
            )

            //  Set cycles the memory bus will be in use.  
            memoryCycles = memTrans->getBusCycles();

            //  Set memory read in progress flag.  
            memoryRead = TRUE;

            break;

        default:

            CG_ASSERT("Unsopported transaction type.");
            break;
    }
}


//  Update the memory request queue.  Generate new memory transactions.  
MemoryTransaction *ZCache::update(U64 cycle, MemState memState)
{
    //  Set memory controller state.  
    memoryState = memState;

    //  Simulate a Z cache cycle.  
    clock(cycle);

    //  Return the generated (if any) memory transaction.  
    return nextTransaction;
}

//  Copy back all the valid cache lines to memory.  
bool ZCache::flush()
{
    //  Check if the cache has been requested to be flushed.  
    if (!flushRequest)
    {
        //  Cache request received.  
        flushRequest = TRUE;

        //  Enable flush mode.  
        flushMode = TRUE;
    }
    else
    {
        //  Check if cache flush has finished.  
        if (!flushMode)
        {
            //  End of cache flush request.  
            flushRequest = FALSE;
        }
    }

    return flushMode;
}

//  Clears the Z cache.  
bool ZCache::clear(U32 depth, U08 stencil)
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
                //  Set the depth and stencil clear register.  
                clearDepth = depth;
                clearStencil = stencil;

                //  Unset reset mode.  
                clearMode = FALSE;
            }
        }
    }
    else
    {
        /*  NOTE:  SHOULD TAKE INTO ACCOUNT THE RESOLUTION SO NOT ALL
            BLOCKS HAD TO BE CLEARED EVEN IF UNUSED AT CURRENT RESOLUTION.  */

        //  Set clear cycles.  
        clearCycles = (U32) ceil((F32) maxBlocks / (F32) blocksCycle);

        //  Set clear mode.  
        clearMode = TRUE;

        //  Reset the cache.  
        resetMode = TRUE;
    }

    return clearMode;
}

//  Signals a swap in the Z buffer address.  
void ZCache::swap(U32 bufferAddress)
{
    //  Sets the Z buffer addresss.  
    zBufferAddress = bufferAddress;
}

//  Check HZ updates.  
bool ZCache::updateHZ(U32 &block, U32 &z)
{
    //  Check if there is an updated block.  
    if (updatedHZ)
    {
        //  Return block and block Z.  
        block = hzBlock;
        z = hzBlockZ;

        //  Reset updated HZ block flag.  
        updatedHZ = FALSE;

        return TRUE;
    }
    else
        return FALSE;
}


//  Z cache simulation rutine.  
void ZCache::clock(U64 cycle)
{
    U32 i, e;
    U32 readEndReq;
    MemoryTransaction *memTrans;
    //U32 comprSize;
    U32 fillReadRequest;
    bool waitWrite;
    U08 comprBuffer[UNCOMPRESSED_BLOCK_SIZE];
    bmoFragmentOperator::CompressionMode comprMode;
    U32 min;
    U32 max;
    U32 minZ;
    U32 maxZ;

    GPU_DEBUG(
        printf("ZCache => Cycle %lld\n", cycle);
    )

    //  Receive memory transactions and state from Memory Controller.  

    //  Check if flushing.  
    if (flushMode)
    {
        GPU_DEBUG(
            printf("ZCache => Flush.\n");
        )

        //  Continue flushing the cache lines.  
        flushMode = !cache->flush();
    }

    //  Check if the Z cache is in reset mode.  
    if (resetMode)
    {
        GPU_DEBUG(
            printf("ZCache => Reset.\n");
        )

        //  Reset the cache.  
        cache->reset();

        //  Reset the state of the Z block state table.  
        for(i = 0; i < maxBlocks; i++)
        {
            //  Set block as clear at reset.  
            blockState[i] = Z_BLOCK_CLEAR;
        }

        //  Check if it is clear mode.  
        if (!clearMode)
        {
            //  Set default value of the z buffer address register.  
            zBufferAddress = 0x500000;

            //  Set default value of the clear depth and stencil registers.  
            clearDepth = 0x00ffffff;
            clearStencil = 0x00;
        }

        //  Reset free memory tickets counter.  
        freeTickets = MAX_MEMORY_TICKETS;

        //  Reset memory bus cycles.  
        memoryCycles = 0;

        //  Reset cache write port cycles.  
        for(i = 0; i < writePorts; i++)
            writeCycles[i] = 0;

        //  Reset cache read port cycles.  
        for(i = 0; i < readPorts; i++)
            readCycles[i] = 0;

        //  Reset compress cycles.  
        compressCycles = 0;

        //  Reset uncompress cycles.  
        uncompressCycles = 0;

        //  Reset compressed/uncompressed buffer counters.  
        compressed = uncompressed = 0;

        //  Reset compressed/uncompressed buffer pointers.  
        nextCompressed = nextUncompressed = 0;

        //  Reset input/output buffer counters.  
        inputs = outputs = 0;

        //  Reset input/output buffer pointers.  
        nextInput = nextOutput = 0;

        //  Reset free read/write queue entry counters.  
        freeReads = inputRequests;
        freeWrites = outputRequests;

        //  Reset free queue entries pointers.  
        nextFreeRead = nextFreeWrite = 0;

        //  Reset counter of read/write inputs/outputs.  
        readInputs = writeOutputs = 0;

        //  Reset pointers to the read/write inputs/outputs.  
        nextRead = nextWrite = 0;

        //  Reset number of input requests requested to memory.  
        inputsRequested = 0;

        //  Reset the number of blocks being uncompressed.  
        uncompressing = 0;

        //  Reset the number of read queue entries writing to a cache line.  
        readsWriting = 0;

        //  Unset memory read and memory write flags.  
        memoryRead = memoryWrite = FALSE;

        //  Unset reading and writing line flags.  
        readingLine = writingLine = FALSE;

        //  No cache request on process right now.  
        cacheRequest = NULL;

        //  Reset updated HZ block flag.  
        updatedHZ = FALSE;

        //  Disable flush mode.  
        flushMode = FALSE;

        //  No request to flush received.  
        flushRequest = FALSE;

        //  End of the reset mode.  
        resetMode = FALSE;
    }

    /*  NOTE:  TO BE IMPLEMENTED WHEN THE CLASS IS CONVERTED TO A
        BOX AND COMMUNICATION GOES THROUGH SIGNALS.  */

    //  Process cache commands.  
    //  Process cache reads.  
    //  Process cache writes.  

    //  Unset fetch/allocate performed in current cycle flag.  
    fetchPerformed = false;

    //  Update read port cycle counters.  
    for(i = 0; i < readPorts; i++)
    {
        //  Check if there is line being read from the cache.  
        if (readCycles[i] > 0)
        {
            GPU_DEBUG(
                printf("ZCache => Remaining cache read cycles %d.\n", readCycles[i]);
            )

            //  Update read cycles counter.  
            readCycles[i]--;

            //  Check if read has finished.  
            if (readCycles[i] == 0)
            {
                //  Reset pointer to the next read port to the first free read port.  
                if (i < nextReadPort)
                    nextReadPort = i;

                GPU_DEBUG(
                    printf("ZCache => Cache read end.\n");
                )

                //  Check if a line was being read.  
                if (readingLine && (readLinePort == i))
                {
                    //  Update number of write blocks read from the cache.  
                    writeOutputs++;

                    //  Unset reading line flag.  
                    readingLine = FALSE;
                }
            }
        }
        else
        {
            //  Reset pointer to the next read port to the first free read port.  
            if (i < nextReadPort)
                nextReadPort = i;
        }
    }

    //  Update write port cycle counters.  
    for(i = 0; i < writePorts; i++)
    {
        //  Check if there is a line being written to the cache.  
        if (writeCycles[i] > 0)
        {
            GPU_DEBUG(
                printf("ZCache => Cache write cycles %d\n", writeCycles[i]);
            )

            //  Update write port cycles.  
            writeCycles[i]--;

            //  Check if write has finished.  
            if (writeCycles[i] == 0)
            {
                //  Reset pointer to the next write port to the first free write port.  
                if (i < nextWritePort)
                    nextWritePort = i;

                //  Check if there was a line being written.  
                if (writingLine && (writeLinePort == i))
                {
                    GPU_DEBUG(
                        printf("ZCache => End of cache write.\n");
                    )

                    //  Free fill cache request.  
                    cache->freeRequest(readQueue[nextUncompressed].requestID, FALSE, TRUE);

                    //  Clear cache request pointer.  
                    readQueue[nextUncompressed].request = NULL;

                    //  Update pointer to the next uncompressed block.  
                    nextUncompressed = GPU_MOD(nextUncompressed + 1, inputRequests);

                    //  Update free inputs counter.  
                    freeReads++;

                    //  Update the number of input requests writing to a cache line.  
                    readsWriting--;

                    //  Unset writing line flag.  
                    writingLine = FALSE;
                }
            }
        }
        else
        {
            //  Reset pointer to the next write port to the first free write port.  
            if (i < nextWritePort)
                nextWritePort = i;
        }
    }

    //  No memory transaction generated yet.  
    memTrans = NULL;

    //  Check if there is no cache request to process.  
    if (cacheRequest == NULL)
    {
        //  Receive request from the cache.  
        cacheRequest = cache->getRequest(requestID);
    }

    //  Check if there is a cache request being served.  
    if ((cacheRequest != NULL) && (freeWrites > 0) && (freeReads > 0))
    {

        //  Do not wait for a write request.  
        waitWrite = FALSE;

        //  Set read request that will be assigned to the fill request.  
        fillReadRequest = nextFreeRead;

        //  If it is a cache spill add to the write queue.  
        if (cacheRequest->spill)
        {
            GPU_DEBUG(
                printf("ZCache => Adding write (spill) request %d line (%d, %d) to address %x\n",
                    nextFreeWrite, cacheRequest->way, cacheRequest->line,
                    cacheRequest->outAddress);
            )

            GPU_ASSERT(
                /*  Search in the read request queue for the address to
                    write.  */
                for(i = 0, e = GPU_PMOD(nextFreeRead - 1, inputRequests); i < (inputs + readInputs + uncompressed);
                    i++, e = GPU_PMOD(e - 1, inputRequests))
                {
                    //  Check if it is the same line.  
                    if (readQueue[e].address == cacheRequest->outAddress)
                    {
                        printf("e %d i %d inputs %d readInputs %d uncompressed %d\n",
                            e, i, inputs, readInputs, uncompressed);
                        printf("nextInput %d nextRead %d nextUncompressed %d nextFreeRead %d\n",
                            nextInput, nextRead, nextUncompressed, nextFreeRead);
                        printf("Request address %x ReadQueue address %x\n",
                            cacheRequest->outAddress, readQueue[e].address);
                        CG_ASSERT("Writing a line not fully read from the cache.");
                    }
                }
            )


            //  Add to the next free write queue entry.  
            writeQueue[nextFreeWrite].address = cacheRequest->outAddress;
            writeQueue[nextFreeWrite].block = address2block(cacheRequest->outAddress);
            writeQueue[nextFreeWrite].size = 0;
            writeQueue[nextFreeWrite].written = 0;
            writeQueue[nextFreeWrite].request = cacheRequest;
            writeQueue[nextFreeWrite].requestID = requestID;

            //  Check if the request is also doing a fill.  
            if (cacheRequest->fill)
            {
                //  Set that the paired fill read request has a dependency with this write request.  
                writeQueue[nextFreeWrite].isReadWaiting = TRUE;

                //  Set the read queue entry waiting for this write request.  
                writeQueue[nextFreeWrite].readWaiting = fillReadRequest;
            }
            else
            {
                //  Only spill request.  No dependency with a read request.  
                writeQueue[nextFreeWrite].isReadWaiting = FALSE;
            }
        }

        /*  Add a read request to the queue if the cache request is
            a fill.  */
        if ((cacheRequest->fill) && (freeReads > 0))
        {
            GPU_DEBUG(
                printf("ZCache => Adding read (fill) request %d line (%d, %d) to address %x\n",
                    nextFreeRead, cacheRequest->way, cacheRequest->line,
                    cacheRequest->inAddress);
            )

            /*  Search in the write request queue for the address
                to read.  */
            for(i = 0, e = GPU_MOD(nextFreeWrite + freeWrites, outputRequests);
                ((i < (outputRequests - freeWrites)) && (!waitWrite));
                i++, e = GPU_MOD(e + 1, outputRequests))
            {
                //  Check if it is the same line.  
                if (writeQueue[e].address == cacheRequest->inAddress)
                {
                    //  Set wait write.  
                    waitWrite = TRUE;
                    GPU_DEBUG(
                        printf("ZCache => Read request address %x found at write queue entry %d.\n",
                            cacheRequest->inAddress, e);
                        printf("ZCache => Waiting for end of write request.\n");
                    )
                }
            }

            /*  NOTE: THIS IS A CLEARLY A CONFLICT MISS AND IT SHOULDN'T BE THAT
                FREQUENT UNLESS THE REPLACING POLICY IS NOT WORKING WELL.  */

            //  Check if the read request must wait to a write request to end.  
            if (!waitWrite)
            {
                //  Add to the next free read queue entry.  
                readQueue[nextFreeRead].address = cacheRequest->inAddress;
                readQueue[nextFreeRead].block = address2block(cacheRequest->inAddress);
                readQueue[nextFreeRead].size = 0;
                readQueue[nextFreeRead].requested = 0;
                readQueue[nextFreeRead].received = 0;
                readQueue[nextFreeRead].request = cacheRequest;
                readQueue[nextFreeRead].requestID = requestID;

                //  Set wait flag for a write queue to read from the same cache line.  
                readQueue[nextFreeRead].writeWait = cacheRequest->spill;

                //  Update inputs counter.  
                inputs++;

                //  Update free read queue entries counter.  
                freeReads--;

                //  Update pointer to the next free read queue entry.  
                nextFreeRead = GPU_MOD(nextFreeRead + 1, inputRequests);
            }
        }

        if (!waitWrite && cacheRequest->spill)
        {
            //  Update outputs counter.  
            outputs++;

            //  Update free write queue entries counter.  
            freeWrites--;

            //  Update pointer to the next free write queue entry.  
            nextFreeWrite = GPU_MOD(nextFreeWrite + 1, outputRequests);
        }

        //  Check if we could add the request to the queues.  
        if (!waitWrite)
        {
            //  Release the cache request.  
            cacheRequest = NULL;
        }
    }


    //  Check if there is a memory read transaction being received.  
    if (memoryCycles > 0)
    {
        GPU_DEBUG(
            printf("ZCache => Remaining memory cycles %d\n", memoryCycles);
        )

        //  Update memory bus busy cycles counter.  
        memoryCycles--;

        //  Check if transmission has finished.  
        if (memoryCycles == 0)
        {
            //  Check if it was a read transaction.  
            if(memoryRead)
            {
                //  Search ticket in the memory request table.  
                readEndReq = memoryRequest[GPU_MOD(readTicket, MAX_MEMORY_TICKETS)];

                GPU_DEBUG(
                    printf("ZCache => End of read transaction for read request queue %d\n",
                        readEndReq);
                )

                //  Update read queue entry.  
                readQueue[readEndReq].received += lastSize;

                //  Check if the full block was received.  
                if (readQueue[readEndReq].received == readQueue[readEndReq].size)
                {
                    GPU_DEBUG(
                        printf("ZCache => Input block %d fully read.\n",
                            readEndReq);
                    )

                    //  Update number of read input blocks.  
                    readInputs++;

                    //  Update the number of inputs requested to memory.  
                    inputsRequested--;

                }

                //  Unset read on progress flag.  
                memoryRead = FALSE;
            }

            //  Check if it was a write transaction.  
            if (memoryWrite)
            {
                GPU_DEBUG(
                    printf("ZCache => End of write transaction for write request queue %d\n",
                        nextCompressed);
                )

                //  Check if current block has been fully written.  
                if (writeQueue[nextCompressed].written == writeQueue[nextCompressed].size)
                {
                    GPU_DEBUG(
                        printf("ZCache => Output block %d fully written.\n",
                            nextUncompressed);
                    )

                    //  Free spill cache request.  
                    cache->freeRequest(writeQueue[nextCompressed].requestID, TRUE, FALSE);

                    //  Clear cache request pointer.  
                    writeQueue[nextCompressed].request = NULL;

                    //  Update pointer to next compressed block to write.  
                    nextCompressed = GPU_MOD(nextCompressed + 1, outputRequests);

                    //  Update number of compressed blocks.  
                    compressed--;

                    //  Update free outputs counter.  
                    freeWrites++;
                }

                //  Unset write on progress flag.  
                memoryWrite = FALSE;
            }

            //  Update number of free tickets.  
            freeTickets++;
        }
    }

    //  Request input blocks to memory.  
    if (inputs > 0)
    {
        //  Check compression state for the current block.  
        switch(blockState[readQueue[nextInput].block])
        {
            case Z_BLOCK_CLEAR:

                //  Block is cleared so it shouldn't be requested to memory.  

                GPU_DEBUG(
                    printf("ZCache => Input Request %d is for CLEAR block %d.\n",
                        nextInput, readQueue[nextInput].block);
                )

                //  Set block size and received bytes.  
                readQueue[nextInput].size = readQueue[nextInput].received = lineSize;

                //  Update pointer to next input.  
                nextInput = GPU_MOD(nextInput + 1, inputRequests);

                //  Update number of inputs.  
                inputs--;

                //  Update number of inputs read from memory.  
                readInputs++;

                break;

            case Z_BLOCK_UNCOMPRESSED:

                //  Block is uncompressed in memory.  Request the full block.  

                GPU_DEBUG(
                    printf("ZCache => Input Request %d is for UNCOMPRESSED block %d.\n",
                        nextInput, readQueue[nextInput].block);
                )

                //  If there is no previous request for the entry.  
                if (readQueue[nextInput].size == 0)
                {
                    //  Set uncompressed block size.  
                    readQueue[nextInput].size = lineSize;
                }

                //  Request data to memory.  
                memTrans = requestBlock(nextInput);

                //  Copy memory transaction cookies.  

                //  Send memory transaction to the memory controller.  

                //  Check if current input block has been fully requested.  
                if (readQueue[nextInput].requested == readQueue[nextInput].size)
                {

                    GPU_DEBUG(
                        printf("ZCache => Input request %d fully requested to memory.\n",
                            nextInput);
                    )

                    //  Update pointer to the next input.  
                    nextInput = GPU_MOD(nextInput + 1, inputRequests);

                    //  Update waiting inputs counter.  
                    inputs--;

                    //  Update the number of inputs requested to memory.  
                    inputsRequested++;
                }

                break;

            case Z_BLOCK_COMPRESSED_BEST:

                //  Request a best compression level compressed block.  

                GPU_DEBUG(
                    printf("ZCache => Input Request %d is for COMPRESSED BEST block %d.\n",
                        nextInput, readQueue[nextInput].block);
                )

                //  If there is no previous request for the entry.  
                if (readQueue[nextInput].size == 0)
                {
                    //  Set compressed best level block size.  
                    readQueue[nextInput].size = COMPRESSED_BLOCK_SIZE_BEST;
                }

                //  Request data to memory.  
                memTrans = requestBlock(nextInput);

                //  Copy memory transaction cookies.  

                //  Send memory transaction to the memory controller.  

                //  Check if current input block has been fully requested.  
                if (readQueue[nextInput].requested == readQueue[nextInput].size)
                {

                    GPU_DEBUG(
                        printf("ZCache => Input request %d fully requested to memory.\n",
                            nextInput);
                    )

                    //  Update pointer to the next input.  
                    nextInput = GPU_MOD(nextInput + 1, inputRequests);

                    //  Update waiting inputs counter.  
                    inputs--;

                    //  Update the number of inputs requested to memory.  
                    inputsRequested++;
                }

                break;

            case Z_BLOCK_COMPRESSED_NORMAL:

                //  Request a normal compression level compressed block.  

                GPU_DEBUG(
                    printf("ZCache => Input Request %d is for COMPRESSED NORMAL block %d.\n",
                        nextInput, readQueue[nextInput].block);
                )

                //  If there is no previous request for the entry.  
                if (readQueue[nextInput].size == 0)
                {
                    //  Set compressed normal level block size.  
                    readQueue[nextInput].size = COMPRESSED_BLOCK_SIZE_NORMAL;
                }

                //  Request data to memory.  
                memTrans = requestBlock(nextInput);

                //  Copy memory transaction cookies.  

                //  Send memory transaction to the memory controller.  

                //  Check if current input block has been fully requested.  
                if (readQueue[nextInput].requested == readQueue[nextInput].size)
                {

                    GPU_DEBUG(
                        printf("ZCache => Input request %d fullyrequested to memory.\n",
                            nextInput);
                    )

                    //  Update pointer to the next input.  
                    nextInput = GPU_MOD(nextInput + 1, inputRequests);

                    //  Update waiting inputs counter.  
                    inputs--;

                    //  Update the number of inputs requested to memory.  
                    inputsRequested++;
                }

                break;

            default:

                CG_ASSERT("Unsupported color block state mode.");
        }
    }

    //  Request output blocks to the cache.  
    if ((outputs > 0) && (!readingLine) &&  (nextReadPort < readPorts) && (readCycles[nextReadPort] == 0))
    {
        //  Try to read the cache line for the write request.  
        if (cache->readLine(writeQueue[nextOutput].request->way,
            writeQueue[nextOutput].request->line, outputBuffer[nextOutput]))
        {
            GPU_DEBUG(
                printf("ZCache => Reading output block %d from cache.\n",
                    nextOutput);
            )

            //  Check if it is a masked write.  
            if (writeQueue[nextOutput].request->masked)
            {
                //  Get the mask for the cache line. 
                cache->readMask(writeQueue[nextOutput].request->way,
                    writeQueue[nextOutput].request->line, maskBuffer[nextOutput]);
            }

            //  Set cycles until the line is fully received.  
            readCycles[nextReadPort] += (U32) ceil( (F32) lineSize/ (F32) portWidth);

            //  Set port the line read.  
            readLinePort = nextReadPort;

            //  Update pointer to the next read port.  
            nextReadPort++;

            //  Set reading line flag.  
            readingLine = TRUE;

            //  Signal waiting read request that the cache line has been already read.  
            if (writeQueue[nextOutput].isReadWaiting &&
                (readQueue[writeQueue[nextOutput].readWaiting].request == writeQueue[nextOutput].request))
            {
                //  Unset write wait flag for the read queue.  
                readQueue[writeQueue[nextOutput].readWaiting].writeWait = FALSE;
            }

            //  Update pointer to next output.  
            nextOutput = GPU_MOD(nextOutput + 1, outputRequests);

            //  Update write requests counter.  
            outputs--;
        }
    }

    //  Uncompress input blocks.  
    if (uncompressCycles > 0)
    {
        GPU_DEBUG(
            printf("ZCache => Remaining cycles to decompress block %d\n",
                uncompressCycles);
        )

        //  Update uncompress cycles.  
        uncompressCycles--;

        //  Check if uncompression has finished.  
        if (uncompressCycles == 0)
        {
            GPU_DEBUG(
                printf("ZCache => End of block decompression.\n");
            )

            //  Update number of uncompressed inputs.  
            uncompressed++;

            //  Update the number of blocks being uncompressed.  
            uncompressing--;
        }
    }

    //  Check if there are blocks to uncompress and the uncompressor is ready.  
    if ((readInputs > 0) && (uncompressCycles == 0))
    {
        //  Check if the next read input has been fully received from memory.  
        if (readQueue[nextRead].size == readQueue[nextRead].received)
        {
            //  Set uncompression cycles.  
            uncompressCycles = decomprLatency;

            //  Check block state.  
            switch(blockState[readQueue[nextRead].block])
            {
                case Z_BLOCK_CLEAR:

                    GPU_DEBUG(
                        printf("ZCache => Filling with clear color input request %d.\n",
                            nextRead);
                    )

//printf("ZCache -> clear block -> clearStencil %x clearDepth %x clear %x\n", clearStencil, clearDepth, (clearStencil << 24) | (clearDepth & 0x00ffffff));
                    //  Fill the block/line with the clear color.  
                    for(i = 0; i < lineSize; i += 4)
                        *((U32 *) &inputBuffer[nextRead][i]) = (clearStencil << 24) | (clearDepth & 0x00ffffff);

                    break;

                case Z_BLOCK_UNCOMPRESSED:

                    //  Nothing to do.  

                    GPU_DEBUG(
                        printf("ZCache => Uncompressed block for input request %d.\n",
                            nextRead);
                    )

                    break;

                case Z_BLOCK_COMPRESSED_BEST:

                    //  Nothing to do.  

                    GPU_DEBUG(
                        printf("ZCache => Compressed best block for input request %d.\n",
                            nextRead);
                    )

                    //  Copy the compressed input to the compression buffer.  
                    memcpy(comprBuffer, inputBuffer[nextRead], COMPRESSED_BLOCK_SIZE_BEST);

                    //  Uncompress the block.  
                    bmoFragmentOperator::hiloUncompress(comprBuffer, (U32 *) inputBuffer[nextRead],
                        lineStamps * 4, bmoFragmentOperator::COMPR_L1, COMPRESSION_HIMASK_NORMAL,
                        COMPRESSION_HIMASK_BEST, COMPRESSION_LOSHIFT_NORMAL,
                        COMPRESSION_LOSHIFT_BEST);

                    break;

                case Z_BLOCK_COMPRESSED_NORMAL:

                    //  Nothing to do.  

                    GPU_DEBUG(
                        printf("ZCache => Compressed normal block for input request %d.\n",
                            nextRead);
                    )

                    //  Copy the compressed input to the compression buffer.  
                    memcpy(comprBuffer, inputBuffer[nextRead], COMPRESSED_BLOCK_SIZE_NORMAL);

                    //  Uncompress the block.  
                    bmoFragmentOperator::hiloUncompress(comprBuffer, (U32 *) inputBuffer[nextRead],
                        lineStamps * 4, bmoFragmentOperator::COMPR_L0, COMPRESSION_HIMASK_NORMAL,
                        COMPRESSION_HIMASK_BEST, COMPRESSION_LOSHIFT_NORMAL,
                        COMPRESSION_LOSHIFT_BEST);

                    break;

                default:

                    CG_ASSERT("Unsupported block mode.");
                    break;
            }

            //  Update pointer to the input to decompress.  
            nextRead = GPU_MOD(nextRead + 1, inputRequests);

            //  Update number of inputs to decompress.  
            readInputs--;

            //  Update the number of blocks being uncompressed.  
            uncompressing++;
        }
    }

    //  Compress output blocks.  
    if (compressCycles > 0)
    {
        GPU_DEBUG(
            printf("ZCache => Block compression remaining cycles %d\n",
                compressCycles);
        )

        //  Update compress cycles.  
        compressCycles--;

        //  Check if compression has finished.  
        if (compressCycles == 0)
        {
            GPU_DEBUG(
                printf("ZCache => Block compression end.\n");
            )

            //  Update number of compressed outputs.  
            compressed++;
        }
    }

    //  Check if there are blocks to compress and the compressor is ready.  
    if ((writeOutputs > 0) && (compressCycles == 0))
    {
        GPU_DEBUG(
            printf("ZCache => Compressing block for output request %d\n",
                nextWrite);
        )

        //  Set compression cycles.  
        compressCycles = comprLatency;

        //  For clear blocks the non masked positions must be written too with the clear color.  
        if ((blockState[writeQueue[nextWrite].block] == Z_BLOCK_CLEAR) &&
            (writeQueue[nextWrite].request->masked))
        {
            //  Clear masked pixels  
            for(i = 0; i < lineSize; i += 4)
            {
                //  Write clear stencil and depth values on unmasked writes.  
                *((U32 *) &outputBuffer[nextWrite][i]) =
                    ((*((U32 *) &outputBuffer[nextWrite][i])) & maskBuffer[nextWrite][i >> 2]) |
                    (((clearStencil << 24) | (clearDepth & 0x00ffffff)) & ~maskBuffer[nextWrite][i >> 2]);

                //  Set pixel mask to true.  
                maskBuffer[nextWrite][i >> 2] = 0xffffffff;
            }
        }

        //  Calculate the min and maximum block and Z values.  
        bmoFragmentOperator::blockMinMaxZ((U32 *) outputBuffer[nextWrite], lineStamps * 4, minZ, maxZ, min, max);

        //  Set Hierarchical Z update values.  
        updatedHZ = TRUE;
        hzBlock = writeQueue[nextWrite].block;
        hzBlockZ = maxZ;

        //  Check if it is an uncompressed block.  
        if ((!disableCompr) && (blockState[writeQueue[nextWrite].block] != Z_BLOCK_UNCOMPRESSED))
        {
            //  Copy uncompressed output to the compression buffer.  
            memcpy(comprBuffer, outputBuffer[nextWrite], UNCOMPRESSED_BLOCK_SIZE);

            //  Compress block/line.  
            comprMode = bmoFragmentOperator::hiloCompress((U32 *) comprBuffer, outputBuffer[nextWrite],
                lineStamps * 4, COMPRESSION_HIMASK_NORMAL, COMPRESSION_HIMASK_BEST,
                COMPRESSION_LOSHIFT_NORMAL, COMPRESSION_LOSHIFT_BEST, COMPRESSION_LOMASK_NORMAL,
                COMPRESSION_LOMASK_BEST, min, max);
        }
        else
        {
            //  Do no recompress uncompressed blocks (not read in write only mode!!!).  
            comprMode = bmoFragmentOperator::UNCOMPRESSED;
        }


        //  Select compression method.  
        switch(comprMode)
        {
            case bmoFragmentOperator::UNCOMPRESSED:

                GPU_DEBUG(
                    printf("ZCache => Block compressed as UNCOMPRESSED.\n");
                )

                //  Set new block state.  
                blockState[writeQueue[nextWrite].block] = Z_BLOCK_UNCOMPRESSED;

                // Set compressed block size.  
                writeQueue[nextWrite].size = lineSize;

                break;

            case bmoFragmentOperator::COMPR_L0:

                GPU_DEBUG(
                    printf("ZCache => Block compressed as COMPRESSED NORMAL.\n");
                )

                //  Set new block state.  
                blockState[writeQueue[nextWrite].block] = Z_BLOCK_COMPRESSED_NORMAL;

                // Set compressed block size.  
                writeQueue[nextWrite].size = COMPRESSED_BLOCK_SIZE_NORMAL;

                //  Compressed blocks are not masked.  
                writeQueue[nextWrite].request->masked = FALSE;

                break;

            case bmoFragmentOperator::COMPR_L1:

                GPU_DEBUG(
                    printf("ZCache => Block compressed as COMPRESSED BEST.\n");
                )

                //  Set new block state.  
                blockState[writeQueue[nextWrite].block] = Z_BLOCK_COMPRESSED_BEST;

                // Set compressed block size.  
                writeQueue[nextWrite].size = COMPRESSED_BLOCK_SIZE_BEST;

                //  Compressed blocks are not masked.  
                writeQueue[nextWrite].request->masked = FALSE;

                break;

            default:
                CG_ASSERT("Unsupported compression mode.");
                break;
        }


        //  Update pointer to the next output to compress.  
        nextWrite = GPU_MOD(nextWrite + 1, outputRequests);

        //  Update number of write blocks to compress.  
        writeOutputs--;
    }

    //  Write compressed blocks to memory.  
    if (compressed > 0)
    {
        //  Check if a memory transaction has been already generated.  
        if (memTrans == NULL)
        {
            GPU_DEBUG(
                printf("ZCache => Writing output request %d to memory.\n",
                    nextCompressed);
            )

            //  Send write to memory.  
            memTrans = writeBlock(nextCompressed);

            //  If a transaction has been generated.  

            //  Send transaction to the memory controller.  
        }
    }

    //  Write uncompressed block/line to the cache.  
    if ((uncompressed > 0) && (!writingLine) && (nextWritePort < writePorts) && (writeCycles[nextWritePort] == 0))
    {
        //  Check if the read request must wait before writing the cache line.  
        if (!readQueue[nextUncompressed].writeWait)
        {
            //  Try to write the cache line.  
            if (cache->writeLine(readQueue[nextUncompressed].request->way,
                readQueue[nextUncompressed].request->line,
                inputBuffer[nextUncompressed]))
            {
                GPU_DEBUG(
                    printf("ZCache => Writing input request %d to cache\n",
                        nextUncompressed);
                )

                //  Update number of uncompressed blocks.  
                uncompressed--;

                //  Update number of input requests writing to a cache line.  
                readsWriting++;

                //  Set cache write port cycles.  
                writeCycles[nextWritePort] += (U32) ceil((F32) lineSize / (F32) portWidth);

                //  Set port for the line write.  
                writeLinePort = nextWritePort;

                //  Update pointer to the next write port.  
                nextWritePort++;

                //  Set writing line flag.  
                writingLine = TRUE;
            }
        }
    }

    //  Set the new transaction generated.  
    nextTransaction = memTrans;

}


//  Requests a compressed block/line of data.  
MemoryTransaction *ZCache::requestBlock(U32 readReq)
{
    U32 size;
    U32 offset;
    MemoryTransaction *memTrans;

    /*  Check if there is no write or read transaction in progress
        (bus busy), there are free memory tickets and the memory controller
        accepts read requests.  */
    if ((!memoryWrite) && (!memoryRead) && (freeTickets > 0)
        && ((memoryState & MS_READ_ACCEPT) != 0))
    {
        //  Calculate transaction size.  
        size = GPU_MIN(MAX_TRANSACTION_SIZE,
            readQueue[readReq].size - readQueue[readReq].requested);

        //  Get parameters.  
        offset = readQueue[readReq].requested;

        //  Keep the transaction ticket associated with the request.  
        memoryRequest[GPU_MOD(nextTicket, MAX_MEMORY_TICKETS)] = readReq;

        //  Create the new memory transaction.  
        memTrans = new MemoryTransaction(
            MT_READ_REQ,
            readQueue[readReq].address + offset,
            size,
            &inputBuffer[readReq][offset],
            ZSTENCILTEST,
            cacheID,
            nextTicket++);

        //  If there is a Dynamic Object as request source copy its cookies.  
        if (readQueue[readReq].request->source != NULL)
        {
            memTrans->copyParentCookies(*readQueue[readReq].request->source);
            memTrans->addCookie();
        }

        GPU_DEBUG(
            printf("ZCache => MT_READ_REQ addr %x size %d.\n",
                readQueue[readReq].address + offset, size);
        )

        //  Update requested block bytes counter.  
        readQueue[readReq].requested += size;

        //  Update number of free tickets.  
        freeTickets--;
    }
    else
    {
        //  No memory transaction generated.  
        memTrans = NULL;
    }

    return memTrans;
}

//  Requests a compressed block/line of data.  
MemoryTransaction *ZCache::writeBlock(U32 writeReq)
{
    U32 size;
    U32 offset;
    MemoryTransaction *memTrans;

    /*  Check if there is no write or read transaction in progress
        (bus busy), there are free memory tickets and the memory controller
        accepts read requests.  */
    if ((!memoryWrite) && (!memoryRead) && (freeTickets > 0)
        && ((memoryState & MS_WRITE_ACCEPT) != 0))
    {
        //  Calculate transaction size.  
        size = GPU_MIN(MAX_TRANSACTION_SIZE,
            writeQueue[writeReq].size - writeQueue[writeReq].written);

        //  Get parameters.  
        offset = writeQueue[writeReq].written;

        /*  NOTE:  MASKED WRITES CAN ONLY BE USED WITH UNCOMPRESSED
            BLOCKS.  AS THE CURRENT Z CACHE DOES NOT SUPPORT
            COMPRESSED BLOCKS (OTHER THAN CLEAR, THAT ARE NEVER
            WRITTEN) THIS IS ALLOWED.

         */

        //  Check if the cache line is a masked write.  
        if (writeQueue[writeReq].request->masked)
        {
            //  Masked write.  

            //  Create the new memory transaction.  
            memTrans = new MemoryTransaction(
                writeQueue[writeReq].address + offset,
                size,
                &outputBuffer[writeReq][offset],
                &maskBuffer[writeReq][offset >> 2],
                ZSTENCILTEST,
                cacheID,
                nextTicket++);

            GPU_DEBUG(
                printf("ZCache => MT_WRITE_DATA (masked) addr %x size %d.\n",
                   writeQueue[writeReq].address + offset, size);
            )

        }
        else
        {
            //  Unmasked write.  

            //  Create the new memory transaction.  
            memTrans = new MemoryTransaction(
                MT_WRITE_DATA,
                writeQueue[writeReq].address + offset,
                size,
                &outputBuffer[writeReq][offset],
                ZSTENCILTEST,
                cacheID,
                nextTicket++);

            GPU_DEBUG(
                printf("ZCache => MT_WRITE_DATA addr %x size %d.\n",
                   writeQueue[writeReq].address + offset, size);
            )
        }

        //  If there is a Dynamic Object as request source copy its cookies.  
        if (writeQueue[writeReq].request->source != NULL)
        {
            memTrans->copyParentCookies(*writeQueue[writeReq].request->source);
            memTrans->addCookie();
        }

        //  Update requested block bytes counter.  
        writeQueue[writeReq].written += size;

        //  Update number of free tickets.  
        freeTickets--;

        //  Set write in progress flag.  
        memoryWrite = TRUE;

        //  Set memory write cycles.  
        memoryCycles = memTrans->getBusCycles();
    }
    else
    {
        //  No memory transaction generated.  
        memTrans = NULL;
    }

    return memTrans;
}

//  Copies the block state memory.  
void ZCache::copyBlockStateMemory(ZBlockState *buffer, U32 blocks)
{
    CG_ASSERT_COND(!(blocks > maxBlocks), "More blocks to copy than blocks in the state memory.");
    //  Copy the block states.  
    memcpy(buffer, blockState, sizeof(ZBlockState) * blocks);
}

//  Translates an address to the start of a block into a block number.  
U32 ZCache::address2block(U32 address)
{
    U32 block;

    //  Translate start block/line address to a block address.  
    block = (address - zBufferAddress) >> blockShift;

    return block;
}


