/**************************************************************************
 *
 * cmoStreamController Output Cache class implementation file.
 *
 */

#include "cmStreamerOutputCache.h"
#include "cmStreamerControlCommand.h"
#include <iostream>

using namespace arch;
using namespace std;

//  cmoStreamController Output Cache mdu constructor.  
StreamerOutputCache::StreamerOutputCache(U32 idxCycle, U32 omSize, U32 vertCycle, U32 sLoaderUnits, U32 slIdxCycle, char *name, cmoMduBase *parent) :

    indicesCycle(idxCycle), outputMemorySize(omSize), verticesCycle(vertCycle), streamerLoaderUnits(sLoaderUnits), 
    slIndicesCycle(slIdxCycle), cmoMduBase(name, parent)

{
    U32 i;
    char** sLoaderPrefix;

    //  Check parameters.  
    GPU_ASSERT(
        if (indicesCycle == 0)
            CG_ASSERT("At least an index per cycle required.");
        if (verticesCycle == 0)
            CG_ASSERT("At least a vertex per cycle required.");
        if (outputMemorySize == 0)
            CG_ASSERT("At least a vertex entry in the output cache required.");
        if (indicesCycle != streamerLoaderUnits * slIndicesCycle)
            CG_ASSERT("Indexes per cycle must equal overall indexes per cycle for the total cmoStreamController Loader units."); 
    )

    //  Allocate memory for the output cache tags.  
    outputCacheTagsIndex = new U32[outputMemorySize];
    outputCacheTagsInstance = new U32[outputMemorySize];

    //  Check memory allocation.  
    GPU_ASSERT(
        if (outputCacheTagsIndex == NULL)
            CG_ASSERT("Error allocating memory for the output cache tags (index).");
        if (outputCacheTagsInstance == NULL)
            CG_ASSERT("Error allocating memory for the output cache tags (instance).");
    )

    //  Allocate memory for the output cache valid bits.  
    outputCacheValidBit = new bool[outputMemorySize];

    //  Check memory allocation.  
    CG_ASSERT_COND(!(outputCacheValidBit == NULL), "Error allocating memory for the output cache valid bits.");
    //  Allocate memory for the output memory free list.  
    outputMemoryFreeList = new U32[outputMemorySize];

    //  Check memory allocation.  
    CG_ASSERT_COND(!(outputMemoryFreeList == NULL), "Error allocating memory for the output memory free list.");
    //  Allocate the array of unconfirmed deallocated output memory lines from cmoStreamController Commit.  
    unconfirmedOMLineDeAllocs = new U32*[2];

    //  Check memory allocation.  
    CG_ASSERT_COND(!(unconfirmedOMLineDeAllocs == NULL), "Error allocating the array of unconfirmed deallocated output memory lines from cmoStreamController Commit.");
    for (i = 0; i < 2; i++)
    {
         unconfirmedOMLineDeAllocs[i] = new U32[verticesCycle];

         //  Check memory allocation.  
         CG_ASSERT_COND(!(unconfirmedOMLineDeAllocs[i] == NULL), "Error allocating the array of unconfirmed deallocated output memory lines from cmoStreamController Commit.");    }

    //  Allocate the unconfirmed deallocation counters.  
    unconfirmedDeAllocCounters = new U32[2];

    //  Check memory allocation.  
    CG_ASSERT_COND(!(unconfirmedDeAllocCounters == NULL), "Error allocating the counters of deallocated output memory lines from cmoStreamController Commit.");
    //  Allocate the array of confirmation status of deallocated lines.  
    deAllocConfirmed = new bool[verticesCycle];
      
    //  Create command signal from the cmoStreamController main mdu.  
    streamerCommand = newInputSignal("StreamerOutputCacheCommand", 1, 1);

    //  Create new index signal from the cmoStreamController Fetch mdu.  
    streamerFetchNewIndex = newInputSignal("StreamerOutputCache", indicesCycle, 1);

    //  Create new index signal to the cmoStreamController Commit mdu.  
    streamerCommitNewIndex = newOutputSignal("StreamerCommitNewIndex", indicesCycle, 1);

    sLoaderPrefix = new char*[sLoaderUnits];

    //  Create new index signals to the all cmoStreamController Loader boxes.  
    
    streamerLoaderNewIndex = new Signal*[sLoaderUnits];
    
    for (i = 0; i < sLoaderUnits; i++)
    {
        sLoaderPrefix[i] = new char[6];
        sprintf(sLoaderPrefix[i], "SL%d",i);
    
        streamerLoaderNewIndex[i] = newOutputSignal("StreamerLoaderNewIndex", slIndicesCycle, 1, sLoaderPrefix[i]);
        delete[] sLoaderPrefix[i];
    }

    delete[] sLoaderPrefix;

    //  Create deallocation signal from the cmoStreamController Commit mdu.  
    streamerCommitDeAlloc = newInputSignal("StreamerCommitDeAllocOC", 2 * verticesCycle, 1);

    //  Create update signal to the cmoStreamController Output Cache.  
    streamerOCUpdateWrite = newOutputSignal("StreamerOCUpdate", indicesCycle, 1);

    //  Create update signal from the cmoStreamController Output Cache.  
    streamerOCUpdateRead = newInputSignal("StreamerOCUpdate", indicesCycle, 1);


    //  Allocate the index request queue.  
    indexReqQ = new StreamerControlCommand*[indicesCycle];

    //  Check allocation.  
    CG_ASSERT_COND(!(indexReqQ == NULL), "Error allocating current cycle index request queue.");
    //  Allocate array storing the solved misses allocated output memory lines.  
    missOutputMLines = new U32*[indicesCycle];

    //  Check allocation.  
    CG_ASSERT_COND(!(missOutputMLines == NULL), "Error allocating the allocated output memory lines to misses.");
    for(i = 0; i < indicesCycle; i++)
    {
        missOutputMLines[i] = new U32[3];

        //  Check allocation.  
        CG_ASSERT_COND(!(missOutputMLines[i] == NULL), "Error allocating the allocated output memory lines to misses.");    }

    //  Create statistics.  
    hits = &getSM().getNumericStatistic("Hits", U32(0), "StreamerOutputCache", "StOC");
    misses = &getSM().getNumericStatistic("Misses", U32(0), "StreamerOutputCache", "StOC");
    indices = &getSM().getNumericStatistic("Indices", U32(0), "StreamerOutputCache", "StOC");

    //  Set initial state.  
    state = ST_RESET;

    //  Create dummy first StreamController command.  
    lastStreamCom = new StreamerCommand(STCOM_RESET);

}

