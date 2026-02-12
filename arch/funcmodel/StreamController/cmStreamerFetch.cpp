/**************************************************************************
 *
 * cmoStreamController Fetch class implementation file.
 *
 */

/**
 *
 *  @file StreamerFetch.cpp
 *
 *  This file implements the cmoStreamController Fetch mdu.
 *
 */

#include "cmStreamerFetch.h"
#include "cmStreamerStateInfo.h"
#include "support.h"
#include "GPUMath.h"

using namespace arch;

//  cmoStreamController Fetch constructor.  
StreamerFetch::StreamerFetch(U32 idxCycle, U32 idxBufferSize, U32 oFIFOSize,
        U32 OMSize, U32 IRQSize, U32 vertCycle, char **slPrefixArray, U32 streamLoadUnits,
        U32 slIdxCycle, char *name, cmoMduBase* parent) :

    indicesCycle(idxCycle), indexBufferSize(idxBufferSize), outputFIFOSize(oFIFOSize),
    outputMemorySize(OMSize), inputRequestQueueSize(IRQSize), verticesCycle(vertCycle),
    streamerLoaderUnits(streamLoadUnits), slIndicesCycle(slIdxCycle), cmoMduBase(name, parent)

{
    DynamicObject *defaultState[1];
    unsigned int i;

    //  Check structure sizes.  
    GPU_ASSERT(
        if(indicesCycle == 0)
            CG_ASSERT("At least one index to be fetched per cycle required.");
        if (indexBufferSize == 0)
            CG_ASSERT("Index buffer size can not be zero.");

        if (GPU_MOD(indexBufferSize, MAX_TRANSACTION_SIZE) != 0)
            CG_ASSERT("Index buffer size must be a multiple of the max memory transaction data size.");

        if (outputFIFOSize == 0)
            CG_ASSERT("Output FIFO size can not be zero.");

        if (outputMemorySize == 0)
            CG_ASSERT("Output Memory size can not be zero.");

        if (inputRequestQueueSize == 0)
            CG_ASSERT("Input Request Queue size can not be zero.");

        if(verticesCycle == 0)
            CG_ASSERT("At least one vertex to be deallocated per cycle required.");
        
        if (streamerLoaderUnits == 0)
            CG_ASSERT("At least one cmoStreamController Loader unit is required. ");

        if (indicesCycle != slIdxCycle * streamerLoaderUnits)
            CG_ASSERT("Indexes per cycle must equal overall indexes per cycle for the total cmoStreamController Loader units.");
    )

    //  Allocate index buffer memory.  
    indexBuffer = new U08[indexBufferSize];

    //  Check memory allocation.  
    CG_ASSERT_COND(!(indexBuffer == NULL), "Error allocating memory for the index buffer.");
    //  Initialize free entry counters.  
    freeOutputFIFOEntries = outputFIFOSize;
    freeOutputMemoryLines = outputMemorySize;

    freeIRQEntries = new U32[streamerLoaderUnits];

    //  Check memory allocation.  
    CG_ASSERT_COND(!(freeIRQEntries == NULL), "Error allocating memory for the array of free IRQ Entries.");
    for (i = 0; i < streamerLoaderUnits; i++)
    {
    	freeIRQEntries[i] = inputRequestQueueSize;
    }

    //  Allocate array of sent indices per cmoStreamController Loader Unit.   
    indicesSentThisCycle = new U32[streamerLoaderUnits];

    //  Check memory allocation.  
    CG_ASSERT_COND(!(indicesSentThisCycle == NULL), "Error allocating memory for the array of sent indices per cmoStreamController Loader Unit.");
    //  Initialize streaming state.  
    state = ST_RESET;
    indexedMode = TRUE;
    indexStreamAddress = 0;
    indexStreamData = SD_U32BIT;
    streamStart = 0;
    streamCount = 0;
    streamInstances = 1;

    //  Create the cmoStreamController Fetch signals.  

    //  Request signal to the Memory Controller.  
    streamerFetchMemReq = newOutputSignal("StreamerFetchMemoryRequest", 1, 1);

    //  Data signal from the Memory Controller.  
    streamerFetchMemData = newInputSignal("StreamerFetchMemoryData", 2, 1);

    //  Signal from the cmoStreamController main mdu.  
    streamerFetchCommand = newInputSignal("StreamerFetchCommand", 1, 1);

    //  State signal to the cmoStreamController main mdu.  
    streamerFetchState = newOutputSignal("StreamerFetchState", 1, 1);

    //  Create default state for the StreamController fetch state signal.  
    defaultState[0] = new StreamerStateInfo(ST_RESET);

    //  Set default state for the StreamController fetch state signal.  
    streamerFetchState->setData(defaultState);

    //  Signal to the StreamController output cache.  
    streamerOutputCache = newOutputSignal("StreamerOutputCache", indicesCycle, 1);

    //  Allocate Deallocation signals for cmoStreamController Loader units. 
    streamerLoaderDeAlloc = new Signal*[streamerLoaderUnits];
 
    //  Check memory allocation.  
    CG_ASSERT_COND(!(streamerLoaderDeAlloc == NULL), "Error allocating memory for the StreamController loader unit signals array."); 
    //  Deallocation signals from the cmoStreamController Loader units.  
    for (i = 0; i < streamerLoaderUnits; i++)
    {
        streamerLoaderDeAlloc[i] = newInputSignal("StreamerLoaderDeAlloc", 2 * slIdxCycle, 1, slPrefixArray[i]);
    }

    //  Deallocation signal from the StreamController commit.  
    streamerCommitDeAlloc = newInputSignal("StreamerCommitDeAlloc", 4 * verticesCycle, 1);

    //  Allocate the array of unconfirmed deallocated output memory lines from cmoStreamController Commit.  
    unconfirmedOMLineDeAllocs = new U32*[2];

    //  Check memory allocation.  
    CG_ASSERT_COND(!(unconfirmedOMLineDeAllocs == NULL), "Error allocating array of unconfirmed deallocated output memory lines from cmoStreamController Commit.");
    for (i = 0; i < 2; i++)
    {
         unconfirmedOMLineDeAllocs[i] = new U32[verticesCycle];

         //  Check memory allocation.  
         CG_ASSERT_COND(!(unconfirmedOMLineDeAllocs == NULL), "Error allocating array of unconfirmed deallocated output memory lines from cmoStreamController Commit.");    } 

    //  Allocate the unconfirmed deallocated output memory lines counters.  
    unconfirmedDeAllocCounters = new U32[2];

    //  Check memory allocation.  
    CG_ASSERT_COND(!(unconfirmedDeAllocCounters == NULL), "Error allocating counters of deallocated output memory lines from cmoStreamController Commit.");
    //  Set last StreamController command to a dummy StreamController command.  
    lastStreamCom = new StreamerCommand(STCOM_RESET);

    //  Reset free memory ticket counter.  
    freeTickets = MAX_MEMORY_TICKETS;

    //  Reset current memory ticket counter.  
    currentTicket = 0;

    //  Reset transmission cycles counter.  
    busCycles = 0;

    //  Create statistics.  
    bytesRead = &getSM().getNumericStatistic("ReadBytes", U32(0), "StreamerFetch", "StF");
    transactions = &getSM().getNumericStatistic("MemoryTransactions", U32(0), "StreamerFetch", "StF");
    sentIdx = &getSM().getNumericStatistic("IndexesSent", U32(0), "StreamerFetch", "StF");
}

