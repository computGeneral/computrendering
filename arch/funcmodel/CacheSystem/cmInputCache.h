/**************************************************************************
 *
 * Input Cache class definition file.
 *
 */

/**
 *
 *  @file cmInputCache.h
 *
 *  Defines the Input Cache class.  This class defines the cache used
 *  by the cmoStreamController to load vertex inputs from memory.
 *
 */


#ifndef _INPUTCACHE_

#define _INPUTCACHE_

#include "GPUType.h"
#include "cmFetchCache.h"
//#include "cmMemoryController.h"
#include "cmMemoryTransaction.h"
#include "cmStatisticsManager.h"
#include "cmtoolsQueue.h"

namespace cg1gpu
{

//  Defines a block read request to memory.  
struct InputCacheReadRequest
{
    U32 address;     //  Address to read from.  
    U32 size;        //  Size of the read request.  
    U32 requested;   //  Bytes requested to memory.  
    U32 received;    //  Bytes received from memory.  
    CacheRequest *request;  //  Pointer to the cache request.  
    U32 requestID;   //  Identifier of the cache request.  
};

/**
 *
 *  This class describes and implements the behaviour of the cache
 *  used to get vertex input data from memory.
 *  The input cache is used in the cmoStreamController Loader GPU unit.
 *
 *  This class uses the Fetch Cache.
 *
 */

class InputCache
{

private:

    U32 cacheId;         //  The input cache identifier.  

    //  Input cache parameters.  
    U32 lineSize;        //  Size of a cache line/block.  
    U32 numPorts;        //  Number of read/write ports.  
    U32 portWidth;       //  Size of the cache read/write ports in bytes.  
    U32 inputRequests;   //  Number of read requests and input buffers.  

    //  Input cache structures.  
    FetchCache *cache;      //  Pointer to the Input Cache fetch cache.  

    //  Input cache state.  
    bool resetMode;                     //  Reset mode flag.  
    CacheRequest *cacheRequest;         //  Last cache request received.  
    U32 requestID;                   //  Current cache request identifier.  
    MemState memoryState;               //  Current state of the memory controller.  
    U32 lastSize;                    //  Size of the last memory read transaction received.  
    U32 readTicket;                  //  Ticket of the memory read transaction being received.  
    bool memoryRead;                    //  There is a memory read transaction in progress.  
    MemoryTransaction *nextTransaction; //  Stores the pointer to the new generated memory transaction.  
    bool writingLine;                   //  A full line is being written to the cache.  

    //  Memory Structures.  
    InputCacheReadRequest *readQueue;   //  Read request queue.  
    U32 inputs;              //  Number of input requests.  
    U32 nextInput;           //  Next input request.  
    U32 readInputs;          //  Number of inputs already read from the cache.  
    U32 nextRead;            //  Pointer to the next read input.  
    U32 freeReads;           //  Number of free entries in the read queue.  
    U32 nextFreeRead;        //  Pointer to the next free read queue entry.  
    U32 inputsRequested;     //  Number of input requests requested to memory.  
    U32 readsWriting;        //  Number of blocks being written to a cache line.  
    U08 **inputBuffer;        //  Buffer for the cache line being written to the cache.  
    U32 memoryRequest[MAX_MEMORY_TICKETS];   //  Associates memory tickets (identifiers) to a read queue entry.  
    tools::Queue<U32> ticketList;            //  List with the memory tickets available to generate requests to memory.  
    U32 freeTickets;                         //  Number of free memory tickets.  

    //  Timing.  
    U32 writeCycles;     //  Remaining cycles in the cache write port.  
    U32 *readCycles;     //  Remaining cycles the cache read port is reserved.  
    U32 nextReadPort;    //  Next read port.  
    U32 memoryCycles;    //  Remaining cycles the memory bus is used/reserved.  

    //  Statistics.  
    //gpuStatistics::Statistic *readTrans;    //  Read transactions to memory.  
    //gpuStatistics::Statistic *writeTrans;   //  Write transactions to memory.  
    //gpuStatistics::Statistic *readMem;      //  Read bytes from memory.  
    //gpuStatistics::Statistic *writeMem;     //  Write bytes to memory.  
    //gpuStatistics::Statistic *fetches;      //  Fetch operations.  
    //gpuStatistics::Statistic *fetchOK;      //  Successful fetch operations.  
    //gpuStatistics::Statistic *fetchFail;    //  Failed fetch operation.  
    //gpuStatistics::Statistic *allocOK;      //  Successful allocation operation.  
    //gpuStatistics::Statistic *allocFail;    //  Failed allocation operation.  
    //gpuStatistics::Statistic *readOK;       //  Succesful read operation.  
    //gpuStatistics::Statistic *readFail;     //  Failed read operation.  
    //gpuStatistics::Statistic *writeOK;      //  Succesful write operation.  
    //gpuStatistics::Statistic *writeFail;    //  Failed write operation.  