//  cmoStreamController Output Cache mdu simulation rutine.  
void StreamerOutputCache::clock(U64 cycle)
{
    StreamerCommand *streamCom;
    StreamerControlCommand *streamCCom, *streamCComClone, *streamCComAux;
    bool outputCacheHit;
    U32 outputMemoryLine;
    U32 i;
    U32 j;
    U32 index;
    U32 instance;
    U32 e, ePrev;

    GPU_DEBUG_BOX( printf("StreamerOutputCache => Clock %lld\n", cycle);)
/*
if ((cycle > 240000) && (GPU_MOD(cycle, 10000) == 0))
{
    printf("StOC %lld => freeOutputMemoryLines %d \n", cycle, freeOutputMemoryLines);
}
*/
    //  Simulate current cycle.  
    switch(state)
    {
        case ST_RESET:
            //  Reset state.  Just reset some structures and go back to ready state.  

            GPU_DEBUG_BOX( printf("StreamerOutputCache => RESET state.\n"); )

            //  Set all output cache valid bits to FALSE.  
            for (i = 0; i < outputMemorySize; i++)
                outputCacheValidBit[i] = FALSE;

            //  Fill the output memory free list.  
            for (i = 0; i < outputMemorySize; i++)
                outputMemoryFreeList[i] = i;

            //  Reset first free output memory list pointer.  
            nextFreeOMLine = 0;

            //  Reset free output memory lines counter.  
            freeOutputMemoryLines = outputMemorySize;

            //  Reset current cycle index request queue.  
            for(i = 0; i < indicesCycle; i++)
                indexReqQ[i] = NULL;

            //  Initialize unconfirmed deallocated output memory line counters.  
            unconfirmedDeAllocCounters[0] = 0;
            unconfirmedDeAllocCounters[1] = 0;

            //  Change state to ready.  
            state = ST_READY;

            break;

        case ST_READY:
            //  Ready state.  Just wait for a START command.  

            GPU_DEBUG_BOX( printf("StreamerOutputCache => READY state.\n"); )

            //  Read commands from the main cmoStreamController mdu.  
            if (streamerCommand->read(cycle, (DynamicObject *&) streamCom))
                processStreamerCommand(streamCom);

            break;

        case ST_STREAMING:

            /*  Streaming state.  Wait for indexes from the cmoStreamController Fetch.
                Change to ready state when the END command is received.  */

            GPU_DEBUG_BOX( printf("StreamerOutputCache => STREAMING state.\n"); )

            //  Read output cache updates.  
            while (streamerOCUpdateRead->read(cycle, (DynamicObject *&) streamCCom))
            {
                CG_ASSERT_COND(!(streamCCom->getCommand() != STRC_UPDATE_OC), "Unsupported cmoStreamController Control Command from the cmoStreamController Output Cache.");
                GPU_DEBUG_BOX(
                    printf("StreamerOutputCache => UPDATE_OC command received: (Index = %d, Line = %d).\n",
                        streamCCom->getIndex(), streamCCom->getOMLine());
                )

                //  Update output cache.  
                outputCacheTagsIndex[streamCCom->getOMLine()] = streamCCom->getIndex();
                outputCacheTagsInstance[streamCCom->getOMLine()] = streamCCom->getInstanceIndex();

                //  Set output cache valid bit.  
                outputCacheValidBit[streamCCom->getOMLine()] = TRUE;

                //  Delete StreamController control command.  
                delete streamCCom;
            }

            //  Initialize the proper unconfirmed deallocated OM line counter.  
            unconfirmedDeAllocCounters[GPU_MOD(cycle, 2)] = 0;

            //  Initialize the array to track confirmed lines.  
            for (i = 0; i < verticesCycle; i++)
            {
                 deAllocConfirmed[i] = false;
            }

            //  Read deallocation updates from the cmoStreamController Commit (confirmed and unconfirmed).  
            while (streamerCommitDeAlloc->read(cycle, (DynamicObject *&) streamCCom))
            {
                //  Check StreamController control command.  
                CG_ASSERT_COND(!((streamCCom->getCommand() != STRC_DEALLOC_OM) && (streamCCom->getCommand() != STRC_DEALLOC_OM_CONFIRM)), "Unsupported cmoStreamController Control Command from the cmoStreamController Commit.");
                switch(streamCCom->getCommand())
                {
                    case STRC_DEALLOC_OM:

                        GPU_DEBUG_BOX(
                            printf("StreamerOutputCache => DEALLOC_OM command received: %d.\n",
                            streamCCom->getOMLine());
                        )

                        /*  Add new unconfirmed deallocated output memory line from cmoStreamController Commit to the list. Alternate between
                            lists for even and odd cycles.  */
                        unconfirmedOMLineDeAllocs[GPU_MOD(cycle, 2)][unconfirmedDeAllocCounters[GPU_MOD(cycle, 2)]] = streamCCom->getOMLine();

                        //  Update received unconfirmed deallocs counter.  
                        unconfirmedDeAllocCounters[GPU_MOD(cycle, 2)]++;

                        //  Reset output memory line valid bit.  
                        outputCacheValidBit[streamCCom->getOMLine()] = FALSE;

                        break;

                    case STRC_DEALLOC_OM_CONFIRM:

                        GPU_DEBUG_BOX(
                            printf("StreamerOutputCache => DEALLOC_OM_CONFIRM command received: %d.\n",
                                 streamCCom->getOMLine());
                        )

                        //  Check if any allocated output memory line the previous cycle is being confirmed.  
                        for (i = 0; i < unconfirmedDeAllocCounters[GPU_MOD(cycle + 1, 2)]; i++)
                        {
                             //  If confirmed output memory line deallocation then add to the free list.  
                             if (unconfirmedOMLineDeAllocs[GPU_MOD(cycle + 1, 2)][i] == streamCCom->getOMLine())
                             {
                                  //  Mark the OMLine as confirmed.  
                                  deAllocConfirmed[i] = true;
                    
                                  //  Search for the line in the free list.  
                                  for(i = 0, e = nextFreeOMLine, outputMemoryLine = streamCCom->getOMLine();
                                     (i < freeOutputMemoryLines) && (outputMemoryFreeList[e] != outputMemoryLine);
                                      i++, e = GPU_MOD(e + 1, outputMemorySize));

                                  //  Check if it was found.  
                                  if (i == freeOutputMemoryLines)
                                  {
                                       //  Check number of free output cache lines.  
                                       CG_ASSERT_COND(!(freeOutputMemoryLines == outputMemorySize), "All output cache lines are already free.");
                                       //  Update output memory free list.  
                                       outputMemoryFreeList[GPU_MOD(nextFreeOMLine + freeOutputMemoryLines, outputMemorySize)] = streamCCom->getOMLine();

                                       //  Update free output memory lines free counter.  
                                       freeOutputMemoryLines++;
                                  }

                             }
                        }

                        break;
                }

                //  Delete StreamController control command.  
                delete streamCCom;
            }

            //  Re-validate cache line for finally non-confirmed lines.  
            for (i = 0; i < unconfirmedDeAllocCounters[GPU_MOD(cycle + 1, 2)]; i++)
            {
                 if (!deAllocConfirmed[i])
                 {
                      outputCacheValidBit[unconfirmedOMLineDeAllocs[GPU_MOD(cycle + 1, 2)][i]] = TRUE;
                 }
            }

            //  Reset number of indices requested by StreamController fetch in the current cycle.  
            indicesRequested = 0;


            //  Read new indexes from the cmoStreamController Fetch.  
            while (streamerFetchNewIndex->read(cycle, (DynamicObject *&) streamCCom))
            {
                //  Check StreamController control command.  
                CG_ASSERT_COND(!(streamCCom->getCommand() != STRC_NEW_INDEX), "Unsupported cmoStreamController Control Command from the cmoStreamController Fetch.");
                GPU_DEBUG_BOX(
                    printf("StreamerOutputCache => NEW_INDEX command: Index %d Instance %d OFIFO %d.\n",
                        streamCCom->getIndex(), streamCCom->getInstanceIndex(), streamCCom->getOFIFOEntry());
                )

                //  Search the requested index in the output cache.  
                outputCacheHit = outputCacheSearch(streamCCom->getIndex(), streamCCom->getInstanceIndex(), outputMemoryLine);

                //  Set output cache hit attribute.  
                streamCCom->setHit(outputCacheHit);

                //  Yes, use the output memory line found in the output cache.  
                streamCCom->setOMLine(outputMemoryLine);

                //  Store the index request in the current requested index array.  
                indexReqQ[indicesRequested] = streamCCom;

                //  Update number of indices requested this cycle.  
                indicesRequested++;
            }

            //  Process the requested indices.  Process hits.  
            for (i = 0; i < indicesRequested; i++)
            {
                //  Was a hit?  
                if (indexReqQ[i]->isAHit())
                {
                    //  Get the StreamController control command for the index request.  
                    streamCCom = indexReqQ[i];

                    GPU_DEBUG_BOX(
                        printf("StreamerOutputCache => HIT index %d instance %d.\n", streamCCom->getIndex(), streamCCom->getInstanceIndex());
                    )

                    //  Get the output memory line that was found for the index.  
                    outputMemoryLine = streamCCom->getOMLine();

                    //  Search for the line in the free list.  
                    for(j = 0, e = nextFreeOMLine;
                        (j < freeOutputMemoryLines) && (outputMemoryFreeList[e] != outputMemoryLine);
                        j++, e = GPU_MOD(e + 1, outputMemorySize));

                    //  Check if it was found.  
                    if (j < freeOutputMemoryLines)
                    {
                        //  Move all the other lines a position in the list.  
                        ePrev = e;
                        e = GPU_MOD(e + 1, outputMemorySize);
                        j++;

                        //  Remove the line from the free list.  
                        for(; j < freeOutputMemoryLines; j++)
                        {
                            //  Move line.  
                            outputMemoryFreeList[ePrev] = outputMemoryFreeList[e];

                            //  Keep previous address.  
                            ePrev = e;

                            //  Calculate next address.  
                            e = GPU_MOD(e + 1, outputMemorySize);
                        }

                        //  Update number of free lines.  
                        freeOutputMemoryLines--;
                    }

                    //  Update statistics.  
                    hits->inc();
                }
            }

            //  Process the requested indices.  Process those who missed.  
            for(i = 0, solvedMisses = 0; i < indicesRequested; i++)
            {
                //  Was a hit?  
                if(!indexReqQ[i]->isAHit())
                {
                    //  No, allocate a free output memory line for this index.  

                    //  Get the StreamController control command for the index request.  
                    streamCCom = indexReqQ[i];

                    //  Check there are free output lines.  
                    CG_ASSERT_COND(!(freeOutputMemoryLines == 0), "No free output memory lines.");
                    //  Search the index in the solved misses table.  
                    for(j = 0, index = streamCCom->getIndex(), instance = streamCCom->getInstanceIndex();
                        (j < solvedMisses) && ((missOutputMLines[j][0] != index) || (missOutputMLines[j][1] != instance)); j++);

                    //  Check if the index was found between the solved misses.  
                    if (j < solvedMisses)
                    {
                        GPU_DEBUG_BOX(
                            printf("StreamerOutputCache => HIT (secondary) index %d instance %d.\n", streamCCom->getIndex(), streamCCom->getInstanceIndex());
                        )

                        //  Get the output memory line for the index from the solved misses table.  
                        outputMemoryLine = missOutputMLines[j][2];

                        //  Set output memory line allocated for the index.  
                        streamCCom->setOMLine(outputMemoryLine);

                        //  Set as hit.  
                        streamCCom->setHit(TRUE);

                        //  Update statistics.  
                        hits->inc();
                    }
                    else
                    {

                        GPU_DEBUG_BOX(
                            printf("StreamerOutputCache => MISS index %d instance %d.\n", streamCCom->getIndex(), streamCCom->getInstanceIndex());
                        )

                        //  Get next free output memory line.  
                        outputMemoryLine = outputMemoryFreeList[nextFreeOMLine];

                        //  Update pointer to next output memory free line.  
                        nextFreeOMLine = GPU_MOD(nextFreeOMLine + 1, outputMemorySize);

                        //  Update output memory free lines counter.  
                        freeOutputMemoryLines--;

                        //  Add solved miss to the table.  
                        missOutputMLines[solvedMisses][0] = index;
                        missOutputMLines[solvedMisses][1] = instance;
                        missOutputMLines[solvedMisses][2] = outputMemoryLine;

                        //  Update number of solved misses.  
                        solvedMisses++;

                        //  Set output memory line allocated for the index.  
                        streamCCom->setOMLine(outputMemoryLine);

                        //  Create update command to the output cache.  
                        streamCComAux = new StreamerControlCommand(STRC_UPDATE_OC,
                            streamCCom->getIndex(), streamCCom->getInstanceIndex(), outputMemoryLine);

                        //  Copy trace signal info from the new index command.  
                        streamCComAux->copyParentCookies(*streamCCom);

                        //  Add a cookie level.  
                        streamCComAux->addCookie();

                        //  Send an output cache update to the cmoStreamController Output Cache.  
                        streamerOCUpdateWrite->write(cycle, streamCComAux);

                        //  Update statistics.  
                        misses->inc();
                    }
                }
            }

            //  Send the index and the output vertex cache result to cmoStreamController Loader and/or cmoStreamController Commit.  
            for(i = 0; i < indicesRequested; i++)
            {
                //  Get the StreamController control command for the index request.  
                streamCCom = indexReqQ[i];

                //  Clone the new index StreamController control command.  
                streamCComClone = new StreamerControlCommand(STRC_NEW_INDEX,
                    streamCCom->getIndex(), streamCCom->getInstanceIndex(), streamCCom->getOFIFOEntry());

                //  Set clone hit.  
                streamCComClone->setHit(streamCCom->isAHit());

                //  Set clone last in batch flag.  
                streamCComClone->setLast(streamCCom->isLast());

                //  Set clone output memory line.  
                streamCComClone->setOMLine(streamCCom->getOMLine());

                //  Set cmoStreamController Loader destination unit.  
                streamCComClone->setUnitID(streamCCom->getUnitID());

                //  Copy signal trace info.  
                streamCComClone->copyParentCookies(*streamCCom);

                //  Send index to the corresponding cmoStreamController Loader Unit.  
                streamerLoaderNewIndex[streamCComClone->getUnitID()]->write(cycle, streamCComClone);

                //  Send index to the StreamController commit.  
                streamerCommitNewIndex->write(cycle, streamCCom);

                // Reset index request entry.  
                indexReqQ[i] = NULL;

                //  Update statistics.  
                indices->inc();
            }

            //  Read commands from the main cmoStreamController mdu.  
            if (streamerCommand->read(cycle, (DynamicObject *&) streamCom))
                processStreamerCommand(streamCom);

            break;

        default:
            CG_ASSERT("Unsupported cmoStreamController state.");
            break;

    }
}