//  Stremer Fetch simulation rutine.  
void StreamerFetch::clock(U64 cycle)
{
    StreamerCommand *streamCom;
    MemoryTransaction *memTrans;
    StreamerControlCommand *streamCCom;
    U32 newIndex;
    U32 size;
    U32 i;
    U32 request;
    U32 indexAddress;
    bool previousSent;
    U32 selectedSLUnit;
    U32 testedUnits;

    GPU_DEBUG_BOX(
        printf("StreamerFetch => Clock %lld\n", cycle);
    )

    //  Get memory transactions and state from Memory Controller.  
    while(streamerFetchMemData->read(cycle, (DynamicObject *&) memTrans))
    {
        //  Process memory transaction.  
        processMemoryTransaction(memTrans);
    }

    //  Check data transmission.  
    if (busCycles > 0)
    {
        //  Update transmission counter.  
        busCycles--;

        //  Check if end of transmission.  
        if (busCycles == 0)
        {
            //  Get reorder entry for the transaction.  
            request = memoryRequest[GPU_MOD(readTicket, MAX_MEMORY_TICKETS)];

            //  Store read data in reorder queue entry.  
            reorderQueue[request] = lastSize;

            //  Free memory ticket.  
            freeTickets++;

            //  Update statistics.  
            bytesRead->inc(lastSize);
        }
    }
/*
if ((cycle > 0) && (GPU_MOD(cycle, 1000) == 0))
{
printf("StF %lld => state ", cycle);
switch(state)
{
    case ST_RESET: printf("RESET\n"); break;
    case ST_READY: printf("READY\n"); break;
    case ST_STREAMING: printf("STREAMING\n"); break;
    case ST_FINISHED: printf("FINISHED\n"); break;

}

//printf("StF => requestedIndexData %d fetchedIndexes %d streamCount %d indexData %d requestedData %d\n",
//requestedIndexData, fetchedIndexes, streamCount, indexData, requestedData);
U32 accum = 0;

for (U32 count = 0; count < streamerLoaderUnits; count++) accum += freeIRQEntries[count];

printf("StF %lld => indexedMode %s fetchedIndexes %d freeIndexBuffer %d freeOutputFIFOEntries %d freeOutputMemoryLines %d totalFreeIRQEntries %d\n",
cycle, indexedMode?"T":"F", fetchedIndexes, (indexBufferSize - indexData), freeOutputFIFOEntries, freeOutputMemoryLines, accum);

}
*/
    //  Simulate current cycle.  
    switch(state)
    {
        case ST_RESET:

            //  Reset stream state.  

            GPU_DEBUG_BOX( printf("cmoStreamController => RESET state.\n"); )

            //  Set reset stream state.  

            //  Reset memory request table.  
            for(i = 0; i < MAX_MEMORY_TICKETS; i++)
                memoryRequest[i] = 0;

            //  Reset memory reorder queue.  
            for(i = 0; i < MAX_MEMORY_REQUESTS; i++)
                reorderQueue[i] = 0;

            //  Reset pointers and counters to the reorder queue.  
            nextCommit = 0;
            nextRequest = 0;
            activeRequests = 0;

            //  Initialize the unconfirmed deallocated OM line counters.  
            unconfirmedDeAllocCounters[0] = 0;
            unconfirmedDeAllocCounters[1] = 0;

            //  Change state to ready.  
            
            state = ST_READY;

            break;

        case ST_READY:

            //  Accepts stream start commands and stream state changes.  

            GPU_DEBUG_BOX( printf("StreamerFetch => READY state.\n"); )

            //  Get command from the cmoStreamController main mdu.  
            if (streamerFetchCommand->read(cycle, (DynamicObject *&) streamCom))
                processStreamerCommand(streamCom);

            break;

        case ST_STREAMING:

            //  Streaming a batch of vertex/inputs.  

            GPU_DEBUG_BOX( printf("StreamerFetch => STREAMING state.\n"); )

            //  Read signals from the stream loader units.  
            for (i = 0; i < streamerLoaderUnits; i++)
            {
                while(streamerLoaderDeAlloc[i]->read(cycle, (DynamicObject *&) streamCCom))
                {
                    GPU_DEBUG_BOX(
                        printf("StreamerFetch => Received DEALLOC from Loader.\n");
                    )

                    processStreamerControlCommand(cycle, streamCCom);
                }
            }

            //  Initialize the proper unconfirmed deallocated OM line counter.  
            unconfirmedDeAllocCounters[GPU_MOD(cycle, 2)] = 0;

            /*  Read signal from the stream commit.  In a single cycle can
                arrrive an Output FIFO DeAlloc and an Output Memory DeAlloc.  */
            while(streamerCommitDeAlloc->read(cycle, (DynamicObject *&) streamCCom))
            {
                GPU_DEBUG_BOX(
                    printf("StreamerFetch => Received DEALLOC from Commit.\n");
                )

                processStreamerControlCommand(cycle, streamCCom);
            }

            //  Commit received memory requests.  
            if (reorderQueue[nextCommit] > 0)
            {
                //  Update index data counter.  
                indexData += reorderQueue[nextCommit];

                //  Update requested data counter.  
                requestedData -= reorderQueue[nextCommit];

                //  Clear reorder queue entry.  
                reorderQueue[nextCommit] = 0;

                //  Update pointer to next request to commit.  
                nextCommit = GPU_MOD(nextCommit + 1, MAX_MEMORY_REQUESTS);

                //  Update the number of active memory requests.  
                activeRequests--;
            }

            /*  Ask for index data if index mode and index buffer not full and there is
                free entry in the reorder queue.  */
            if (((indexData + requestedData + bufferPaddingBytes) <= (indexBufferSize - MAX_TRANSACTION_SIZE))
                && ((memoryState & MS_READ_ACCEPT) != 0) && (requestedIndexData < (streamCount * streamDataSize[indexStreamData]))
                && (reorderQueue[nextRequest] == 0) && (activeRequests < MAX_MEMORY_REQUESTS))
            {
                //  Check available memory ticket.  
                CG_ASSERT_COND(!(freeTickets == 0), "No memory tickets available.");
                //  Calculate the address for the next index data to fetch.  
                indexAddress = indexStreamAddress + (streamStart * streamDataSize[indexStreamData]) + requestedIndexData;

                U32 alignPadding = 0;
                
                //  Check if start of the index stream is not aligned to MAX_TRANSACTION_SIZE.
                if ((requestedIndexData == 0) && (GPU_MOD(indexAddress, MAX_TRANSACTION_SIZE) != 0))
                {
                    //  Add padding bytes used for alignment to skip at the start of the index stream.
                    alignPadding = streamPaddingBytes = GPU_MOD(indexAddress, MAX_TRANSACTION_SIZE);

                    //  Align request address.
                    indexAddress = indexAddress - streamPaddingBytes;
                    
                    GPU_DEBUG_BOX(
                        printf("StreamerFetch (%lld) => Adding %d bytes of padding to align the start of the index stream\n",
                            cycle, streamPaddingBytes);
                    )
                }

                /*  Calculate request size.  Avoid crossing MAX_TRANSACTION_SIZE data alignment boundary.
                    Avoid overflowing the index buffer.  */
                size = GPU_MIN(GPU_MIN(MAX_TRANSACTION_SIZE - (indexAddress & TRANSACTION_OFFSET_MASK),
                    (streamCount * streamDataSize[indexStreamData]) - requestedIndexData + alignPadding),
                    indexBufferSize - nextFreeIndex);
                    
                GPU_DEBUG_BOX(
                    printf("StreamerFetch (%lld) => Requesting Index data: Address %08x | Size %d | BufferPosition = %08x\n",
                        cycle, indexAddress, size, nextFreeIndex);
                )

//printf("StFetch %lld > reading %d bytes from %x storing at %d into index buffer of size %d\n", cycle, size,
//indexStreamAddress + (streamStart * streamDataSize[indexStreamData]) + requestedIndexData, nextFreeIndex,
//indexBufferSize);

//printf("StreamerFetch => indexStreamAddress = %x | streamStart = %d | requestedIndexData = %d | dataSize = %d\n",
//       indexStreamAddress, streamStart, requestedIndexData, streamDataSize[indexStreamData]);
//printf("StreamerFetch => requesting index data at address %x bytes %d\n", indexAddress, size);

                //  Create memory transaction.  
                memTrans = new MemoryTransaction(
                    MT_READ_REQ,
                    indexAddress,
                    size,
                    &indexBuffer[nextFreeIndex],
                    STREAMERFETCH,
                    currentTicket);

                /*  Set signal tracing info.
                    Copy cookies from the last StreamController command.  */
                memTrans->copyParentCookies(*lastStreamCom);
                memTrans->addCookie();

                //  Update statistics.  
                transactions->inc();

                //  Update free memory ticket counter.  
                freeTickets--;

                //  Store reorder queue entry in the request translation table.  
                memoryRequest[GPU_MOD(currentTicket, MAX_MEMORY_TICKETS)] = nextRequest;

                //  Update pointer to the next request.  
                nextRequest = GPU_MOD(nextRequest + 1, MAX_MEMORY_REQUESTS);

                //  Update the number of active memory requests.  
                activeRequests++;

                //  Update current ticket counter.  
                currentTicket++;

                //  Send memory request to the memory controller.  
                streamerFetchMemReq->write(cycle, memTrans);

                //  Update requested index pointer.  
                nextFreeIndex = GPU_MOD(nextFreeIndex + size, indexBufferSize);

                //  Update requested data pointer.  
                requestedData += size;

                //  Update requested index data counter.
                requestedIndexData += (requestedIndexData == 0) ? (size - alignPadding) : size;
            }

            GPU_DEBUG_BOX(
                printf("StreamerFetch => requestedIndexData = %d | instance data = %d\n",
                requestedIndexData, (streamCount * streamDataSize[indexStreamData]));
            )
            
            //  Check if there are more instances to process.
            if (readInstance < streamInstances)
            {
                //  Check if all the index data for the current instance was requested.
                if (requestedIndexData == (streamCount * streamDataSize[indexStreamData]))
                {            
                    //  Compute how many bytes have to be advanced the free pointer into
                    //  the index buffer to reach an address aligned to the memory transaction
                    //  size.
                    U32 alignPadding = GPU_MOD(MAX_TRANSACTION_SIZE - GPU_MOD(nextFreeIndex, MAX_TRANSACTION_SIZE), MAX_TRANSACTION_SIZE);
                    
                    GPU_DEBUG_BOX(
                        printf("StreamerFetch (%lld) => indexData = %d | requestedData = %d | bufferPaddingBytes = %d | alignPadding = %d | indexBufferSize = %d\n",
                            cycle, indexData, requestedData, bufferPaddingBytes, alignPadding, indexBufferSize);
                    )
                    
                    //  Check if there is enough free space available in the index buffer.
                    if ((indexData + requestedData + bufferPaddingBytes + alignPadding) <= indexBufferSize)
                    {
                        //  Update the number of instances already read.
                        readInstance++;
                     
                        GPU_DEBUG_BOX(
                            printf("StreamerFetch (%lld) => readInstance = %d | streamInstances = %d\n", cycle, readInstance, streamInstances);
                        )
                        
                        //  Check if all the instances were read.   
                        if (readInstance < streamInstances)
                        {                                   
                            //  Restart requests for index data.
                            requestedIndexData = 0;
                            
                            //  Move pointer to next free position in the index buffer to an aligned address.
                            nextFreeIndex = GPU_MOD(nextFreeIndex + alignPadding, indexBufferSize);
                            
                            //  Set buffer padding bytes.
                            bufferPaddingBytes += alignPadding;
                            
                            GPU_DEBUG_BOX(
                                printf("StreamerFetch (%lld) => Restarting index buffer read for instance %d.  Next free buffer position at %08x.\n",
                                    cycle, readInstance, nextFreeIndex);
                                printf("StreamerFetch (%lld) => Adding %d bytes for padding.  Total buffer padding bytes %d\n", cycle, alignPadding, bufferPaddingBytes);
                            )
                        }
                    }
                }
            }
            
            /* Since each index is sent in order to different cmoStreamController Loader units, new indices are only sent if all the previous 
               were successfully sent this cycle (the corresponding SLs had
               enough available entries). */
            
            previousSent = true;
            
            //  Initialize counts of indices sent per cmoStreamController Loader unit.  
            for (i = 0; i < streamerLoaderUnits; i++)
            {
                indicesSentThisCycle[i] = 0;
            }

            //  Send indices to the StreamController output cache.  
            for(i = 0; (i < indicesCycle) &&  (fetchedIndexes < streamCount) && previousSent; i++)
            {  
                //  Start with the cmoStreamController Loader Unit 0 and so on.  
                selectedSLUnit = 0;
                testedUnits = 0;       
      
                //  Check if free IRQ entries and any free index slot for the selected unit this cycle.   
                while ((freeIRQEntries[selectedSLUnit] == 0 || (indicesSentThisCycle[selectedSLUnit] == slIndicesCycle)) 
                        && testedUnits < streamerLoaderUnits)
                { 
                     //  Keep searching for the next cmoStreamController Loader unit in a circular way.   
                     selectedSLUnit = GPU_MOD(selectedSLUnit + 1, streamerLoaderUnits);
                     testedUnits++;
                }
  
  
                //  If index data available and free IRQ, output FIFO and output memory entries fetch the next index.  
                if ((indexData != 0) && (freeOutputFIFOEntries > 0) && (freeOutputMemoryLines > 0) && (freeIRQEntries[selectedSLUnit] > 0)
                     && (indicesSentThisCycle[selectedSLUnit] < slIndicesCycle))
                {
                    //  Check indexed mode or stream mode flag.  
                    if (indexedMode)
                    {
                        //  Check for padding bytes at the start of the index stream.
                        if (skipPaddingBytes && (streamPaddingBytes > 0))
                        {
                            //  Move pointer to the first index.
                            nextIndex = GPU_MOD(nextIndex + streamPaddingBytes, indexBufferSize);
                            
                            //  Update counter for the available index data.
                            indexData = indexData - streamPaddingBytes;

                            skipPaddingBytes = false;
                        }

                        //  Fetch next index from the index buffer.  
                        newIndex = indexDataConvert(indexStreamData, &indexBuffer[nextIndex]);

                        GPU_DEBUG_BOX(
                            printf("StreamerFetch (%lld) => Getting new index %d from the index buffer position %08x for instance %d\n", cycle, newIndex, nextIndex, fetchInstance);
                        )
                    }
                    else
                    {
                        //  Sequential stream mode.  

                        //  Generate a sequential index number.  
                        newIndex = nextIndex;

                        GPU_DEBUG_BOX(
                            printf("StreamerFetch (%lld) => Generating sequential index %d for instance %d\n", cycle, newIndex, fetchInstance);
                        )

                    }

                    /*  Create StreamController control command to send the new index to the output cache.
                        Get a free input request queue entry, a free output FIFO entry and a free output memory line.  */
                    streamCCom = new StreamerControlCommand(STRC_NEW_INDEX, newIndex, fetchInstance, nextFreeOFIFOEntry);

                    /*  Set the destination cmoStreamController Loader Unit. Indexes are sequentially distributed among available units. 
                        NOTE: Other distribution schemes should be considered as well, to improve index locality. */
                    streamCCom->setUnitID(selectedSLUnit);

                    GPU_DEBUG_BOX(
                        printf("StreamerFetch (%lld) => NEW_INDEX idx %d instance %d OFIFO %d\n", cycle, newIndex, fetchInstance, streamCCom->getOFIFOEntry());
                    )

                    /*  Set signal tracing info.
                        Copy cookies from the last StreamController command.  */
                    streamCCom->copyParentCookies(*lastStreamCom);
                    streamCCom->addCookie();

                    //  Update statistics.  
                    sentIdx->inc();

                    //  Update the proper IRQ entry counter.  
                    freeIRQEntries[selectedSLUnit]--;

                    //  Update indices sent the current cycle to the selected cmoStreamController Loader Unit.  
                    indicesSentThisCycle[selectedSLUnit]++;

                    //  Update free output FIFO entry counter.  
                    freeOutputFIFOEntries--;

                    //  Update next free output FIFO entry pointer.  
                    nextFreeOFIFOEntry = GPU_MOD(nextFreeOFIFOEntry + 1, outputFIFOSize);

                    //  Update free output memory lines counter.  
                    freeOutputMemoryLines--;

                    //  Check indexed mode flag.  
                    if (indexedMode)
                    {
                        //  Update index data in the index buffer counter.  
                        indexData = indexData - streamDataSize[indexStreamData];

                        //  Update next index counter.  
                        nextIndex = GPU_MOD(nextIndex + streamDataSize[indexStreamData], indexBufferSize);
                    }
                    else
                    {
                        //  Sequential stream mode.  

                        //  There is always index data, so don't change indexData.  

                        //  Update sequentally the next index counter.  
                        nextIndex++;
                    }

                    //  Update number of fetched indexes.  
                    fetchedIndexes++;

                    //  Mark as last index if required.  
                    streamCCom->setLast((fetchedIndexes == streamCount) && (fetchInstance == (streamInstances - 1)));
                    
                    GPU_DEBUG_BOX(
                        if ((fetchedIndexes == streamCount) && (fetchInstance == (streamInstances - 1)))
                        {
                            printf("StreamerFetch => Marked INDEX as LAST.\n");
                        }
                    )
                    

                    //  Send new index to the output cache.  
                    streamerOutputCache->write(cycle, streamCCom);
                }
                else
                {
                    previousSent = false;
                }
            }

            //  Check if all the indices for the current instance were fetched.
            if (fetchedIndexes == streamCount)
            {
                //  Update the fetched instances counter.
                fetchInstance++;
                
                //  Check if all the instances were fetched.
                if (fetchInstance < streamInstances)
                {
                    //  Restart index fetching for the next instance.
                    if (indexedMode)
                    {
                        //  Compute the bytes require to align the pointer to the next index data to the
                        //  next memory transaction size aligned position in the index buffer.
                        U32 alignPadding = GPU_MOD(MAX_TRANSACTION_SIZE - GPU_MOD(nextIndex, MAX_TRANSACTION_SIZE), MAX_TRANSACTION_SIZE);
                        
                        //  Move pointer to the next index with data to the next position in the buffer
                        //  aligned to the memory transaction size.
                        nextIndex = GPU_MOD(nextIndex + alignPadding, indexBufferSize);
                        
                        //  Free the buffer space used for padding.
                        bufferPaddingBytes -= alignPadding;
                        
                        skipPaddingBytes = true;
                    }
                    else
                        nextIndex = streamStart;

                    fetchedIndexes = 0;
                    
                    GPU_DEBUG_BOX(
                        printf("StreamerFetch (%lld) => Starting new instance %d.  Next index at buffer position %08x.\n", cycle, fetchInstance,
                            nextIndex);
                    )
                    
                }
                else
                {
                    //  Change to finished state if all the instance and indexes have been fetched.
                    state = ST_FINISHED;
                    
                    GPU_DEBUG_BOX(
                        printf("StreamerFetch => End of current draw call.\n");
                    )                   
                }
            }
            
            break;

        case ST_FINISHED:

            //  End of streaming of a batch of vertexes/inputs.  

            GPU_DEBUG_BOX( printf("StreamerFetch => FINISHED state.\n"); )

            //  Read signals from the stream loader units.  
            for (i = 0; i < streamerLoaderUnits; i++)
            {
                while (streamerLoaderDeAlloc[i]->read(cycle, (DynamicObject *&) streamCCom))
                {
                    GPU_DEBUG_BOX(
                        printf("StreamerFetch => Received DEALLOC from Loader.\n");
                    )

                    processStreamerControlCommand(cycle, streamCCom);
                }
            }

            //  Initialize the proper unconfirmed deallocated OM line counter.  
            unconfirmedDeAllocCounters[GPU_MOD(cycle, 2)] = 0;

            /*  Read signal from the stream commit.  In a single cycle can
                arrrive an Output FIFO DeAlloc and an Output Memory DeAlloc.  */
            while(streamerCommitDeAlloc->read(cycle, (DynamicObject *&) streamCCom))
            {
                GPU_DEBUG_BOX(
                    printf("StreamerFetch => Received DEALLOC from Commit.\n");
                )

                processStreamerControlCommand(cycle, streamCCom);
            }

            //  Wait for end command.  

            //  Get command from the cmoStreamController main mdu.  
            if (streamerFetchCommand->read(cycle, (DynamicObject *&) streamCom))
                processStreamerCommand(streamCom);

            break;

        default:
            CG_ASSERT("Unsupported StreamController fetch state.");
    }

    //  Send cmoStreamController Fetch state to the cmoStreamController main mdu.  
    streamerFetchState->write(cycle, new StreamerStateInfo(state));
}

