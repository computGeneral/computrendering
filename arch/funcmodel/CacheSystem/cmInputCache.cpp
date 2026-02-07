/**************************************************************************
 *
 * Input Cache class implementation file.
 *
 */

/**
 *
 * @file InputCache.cpp
 *
 * Implements the Input Cache class.  This class implements the cache used to
 * retrieve vertex input data from memory in the cmoStreamController unit.
 *
 */

#include "cmInputCache.h"
#include "GPUMath.h"

using cg1gpu::tools::Queue;

using namespace cg1gpu;

//  Input cache constructor.  
InputCache::InputCache(U32 cacheId, U32 ways, U32 lines, U32 bytesLine,
        U32 ports, U32 pWidth, U32 reqQSize, U32 inReqs) :

    cacheId(cacheId), numPorts(ports), portWidth(pWidth), inputRequests(inReqs), lineSize(bytesLine)

{
    U32 i;


    //  Create the fetch cache object.  
    cache = new FetchCache(ways, lines, lineSize, reqQSize, "InC");

    //  Create the input buffer.  
    inputBuffer = new U08*[inputRequests];

    //  Check allocation.  
    CG_ASSERT_COND(!(inputBuffer == NULL), "Error allocating input buffers.");
    //  Create read request queue.  
    readQueue = new InputCacheReadRequest[inputRequests];

    //  Check allocation.  
    CG_ASSERT_COND(!(readQueue == NULL), "Error allocating the read queue.");
    //  Allocate individual buffers.  
    for(i = 0; i < inputRequests; i++)
    {
        //  Allocate input buffer.  
        inputBuffer[i] = new U08[lineSize];

        //  Check allocation.  
        CG_ASSERT_COND(!(inputBuffer[i] == NULL), "Error allocating input buffer.");    }

    //  Allocate read/write port cycle counters.  
    readCycles = new U32[numPorts];

    //  Check allocation.  
    CG_ASSERT_COND(!(readCycles == NULL), "Error allocating the per port read cycles counter.");
    //  Create statistics.  
    //readTrans = &gpuStatistics::StatisticsManager::instance().getNumericStatistic("ReadTransactions", U32(0), "InputCache", "InC");
    //writeTrans = &gpuStatistics::StatisticsManager::instance().getNumericStatistic("WriteTransactions", U32(0), "InputCache", "InC");
    //readMem = &gpuStatistics::StatisticsManager::instance().getNumericStatistic("ReadBytesMem", U32(0), "InputCache", "InC");
    //writeMem = &gpuStatistics::StatisticsManager::instance().getNumericStatistic("WriteBytesMem", U32(0), "InputCache", "InC");
    //fetches = &gpuStatistics::StatisticsManager::instance().getNumericStatistic("Fetches", U32(0), "InputCache", "InC");
    //fetchOK = &gpuStatistics::StatisticsManager::instance().getNumericStatistic("FetchesOK", U32(0), "InputCache", "InC");
    //fetchFail = &gpuStatistics::StatisticsManager::instance().getNumericStatistic("FetchesFail", U32(0), "InputCache", "InC");
    //readOK = &gpuStatistics::StatisticsManager::instance().getNumericStatistic("ReadsOK", U32(0), "InputCache", "InC");
    //readFail = &gpuStatistics::StatisticsManager::instance().getNumericStatistic("ReadsFail", U32(0), "InputCache", "InC");
    //writeOK = &gpuStatistics::StatisticsManager::instance().getNumericStatistic("WritesOK", U32(0), "InputCache", "InC");
    //writeFail = &gpuStatistics::StatisticsManager::instance().getNumericStatistic("WritesFail", U32(0), "InputCache", "InC");

    //  Reset the input cache.  
    resetMode = TRUE;

    //  Set the ticket list to the maximum number of memory tickets.
    ticketList.resize(MAX_MEMORY_TICKETS);
    
    string queueName;
    queueName.clear();
    queueName = "ReadTicketQueue-InCache";
    ticketList.setName(queueName);    
}


//  Fetches a cache line.  
bool InputCache::fetch(U32 address, U32 &way, U32 &line, DynamicObject *source)
{
    //  Update statistics.  
    //fetches->inc();

    //  Try to fetch the address in the Fetch Cache.  
    return cache->fetch(address, way, line, source);
}

