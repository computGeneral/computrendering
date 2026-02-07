/**************************************************************************
 *
 * cmoStreamController Fetch class definition file.
 *
 */

/**
 *
 *  @file cmStreamerFetch.h
 *
 *  This file defines the StreamerFetch mdu class.
 *
 */

#ifndef _STREAMERFETCH_

#define _STREAMERFETCH_


namespace cg1gpu
{
    class StreamerFetch;
} // namespace cg1gpu


#include "GPUType.h"
#include "cmMduBase.h"
#include "cmStreamer.h"
#include "cmStreamerControlCommand.h"
#include "cmStreamerCommand.h"
// #include "cmMemoryController.h"
#include "cmMemoryTransaction.h"
#include "cmMemoryControllerDefs.h"
#include "GPUReg.h"

namespace cg1gpu
{

/**
 *
 *  Defines the size of the memory request reorder queue for the cmoStreamController
 *  Fetch index buffer.
 */
static const U32 MAX_MEMORY_REQUESTS = 64;

/**
 *
 *  This class implements the cmoStreamController Fetch mdu.
 *
 *  This mdu simulates the load of new indexes from the
 *  memory and the fetch and allocations of new input indexes.
 *  Inherits from the cmoMduBase class that provides basic simulation
 *  support.
 *
 */

class StreamerFetch : public cmoMduBase
{

private:

    //  cmoStreamController Fetch Registers.  
    bool indexedMode;               //  Indexed mode enabled.  
    U32 indexStreamAddress;      //  The index stream address.  
    StreamData indexStreamData;     //  The index stream data format.  
    U32 streamStart;             //  Start index position (non indexed mode).  
    U32 streamCount;             //  Stream count (number of indexes/inputs) to fetch.  
    U32 streamInstances;         //  Number of instances of the current stream to process.  

    //  cmoStreamController Fetch Signals.  
    Signal *streamerFetchMemReq;    //  Request signal to the Memory Controller.  
    Signal *streamerFetchMemData;   //  Data signal from the Memory Controller.  
    Signal *streamerFetchCommand;   //  cmoStreamController Command signal from the cmoStreamController mdu.  
    Signal *streamerFetchState;     //  State signal to the cmoStreamController.  
    Signal *streamerOutputCache;    //  Signal to the StreamController output cache.  
    Signal **streamerLoaderDeAlloc; //  Deallocation signal from the StreamController loader units.  
    Signal *streamerCommitDeAlloc;  //  Deallocation signal from the StreamController commit.  

    //  cmoStreamController Fetch parameters.  
    U32 indicesCycle;            //  Number of indices to fetch per cycle.  
    U32 indexBufferSize;         //  Size in bytes of the index buffer for the index data to fetch.  
    U32 outputFIFOSize;          //  Number of output/indexes in the output FIFO.  
    U32 outputMemorySize;        //  Number of lines/outputs in the output memory.  
    U32 inputRequestQueueSize;   //  Number of inputs/indexes in the input request queue.  
    U32 verticesCycle;           //  Number of vertices that cmoStreamController Commit can deallocate per cycle.  
    U32 streamerLoaderUnits;     //  Number of cmoStreamController Loader Units.  
    U32 slIndicesCycle;          //  Number of indices per cycle processed by each cmoStreamController Loader Unit.  

    //  Index buffer structures.  
    U08 *indexBuffer;             //  The index data buffer.  
    U32 nextIndex;               //  Next index pointer/counter.  
    U32 nextFreeIndex;           //  Pointer to next index to fetch from the index buffer.  
    U32 requestedIndex;          //  Pointer to next free position in the index buffer.  
    U32 indexData;               //  Index data (in bytes) stored in the index buffer.  
    U32 requestedData;           //  Data in the index buffer requested to the memory controller.  
    U32 fetchedIndexes;          //  Total number of indexes already fetched.  
    U32 requestedIndexData;      //  Total number of index data bytes requested to memory.  
    U32 streamPaddingBytes;      //  Number of padding bytes at the start of the index stream to align to the memory transaction size.  
    U32 bufferPaddingBytes;      //  Number of padding bytes to align the index buffer read pointer to the memory transaction size.  
    bool skipPaddingBytes;          //  Flag that stores if before the next index read padding bytes must be skipped.  
    U32 readInstance;            //  Current instance being read from memory.  
    U32 fetchInstance;           //  Current instance begin fetched.  
    