//  Process StreamController command.  
void StreamerOutputCache::processStreamerCommand(StreamerCommand *streamCom)
{
    U32 i;

    //  Delete last StreamController command.  
    delete lastStreamCom;

    //  Keep the new StreamController command for signal tracing purposes.  
    lastStreamCom = streamCom;

    //  Process command.  
    switch(streamCom->getCommand())
    {
        case STCOM_RESET:

            GPU_DEBUG_BOX( printf("StreamerOutputCache => Received RESET command.\n"); )

            //  Set state as reset.  
            state = ST_RESET;

            break;

        case STCOM_START:

            GPU_DEBUG_BOX( printf("StreamerOutputCache => Received START command.\n"); )

            //  Set all output cache valid bits to FALSE.  
            for (i = 0; i < outputMemorySize; i++)
            {
                outputCacheTagsIndex[i] = 0;
                outputCacheTagsInstance[i] = 0;
                outputCacheValidBit[i] = FALSE;
            }

            //  Fill the output memory free list.  
            for (i = 0; i < outputMemorySize; i++)
                outputMemoryFreeList[i] = i;

            //  Reset first free output memory list pointer.  
            nextFreeOMLine = 0;

            //  Reset free output memory lines counter.  
            freeOutputMemoryLines = outputMemorySize;

            //  Set state to streaming.  
            state = ST_STREAMING;

            break;

        case STCOM_END:

            CG_ASSERT_COND(!(state != ST_STREAMING), "");
            GPU_DEBUG_BOX( printf("StreamerOutputCache => Received END command.\n"); )

            //  Set state to ready.  
            state = ST_READY;

            break;

        default:
            CG_ASSERT("cmoStreamController Command not supported.");
            break;
    }
}

//  Searches in the output cache for an index.  
bool StreamerOutputCache::outputCacheSearch(U32 index, U32 instance, U32 &omLine)
{
    U32 i;

    //  Search in the output cache tag table.  
    for(i = 0; (i < outputMemorySize) && ((outputCacheTagsIndex[i] != index) || (outputCacheTagsInstance[i] != instance) || !outputCacheValidBit[i]); i++);

    //  Check if the index was found.  
    if (i == outputMemorySize)
    {
        //  Not found.  
        return FALSE;
    }
    else
    {
        //  Return the output memory line that stores the output for that index.  
        omLine = i;

        //  Found.  
        return TRUE;
    }
}