//  Process a StreamController command.  
void StreamerFetch::processStreamerCommand(StreamerCommand *streamCom)
{
    U32 i;

    //  Delete last StreamController command.  
    delete lastStreamCom;

    //  Store as last StreamController command (to keep object info).  
    lastStreamCom = streamCom;

    //  Process StreamController command.  
    switch(streamCom->getCommand())
    {
        case STCOM_RESET:
            //  Reset command.  

            GPU_DEBUG_BOX( printf("StreamerFetch => RESET command.\n"); )

            //  Set state to reset.  
            state = ST_RESET;
            break;

        case STCOM_REG_WRITE:
            //  Write to register command.  

            GPU_DEBUG_BOX( printf("StreamerFetch => REG_WRITE command.\n"); )

            //  Check cmoStreamController state.  
            CG_ASSERT_COND(!(state != ST_READY), "Command not allowed in this state.");
            //  Process the register write.  
            processGPURegisterWrite(streamCom->getStreamerRegister(),
                streamCom->getStreamerSubRegister(),
                streamCom->getRegisterData());

            break;

        case STCOM_REG_READ:
            //  Read from register command.  

            GPU_DEBUG_BOX( printf("StreamerFetch => REG_READ command.\n"); )

            //  Not supported.  
            CG_ASSERT("STCOM_REG_READ not supported.");

            break;

        case STCOM_START:
            //  Start streaming.

            GPU_DEBUG_BOX( printf("StreamerFetch => START command.\n"); )

            //  Check cmoStreamController Fetch state.
            CG_ASSERT_COND(!(state != ST_READY), "Command not allowed in this state.");
            //  Initialize streaming state.
            freeOutputFIFOEntries = outputFIFOSize;
            freeOutputMemoryLines = outputMemorySize;

            for (i = 0; i < streamerLoaderUnits; i++)
            {
                freeIRQEntries[i] = inputRequestQueueSize;
            }
 
            nextFreeOFIFOEntry = 0;

            //  Check indexed mode enabled.
            if (indexedMode)
                nextIndex = 0;
            else
                nextIndex = streamStart;
            nextFreeIndex = 0;
            fetchedIndexes = 0;
            requestedIndexData = 0;
            readInstance = 0;
            fetchInstance = 0;

            //  Check indexed mode flag.  
            if(indexedMode)
            {
                //  Set variables to request index data from memory.
                indexData = 0;

                //  Reset requested data pointer.
                requestedData = 0;

                //  Reset padding bytes counter.
                streamPaddingBytes = 0;
                bufferPaddingBytes = 0;
                skipPaddingBytes = true;
            }
            else
            {
                //  Sequential stream mode.

                //  We don't need index data.
                indexData = indexBufferSize;
                requestedData = 0;
            }

            //  Change state to streaming.
            state = ST_STREAMING;

            break;

        case STCOM_END:
            //  End streaming (drawing) command.  

            GPU_DEBUG_BOX( printf("StreamerFetch => END command.\n"); )

            //  Check cmoStreamController Fetch state.  
            CG_ASSERT_COND(!(state != ST_FINISHED), "Command not allowed in this state.");
            //  Change state to ready.  
            state = ST_READY;

            break;

        default:
            CG_ASSERT("Undefined StreamController command.");
            break;
    }

}