    //  Private functions.  

    /**
     *
     *  Generates a memory transaction requesting data to
     *  the memory controller for the read queue entry.
     *
     *  @param readReq The read queue entry for which to generate
     *  the memory transaction.
     *
     *  @return The memory transaction generated.  If it could not
     *  be generated returns NULL.
     *
     */

    MemoryTransaction *requestBlock(U32 readReq);

    /**
     *
     *  Generates a write memory transaction for the write queue
     *  entry.
     *
     *  @param writeReq Write queue entry for which to generate
     *  the memory transaction.
     *
     *  @return If a memory transaction could be generated, the
     *  transaction generated, otherwise NULL.
     *
     */

    MemoryTransaction *writeBlock(U32 writeReq);


public:

    /**
     *
     *  Input Cache constructor.
     *
     *  Creates and initializes a Input Cache object.
     *
     *  @param cacheId The input cache identifier.
     *  @param numWays Number of ways in the cache.
     *  @param numLines Number of lines in the cache.
     *  @param lineSize Bytes per cache line.
     *  @param ports Number of read/write ports.
     *  @param portWidth Width in bytes of the write and read ports.
     *  @param reqQueueSize Input cache request to memory queue size.
     *  @param inputRequests Number of read requests and input buffers.
     *
     *  @return A new initialized cache object.
     *
     */

    InputCache(U32 cacheId, U32 numWays, U32 numLines, U32 lineSize,
        U32 ports, U32 portWidth, U32 reqQueueSize, U32 inputRequests);

    /**
     *
     *  Reserves and fetches (if the line is not already available)
     *  the cache line for the requested address.
     *
     *  @param address The address in GPU memory of the data to read.
     *  @param way Reference to a variable where to store the
     *  way in which the data for the fetched address will be stored.
     *  @param line Reference to a variable where to store the
     *  cache line where the fetched data will be stored.
     *  @param source Pointer to a Dynamic Object that is the source of the cache access.
     *
     *
     *  @return If the line for the address could be reserved and
     *  fetched (ex. all line are already reserved).
     *
     */

    bool fetch(U32 address, U32 &way, U32 &line, DynamicObject *source);

   /**
     *
     *  Reads vertex input data data from the input cache.
     *  The line associated with the requested address
     *  must have been previously fetched, if not an error
     *  is produced.
     *
     *  @param address Address of the data in the input cache.
     *  @param way Way where the data for the address is stored.
     *  @param line Cache line where the data for the address
     *  is stored.
     *  @param size Amount of bytes to read from the input cache.
     *  @param data Pointer to an array where to store the read
     *  color data for the stamp.
     *
     *  @return If the read could be performed (ex. line not
     *  yet received from memory).
     *
     */

    bool read(U32 address, U32 way, U32 line, U32 size, U08 *data);

    /**
     *
     *  Unreserves a cache line.
     *
     *  @param way The way of the cache line to unreserve.
     *  @param line The cache line to unreserve.
     *
     */

    void unreserve(U32 way, U32 line);

    /**
     *
     *  Resets the Input Cache structures.
     *
     *
     */

    void reset();

    /**
     *
     *  Process a received memory transaction from the
     *  Memory Controller.
     *
     *  @param memTrans Pointer to a memory transaction.
     *
     */

    void processMemoryTransaction(MemoryTransaction *memTrans);

    /**
     *
     *  Updates the state of the memory request queue.
     *
     *  @param cycle Current simulation cycle.
     *  @param memoryState Current state of the Memory Controller.
     *
     *  @return A new memory transaction to be issued to the
     *  memory controller.
     *
     */

    MemoryTransaction *update(U64 cycle, MemState memoryState);

    /**
     *
     *  Simulates a cycle of the input cache.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void clock(U64 cycle);

};

} // namespace cg1gpu

#endif