//  Reads vertex data.  
bool InputCache::read(U32 address, U32 way, U32 line, U32 size,
    U08 *data)
{
    //  Search for the next free port.  
    for(; (nextReadPort < numPorts) && (readCycles[nextReadPort] > 0); nextReadPort++);

    //  Determine if there is a free read port.  
    if (nextReadPort == numPorts)
    {
        //  Update statistics.  
        //readFail->inc();

        return FALSE;
    }
    else
    {
        //  Check if cache read port is busy.  
        GPU_ASSERT(
            if (readCycles[nextReadPort] > 0)
            {
                CG_ASSERT("Selected read port is busy.");
                //  Update statistics.  
                //readFail->inc();

                //return FALSE;
            }
        )

        //  Try to read fetched data from the Fetch Cache.  
        if (cache->read(address, way, line, size, data))
        {
            //  Update read cycles counter.  
            readCycles[nextReadPort] += (U32) ceil((F32) size / (F32) portWidth);

            //  Update pointer to the next read port.  
            nextReadPort++;

            //  Update statistics.  
            //readOK->inc();

            return TRUE;
        }
        else
        {
            //  Update statistics.  
            //readFail->inc();

            return FALSE;
        }
    }
}

//  Unreserves a cache line after use.  
void InputCache::unreserve(U32 way, U32 line)
{
    //  Unreserve fetch cache line.  
    cache->unreserve(way, line);
}

//  Resets the input cache structures.  
void InputCache::reset()
{
    //  Reset the color cache.  
    resetMode = TRUE;
}

//  Process a memory transaction.  
void InputCache::processMemoryTransaction(MemoryTransaction *memTrans)
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
                printf("InputCache => MT_READ_DATA ticket %d address %x size %d\n",
                    readTicket, memTrans->getAddress(), lastSize);
            )

            //  Set cycles the memory bus will be in use.  
            memoryCycles = memTrans->getBusCycles();

            //  Set memory read in progress flag.  
            memoryRead = TRUE;

            //  Update statistics.  
            //readTrans->inc();
            //readMem->inc(memTrans->getSize());

            break;

        default:

            CG_ASSERT("Unsopported transaction type.");
            break;
    }
}


//  Update the memory request queue.  Generate new memory transactions.  
MemoryTransaction *InputCache::update(U64 cycle, MemState memState)
{
    //  Set memory controller state.  
    memoryState = memState;

    //  Simulate a input cache cycle.  
    clock(cycle);

    //  Return the generated (if any) memory transaction.  
    return nextTransaction;
}