//  Process a register write.  
void StreamerFetch::processGPURegisterWrite(GPURegister gpuReg, U32 gpuSubReg,
     GPURegData gpuData)
{

    //  Process Register.  
    switch(gpuReg)
    {
        case GPU_STREAM_ADDRESS:
            //  Stream buffer address.  

            //  Write the register.  
            indexStreamAddress = gpuData.uintVal;

            GPU_DEBUG_BOX(
                printf("StreamerFetch => Index GPU_STREAM_ADDRESS = 0x%0x.\n",
                    gpuData.uintVal);
            )

            break;

        case GPU_STREAM_DATA:
            //  Index Stream buffer data type.  

            //  Write the register.  
            indexStreamData = gpuData.streamData;

            GPU_DEBUG_BOX(
                printf("StreamerFetch => Index GPU_STREAMDATA = ");
                switch(gpuData.streamData)
                {
                    case SD_UINT8:
                    case SD_UNORM8:     //  For compatibility with old library.
                        printf("SD_UINT8.\n");
                        break;
                    case SD_UINT16:
                    case SD_UNORM16:    //  For compatibility with old library.
                        printf("SD_UINT16.\n");
                        break;
                    case SD_UINT32:
                    case SD_UNORM32:    //  For compatibility with old library.
                        printf("SD_UINT32.\n");
                        break;
                    case SD_SNORM8:
                    case SD_SNORM16:
                    case SD_SNORM32:
                    case SD_FLOAT16:
                    case SD_FLOAT32:
                    case SD_SINT8:
                    case SD_SINT16:
                    case SD_SINT32:
                        CG_ASSERT("Format type not supported for index data..");
                        break;
                    default:
                        CG_ASSERT("Undefined index data format type.");
                        break;
                }
            )

            break;

        case GPU_STREAM_START:
            //  Streaming start position.  

            streamStart = gpuData.uintVal;

            GPU_DEBUG_BOX(
                printf("StreamerFetch => GPU_STREAM_START = %d.\n", gpuData.uintVal);
            )

            break;

        case GPU_STREAM_COUNT:
            //  Streaming vertex count.  

            streamCount = gpuData.uintVal;

            GPU_DEBUG_BOX(
                printf("StreamerFetch => GPU_STREAM_COUNT = %d.\n", gpuData.uintVal);
            )

            break;

        case GPU_STREAM_INSTANCES:
            
            //  Streaming instance count.
            streamInstances = gpuData.uintVal;

            GPU_DEBUG_BOX(
                printf("StreamerFetch => GPU_STREAM_INSTANCES = %d.\n", gpuData.uintVal);
            )

            break;

        case GPU_INDEX_MODE:
            //  Indexed streaming mode enabled/disabled.  

            indexedMode = gpuData.booleanVal;

            GPU_DEBUG_BOX(
                printf("StreamerFetch => GPU_INDEX_MODE = %s.\n",
                    gpuData.booleanVal?"TRUE":"FALSE");
            )

            break;

        default:
            CG_ASSERT("Not a cmoStreamController register.");
            break;
    }
}

