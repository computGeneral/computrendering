/**************************************************************************
 *
 * cmoStreamController Output Cache class definition file.
 *
 */


/**
 *
 *  @file cmStreamerOutputCache.h
 *
 *  This file contains the definition of the cmoStreamController Output Cache
 *  mdu.
 *
 */


#ifndef _STREAMEROUTPUTCACHE_

#define _STREAMEROUTPUTCACHE_

namespace arch
{
    class StreamerOutputCache;
} // namespace arch;


#include "GPUType.h"
#include "cmMduBase.h"
#include "cmStreamer.h"
#include "cmStreamerCommand.h"
#include "cmStreamerControlCommand.h"

namespace arch
{

/**
 *
 *  This class implements the cmoStreamController Output Cache mdu.
 *
 *  The cmoStreamController Output Cache receives new input indexes from
 *  the cmoStreamController Fetch and look for them in the output cache.
 *  Then sends the new index, a hit/miss flag and the output
 *  memory line allocated for the index to the cmoStreamController Commit
 *  and cmoStreamController Loader.
 *
 *  This class inherits from the cmoMduBase class that offers basic
 *  simulation support.
 *
 */

class StreamerOutputCache : public cmoMduBase
{

private:

    //  cmoStreamController Output Cache signals.  
    Signal *streamerCommand;        //  Command signal from the cmoStreamController main mdu.  
    Signal *streamerFetchNewIndex;  //  New Index signal from the cmoStreamController Fetch.  
    Signal *streamerCommitNewIndex; //  New Index signal to the cmoStreamController Commit.  
    Signal **streamerLoaderNewIndex;//  New Index signal to the cmoStreamController Loader Units.  
    Signal *streamerCommitDeAlloc;  //  Deallocation signal from the cmoStreamController Commit.  
    Signal *streamerOCUpdateWrite;  //  cmoStreamController Output Cache update signal to the cmoStreamController Output Cache.  
    Signal *streamerOCUpdateRead;   //  cmoStreamController Output Cache update signal from the cmoStreamController Output Cache.  

    //  cmoStreamController Output Cache parameters.  
    U32 indicesCycle;            //  Indices received per cycle from cmoStreamController Fetch.  
    U32 outputMemorySize;        //  Size in lines (outputs) of the output memory.  
    U32 verticesCycle;           //  Vertices per cycle commited and sent to Primitive Assembly by cmoStreamController Commit.  
    U32 streamerLoaderUnits;     //  Number of cmoStreamController Loader Units. 
    U32 slIndicesCycle;          //  Indices consumed each cycle per cmoStreamController Loader Unit. 

    //  cmoStreamController Output Cache state.  
    StreamerState state;                //  State of the StreamController output cache.  
    StreamerCommand *lastStreamCom;     //  Last StreamController command received.  For signal tracing.  
    StreamerControlCommand **indexReqQ; //  Array storing the indices requested in the current cycle by cmoStreamController Fetch.  
    U32 indicesRequested;            //  Number of indices requested to the cache in the current cycle.  
    U32 **missOutputMLines;          //  Array storing the output cache line allocated for index misses already processed in the current cycle.  
    U32 solvedMisses;                //  Number of misses already solved (excluded repeated misses).  

    //  cmoStreamController Output Cache structures.  
    U32 *outputCacheTagsIndex;       //  Output cache tags (index).  
    U32 *outputCacheTagsInstance;    //  Output cache tags (instance index).  
    bool *outputCacheValidBit;          //  Output cache valid bit.  
    U32 *outputMemoryFreeList;       //  List of free output memory lines.  
    U32 freeOutputMemoryLines;       //  Number of free output memory lines.  
    U32 nextFreeOMLine;              //  Pointer to the next free output memory line in the output memory free list.  
    U32 **unconfirmedOMLineDeAllocs; //  Array of deallocated output memory lines from cmoStreamController Commit pending of confirmation.  
    U32 *unconfirmedDeAllocCounters; //  Number of deallocated output memory lines from cmoStreamController Commit pending of confirmation.  
    bool *deAllocConfirmed;             //  Array of confirmation status of deallocated lines.  

    //  Statistics.  
    gpuStatistics::Statistic *hits;     //  Number of hits in the cmoStreamController Output Cache.  
    gpuStatistics::Statistic *misses;   //  Number of misses in the cmoStreamController Output Cache.  
    gpuStatistics::Statistic *indices;  //  Number of indices sent to cmoStreamController Output Cache.  

    //  Private functions.  

    /**
     *
     *  Processes a StreamController command.
     *
     *  @param streamComm The StreamController command to process.
     *
     */

    void processStreamerCommand(StreamerCommand *streamCom);

    /**
     *
     *  Searches in the output cache tag table for an index.
     *
     *  @param index The index to search.
     *  @param instance The instance index corresponding to the index to search.
     *  @param omLine Reference to an U32 variable where to store
     *  the output memory line where the index output is stored
     *  if the index was found.
     *
     *  @return TRUE if the index was found in the tag table,
     *  FALSE otherwise.
     *
     */

    bool outputCacheSearch(U32 index, U32 instance, U32 &omLine);


public:

    /**
     *
     *  cmoStreamController Output Cache mdu constructor.
     *
     *  @param idxCycle Indices received per cycle from cmoStreamController Fetch.
     *  @param omSize The output memory size in lines.
     *  @param vertCycle Vertices per cycle that cmoStreamController Commit can commit and send to Primitive Assembly.
     *  @param sLoaderUnits Number of cmoStreamController Loader Units.
     *  @param slIdxCycle Indices consumed each cycle per cmoStreamController Loader Unit.
     *  @param name The mdu name.
     *  @param parent The mdu parent mdu.
     *
     *  @return An initialized cmoStreamController Output Cache object.
     *
     */

    StreamerOutputCache(U32 idxCycle, U32 omSize, U32 vertCycle, U32 sLoaderUnits, 
      U32 slIdxCycle, char *name, cmoMduBase *parent);

    /**
     *
     *  Simulation rutine for the cmoStreamController Output Cache mdu.
     *
     *  @param cycle Cycle to simulate.
     *
     *
     */

    void clock(U64 cycle);

};

} // namespace arch

#endif