//  Input cache simulation rutine.  
void InputCache::clock(U64 cycle)
{
    U32 i;
    U32 readEndReq;
    MemoryTransaction *memTrans;

    GPU_DEBUG(
        printf("InputCache => Cycle %lld\n", cycle);
    )

    //  Receive memory transactions and state from Memory Controller.  

    //  Check if the input cache is in reset mode.  
    if (resetMode)
    {
        GPU_DEBUG(
            printf("InputCache => Reset.\n");
        )

        //  Reset the cache.  
        cache->reset();

        //  Reset free memory tickets counter.  
        freeTickets = MAX_MEMORY_TICKETS;

        //  Fill the ticket list.
        ticketList.clear();
        for(U32 i = 0; i < MAX_MEMORY_TICKETS; i++)
            ticketList.add(i);

        //  Reset memory bus cycles.  
        memoryCycles = 0;

        //  Reset cache write port cycles.  
        writeCycles = 0;

        //  Reset cache read port cycles.  
        for(U32 i = 0; i < numPorts; i++)
            readCycles[i] = 0;

        //  Reset pointer to next read port.  
        nextReadPort = 0;

        //  Reset input buffer counters.  
        inputs = 0;

        //  Reset input buffer pointers.  
        nextInput = 0;

        //  Reset free read queue entry counters.  
        freeReads = inputRequests;

        //  Reset free queue entries pointers.  
        nextFreeRead = 0;

        //  Reset counter of read inputs.  
        readInputs = 0;

        //  Reset pointers to the read inputs.  
        nextRead = 0;

        //  Reset number of input requests requested to memory.  
        inputsRequested = 0;

        //  Reset the number of read queue entries writing to a cache line.  
        readsWriting = 0;

        //  Unset memory read flags.  
        memoryRead = FALSE;

        //  Unset writing line flags.  
        writingLine = FALSE;

        //  No cache request on process right now.  
        cacheRequest = NULL;

        //  End of the reset mode.  
        resetMode = FALSE;
    }

    /*  NOTE:  TO BE IMPLEMENTED WHEN THE CLASS IS CONVERTED TO A
        BOX AND COMMUNICATION GOES THROUGH SIGNALS.  */

    //  Process cache commands.  
    //  Process cache reads.  
    //  Process cache writes.  

    //  Update the read ports cycle counters.  
    for(i = 0; i < numPorts; i++)
    {
        //  Check if there is line being read from the cache.  
        if (readCycles[i] > 0)
        {
            GPU_DEBUG(
                printf("InputCache => Remaining cache read cycles %d.\n",
                    readCycles);
            )

            //  Update read cycles counter.  
            readCycles[i]--;

            //  Check if read has finished.  
            if (readCycles[i] == 0)
            {
                GPU_DEBUG(
                    printf("InputCache => Cache read end.\n");
                )
            }
        }
    }

    //  Reset next read port pointer for this cycle.  
    nextReadPort = 0;

    //  Check if there is a line being written to the cache.  
    if (writeCycles > 0)
    {
        GPU_DEBUG(
            printf("InputCache => Cache write cycles %d\n",
                writeCycles);
        )

        //  Update write port cycles.  
        writeCycles--;

        //  Check if write has finished.  
        if (writeCycles == 0)
        {
            //  Check if there was a line being written.  
            if (writingLine)
            {
                GPU_DEBUG(
                    printf("InputCache => End of cache write.\n");
                )

                //  Free fill cache request.  
                cache->freeRequest(readQueue[nextRead].requestID, FALSE, TRUE);

                //  Clear cache request pointer.  
                readQueue[nextRead].request = NULL;

                //  Update pointer to the next read request.  
                nextRead = GPU_MOD(nextRead + 1, inputRequests);

                //  Update free inputs counter.  
                freeReads++;

                //  Update the number of input requests writing to a cache line.  
                readsWriting--;

                //  Unset writing line flag.  
                writingLine = FALSE;
            }
        }
    }

//if (GPU_MOD(cycle, 1000) == 0)
//printf("InC %lld => freeReads %d inputs %d readInputs %d inputsRequested %d readsWriting %d\n",
//cycle, freeReads, inputs, readInputs, inputsRequested, readsWriting);

    //  No memory transaction generated yet.  
    memTrans = NULL;

    //  Check if there is no cache request to process.  
    if (cacheRequest == NULL)
    {
        //  Receive request from the cache.  
        cacheRequest = cache->getRequest(requestID);
    }

    //  Check if there is a cache request being served.  
    if ((cacheRequest != NULL) && (freeReads > 0))
    {
        /*  Add a read request to the queue if the cache request is
            a fill.  */
        if ((cacheRequest->fill) && (freeReads > 0))
        {
            GPU_DEBUG(
                printf("InputCache => Adding read (fill) request %d line (%d, %d) to address %x\n",
                    nextFreeRead, cacheRequest->way, cacheRequest->line,
                    cacheRequest->inAddress);
            )

            //  Add to the next free read queue entry.  
            readQueue[nextFreeRead].address = cacheRequest->inAddress;
            readQueue[nextFreeRead].size = 0;
            readQueue[nextFreeRead].requested = 0;
            readQueue[nextFreeRead].received = 0;
            readQueue[nextFreeRead].request = cacheRequest;
            readQueue[nextFreeRead].requestID = requestID;

            //  Update inputs counter.  
            inputs++;

            //  Update free read queue entries counter.  
            freeReads--;

            //  Update pointer to the next free read queue entry.  
            nextFreeRead = GPU_MOD(nextFreeRead + 1, inputRequests);
        }

        //  Release the cache request.  
        cacheRequest = NULL;
    }

    //  Check if there is a memory read transaction being received.  
    if (memoryCycles > 0)
    {
        GPU_DEBUG(
            printf("InputCache => Remaining memory cycles %d\n", memoryCycles);
        )

        //  Update memory bus busy cycles counter.  
        memoryCycles--;

        //  Check if transmission has finished.  
        if (memoryCycles == 0)
        {
            //  Search ticket in the memory request table.  
            readEndReq = memoryRequest[readTicket];

            GPU_DEBUG(
                printf("InputCache => End of read transaction for read request queue %d\n",
                    readEndReq);
            )
         
            //  Update read queue entry.  
            readQueue[readEndReq].received += lastSize;

            //  Check if the full block was received.  
            if (readQueue[readEndReq].received == readQueue[readEndReq].size)
            {
                GPU_DEBUG(
                     printf("InputCache => Input block %d fully read.\n",
                        readEndReq);
                )

                //  Update number of read input blocks.  
                readInputs++;

                //  Update the number of inputs requested to memory.  
                inputsRequested--;

            }
            
            //  Add ticket to the ticket list.
            ticketList.add(readTicket);

            //  Update number of free tickets.  
            freeTickets++;

            //  Unset memory read on progress flag.  
            memoryRead = FALSE;
        }
    }

    //  Request input blocks to memory.  
    if (inputs > 0)
    {
        GPU_DEBUG(
            printf("InputrCache => Requesting to memory Input Request %d.\n",
                nextInput);
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
                printf("InputCache => Input request %d fully requested to memory.\n",
                    nextInput);
            )

            //  Update pointer to the next input.  
            nextInput = GPU_MOD(nextInput + 1, inputRequests);

            //  Update waiting inputs counter.  
            inputs--;

            //  Update the number of inputs requested to memory.  
            inputsRequested++;
        }
    }

    //  Write line to the cache.  
    if ((readInputs > 0) && (writeCycles == 0))
    {
        //  Check if the next read input has been fully received from memory.  
        if (readQueue[nextRead].size == readQueue[nextRead].received)
        {
            //  Try to write the cache line.  
            if (cache->writeLine(readQueue[nextRead].request->way,
                readQueue[nextRead].request->line,
                inputBuffer[nextRead]))
            {
                GPU_DEBUG(
                    printf("InputCache => Writing input request %d to cache\n",
                        nextRead);
                    )

                //  Update number of read input requests.  
                readInputs--;

                //  Update number of input requests writing to a cache line.  
                readsWriting++;

                //  Set cache write port cycles.  
                writeCycles += (U32) ceil((F32) lineSize / (F32) (numPorts * portWidth));

                //  Set writing line flag.  
                writingLine = TRUE;
            }
        }
    }

    //  Set the new transaction generated.  
    nextTransaction = memTrans;

}