//  Processes a memory transaction.  
void StreamerFetch::processMemoryTransaction(MemoryTransaction *memTrans)
{
    //  Process the memory transaction.  
    switch(memTrans->getCommand())
    {
        case MT_STATE:

            //  Memory Controller is ready to receive new requests.  
            memoryState = memTrans->getState();

            GPU_DEBUG_BOX( printf("StreamerFetch => MT_STATUS received.\n"); )

            break;

        case MT_READ_DATA:
            //  Return data from the previous memory request.  

            GPU_DEBUG_BOX(
                printf("StreamerFetch => MT_READ_DATA received ID %d.\n",
                    memTrans->getID());
            )

            //  Check there is no memory transmission active.  
            CG_ASSERT_COND(!(busCycles != 0), "Memory transmission still in progress.");
/*{
U08 *data;
printf("StreamerFetch => Received MT_READ_DATA with ID %d for address %08x\n", memTrans->getID(), memTrans->getAddress());
printf("DATA :\n");
data = memTrans->getData();
for(U32 b = 0; b < memTrans->getSize(); b++)
{
printf("%02x ", data[b]);
if ((b % 16) == 15) printf("\n");
}
printf("\n");
}*/

            //  Set transmission cycles.  
            busCycles = memTrans->getBusCycles();

            //  Store ticket for the transaction.  
            readTicket = memTrans->getID();

            //  Set current transaction size.  
            lastSize = memTrans->getSize();

            break;

        default:
            CG_ASSERT("Illegal memory transaction received.");
            break;

    }

    //  Delete memory transaction.  
    delete memTrans;
}