    //  cmoStreamController Fetch state.  
    U32 freeOutputFIFOEntries;       //  Number of free output FIFO entries.  
    U32 freeOutputMemoryLines;       //  Number of free ouput memory lines.  
    U32* freeIRQEntries;             //  Number of free input request queue entries each cmoStreamController Loader unit.  
    U32* indicesSentThisCycle;       //  Number of indices sent the current cycle to each cmoStreamController Loader unit.  
    U32 nextFreeOFIFOEntry;          //  Next free output FIFO entry.  
    StreamerCommand *lastStreamCom;     //  Keeps the last StreamController command received for signal tracing.  
    StreamerState state;                //  State of the StreamController.  
    U32 **unconfirmedOMLineDeAllocs; //  Array of deallocated output memory lines from cmoStreamController Commit pending of confirmation.  
    U32 *unconfirmedDeAllocCounters; //  Number of deallocated output memory lines from cmoStreamController Commit pending of confirmation.  

    //  Memory access structures.  
    U32 currentTicket;           //  Current memory ticket.  
    U32 freeTickets;             //  Number of free memory tickets.  
    U32 busCycles;               //  Remaining cycles of the current memory bus transmission.  
    U32 lastSize;                //  Size of the last memory transaction received.  
    U32 readTicket;              //  Ticket of the last memory transaction received.  
    MemState memoryState;           //  Current state of the Memory Controller.  
    U32 memoryRequest[MAX_MEMORY_TICKETS];   //  Translates tickets into reorder queue entries.  
    U32 reorderQueue[MAX_MEMORY_REQUESTS];   //  Reorder queue for memory requests.  
    U32 nextCommit;              //  Pointer to the next queue entry in the reorder queue to commit.  
    U32 nextRequest;             //  Pointer to the next queue entry in the reorder queue where to store a new request.  
    U32 activeRequests;          //  Number of active memory requests.  

    //  Statistics.  
    gpuStatistics::Statistic *bytesRead;    //  Bytes read from memory.  
    gpuStatistics::Statistic *transactions; //  Number of transactions with the Memory Controller.  
    gpuStatistics::Statistic *sentIdx;      //  Number of indices sent to Output Cache.  

    //  Private functions.  

    /**
     *
     *  Processes a memory transaction received from the Memory
     *  Controller.
     *
     *  @param memTrans The memory transaction to process.
     *
     */

    void processMemoryTransaction(MemoryTransaction *memTrans);

    /**
     *
     *  Processes a StreamController command from the cmoStreamController main mdu.
     *
     *  @param streamCom Pointer to the cmoStreamController Command to process.
     *
     *
     */

    void processStreamerCommand(StreamerCommand *streamCom);

    /**
     *
     *  Processes a register write to a StreamController fetch register.
     *
     *  @param reg Register to write.
     *  @param subreg Register subregister to write.
     *  @param data Register data to write.
     *
     */

    void processGPURegisterWrite(GPURegister reg, U32 subreg, GPURegData data);


    /**
     *
     *  Process a StreamController control command from the StreamController commit
     *  and StreamController loader units.
     *
     *  @param cycle The simulation cycle
     *  @param streamCCom The StreamController control command to process.
     *
     */

    void processStreamerControlCommand(U32 cycle, StreamerControlCommand *streamCCom);

    /**
     *
     *  Converts from a stream buffer data format to a
     *  32 bit unsigned integer value.
     *
     *  @param format Stream buffer data format.
     *  @param data pointer to the data to convert.
     *
     *  @return The converted 32 bit unsigned integer.
     *
     */

    U32 indexDataConvert(StreamData format, U08 *data);

public:

    /**
     *
     *  cmoStreamController Fetch constructor.
     *
     *  Creates and initializes a cmoStreamController Fetch mdu.
     *
     *  @param idxCycle Indices to fetch/generate per cycle.
     *  @param indexBufferSize Size of the index buffer size (in bytes).
     *  @param outputFIFOSize Size of the output FIFO (entries).
     *  @param outputMemorySize Size of the output memory (lines).
     *  @param inputRequestQueueSize Size of the input request queue (IRQ) (entries).
     *  @param vertCycle Vertices per cycle that cmoStreamController Commit can deallocate per cycle.
     *  @param slPrefixArray Array with cmoStreamController Loader Units signal prefixes.
     *  @param streamLoadUnits Number of cmoStreamController Loader Units.
     *  @param slIdxCycle Indices per cycle processed per cmoStreamController Loader unit.
     *  @param name Name of the mdu.
     *  @param parent Parent mdu.
     *
     *  @return An initialized cmoStreamController Fetch mdu.
     *
     */

    StreamerFetch(U32 idxCycle, U32 indexBufferSize, U32 outputFIFOSize,
        U32 outputMemorySize, U32 inputRequestQueueSize, U32 vertCycle,
        char **slPrefixArray, U32 streamLoadUnits, U32 slIdxCycle,
        char *name, cmoMduBase* parent);

    /**
     *
     *  Simulates a cycle in the cmoStreamController Fetch mdu.
     *
     *  @param cycle The cycle to simulate.
     *
     */

    void clock(U64 cycle);

};

} // namespace cg1gpu

#endif