//  Requests a line of data.  
MemoryTransaction *InputCache::requestBlock(U32 readReq)
{
    U32 size;
    U32 offset;
    MemoryTransaction *memTrans;

    /*  Check if there is no write or read transaction in progress
        (bus busy), there are free memory tickets and the memory controller
        accepts read requests.  */
    if ((!memoryRead) && (freeTickets > 0)
        && ((memoryState & MS_READ_ACCEPT) != 0))
    {
        //  Calculate transaction size.  
        size = GPU_MIN(MAX_TRANSACTION_SIZE,
            readQueue[readReq].size - readQueue[readReq].requested);

        //  Get parameters.  
        offset = readQueue[readReq].requested;

        //  Get the next read ticket.
        U32 nextReadTicket = ticketList.pop();
        
        //  Keep the transaction ticket associated with the request.  
        memoryRequest[nextReadTicket] = readReq;

        //  Create the new memory transaction.  
        memTrans = new MemoryTransaction(
            MT_READ_REQ,
            readQueue[readReq].address + offset,
            size,
            &inputBuffer[readReq][offset],
            STREAMERLOADER,
            cacheId,
            nextReadTicket);

        //  Check if there is a DynamicObject as source of the cache request.  
        if (readQueue[readReq].request->source != NULL)
        {
            //  Copy cookies from the source object.  
            memTrans->copyParentCookies(*readQueue[readReq].request->source);
            memTrans->addCookie();
        }

        GPU_DEBUG(
            printf("InputCache => MT_READ_REQ addr %x size %d.\n",
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