//  Converts from the stream buffer data format to a 32 bit integer.  
U32 StreamerFetch::indexDataConvert(StreamData format, U08 *data)
{
    U32 res;

    switch(format)
    {
        case SD_UINT8:
        case SD_UNORM8:
            //  Get a 8 bit integer.  
            res = *data;
            break;
        case SD_UINT16:
        case SD_UNORM16:
            //  Get a 16 bit integer.  
            res = *((U16 *) data);
            break;
        case SD_UINT32:
        case SD_UNORM32:
            //  Get a 32 bit integer.  
            res = *((U32 *) data);
            break;
        case SD_SNORM8:
        case SD_SNORM16:
        case SD_SNORM32:
        case SD_FLOAT16:
        case SD_FLOAT32:
        case SD_SINT8:
        case SD_SINT16:
        case SD_SINT32:
            CG_ASSERT("Format type not supported for index data..");
            break;
        default:
            CG_ASSERT("Undefined index data format type.");
            break;
    }

    return res;
}

//  Process a StreamController control command.  
void StreamerFetch::processStreamerControlCommand(U32 cycle, StreamerControlCommand *streamCCom)
{
    U32 unitId;
    char panicMsg[60];
    U32 i;

    //  Process the StreamController control command.  
    switch(streamCCom->getCommand())
    {
        case STRC_DEALLOC_IRQ:
            //  Deallocate an input request queue entry.  

            unitId = streamCCom->getUnitID();

            GPU_DEBUG_BOX(
                printf("StreamerFetch => Deallocating IRQ entry %d from cmoStreamController Loader unit %d\n",
                    streamCCom->getIRQEntry(), unitId);
            )
            
            GPU_ASSERT(
               
                if (freeIRQEntries[unitId] == inputRequestQueueSize)
                {
                    sprintf(panicMsg, "All IRQ entries in SL%d are already free.", unitId);  
                    CG_ASSERT(panicMsg);
                }
            )

            //  Add to the free IRQ entry list.  Update free IRQ entry counter.  
            freeIRQEntries[unitId]++;

            break;

        case STRC_DEALLOC_OFIFO:
            //  Deallocate an output FIFO entry.  

            GPU_DEBUG_BOX(
                printf("StreamerFetch => Deallocating output FIFO entry %d\n",
                    streamCCom->getOFIFOEntry());
            )

            CG_ASSERT_COND(!(freeOutputFIFOEntries == outputFIFOSize), "All output FIFO entries are already free.");
            /*  Add to the free ouput FIFO entry list.
                Update free output FIFO entry counter.  */
            freeOutputFIFOEntries++;

            break;

        case STRC_DEALLOC_OM:

            //  New deallocated output memory line pending to be confirmed.  

            GPU_DEBUG_BOX(
                printf("StreamerFetch => Received new deallocated output memory line %d pending to be confirmed\n",
                    streamCCom->getOMLine());
            )

            /*  Add new unconfirmed deallocated output memory line from cmoStreamController Commit to the list. Alternate between
                lists for even and odd cycles.  */
            unconfirmedOMLineDeAllocs[GPU_MOD(cycle, 2)][unconfirmedDeAllocCounters[GPU_MOD(cycle, 2)]] = streamCCom->getOMLine();

            //  Update received unconfirmed deallocs counter.  
            unconfirmedDeAllocCounters[GPU_MOD(cycle, 2)]++;

            break;

        case STRC_DEALLOC_OM_CONFIRM:

            //  New confirmation of a deallocated output memory line.  

            GPU_DEBUG_BOX(
                printf("StreamerFetch => Received new deallocated output memory line %d confirmation\n",
                    streamCCom->getOMLine());
            )

            //  Check if any allocated output memory line the previous cycle is being confirmed.  
            for (i = 0; i < unconfirmedDeAllocCounters[GPU_MOD(cycle + 1, 2)]; i++)
            {
                 //  If confirmed output memory line deallocation then release the output memory line.  
                 if (unconfirmedOMLineDeAllocs[GPU_MOD(cycle + 1, 2)][i] == streamCCom->getOMLine())
                 {
                     GPU_DEBUG_BOX(
                         printf("StreamerFetch => Deallocating output memory line %d\n",
                             streamCCom->getOMLine());
                     )

                     CG_ASSERT_COND(!(freeOutputMemoryLines == outputMemorySize), "All output memory lines are already free.");
                     /*  Add to the output memory free list.
                         Update free output memory line counter.  */
                     freeOutputMemoryLines++;
                 }
            }

            break;

        case STRC_OM_ALREADY_ALLOC:

            GPU_DEBUG_BOX(
                printf("StreamerFetch => cmoStreamController Commit had already allocated the OM Line %d for a new index sent\n",   
                    streamCCom->getOMLine());
            )

            CG_ASSERT_COND(!(freeOutputMemoryLines == outputMemorySize), "All output memory lines are already free.");
            /*  Add to the output memory free list.
                Update free output memory line counter.  */
            freeOutputMemoryLines++;

            break;

        default:
            switch ( streamCCom->getCommand() )
            {
                case STRC_NEW_INDEX:
                    CG_ASSERT("STRC_NEW_INDEX. Unsupported StreamController control command.");
                case STRC_UPDATE_OC:
                    CG_ASSERT("STRC_UPDATE_OC. Unsupported StreamController control command.");
            }
            CG_ASSERT("(Unknown command) Unsupported StreamController control command.");
            break;
    }

    //  Delete StreamController control command.  
    delete streamCCom;
}
