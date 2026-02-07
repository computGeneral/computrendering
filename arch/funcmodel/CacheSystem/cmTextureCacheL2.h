/**************************************************************************
 *
 * Texture Cache class definition file.
 *
 */

/**
 *
 *  @file cmTextureCacheL2.h
 *
 *  Defines the Texture Cache class.  This class defines the cache used
 *  to access texture data.
 *
 */


#ifndef _TEXTURECACHE2_

#define _TEXTURECACHE2_

#include "GPUType.h"
#include "cmFetchCache64.h"
#include "cmFetchCache.h"
//#include "cmMemoryController.h"
#include "cmMemoryTransaction.h"
#include "cmStatisticsManager.h"
#include "cmTextureCacheGen.h"
#include "cmtoolsQueue.h"
#include <string>

namespace cg1gpu
{

//  Defines a block read request to memory.  
struct TextureCacheL2ReadRequest
{
    U64 address;     //  Address to read from in the texture address space.  
    U32 memAddress;  //  Address in the GPU memory address space.  
    U32 size;        //  Size of the read request.  
    U32 requested;   //  Bytes requested to memory.  
    U32 received;    //  Bytes received from memory.  
    Cache64Request *request;    //  Pointer to the cache request.  
    CacheRequest *requestL1;    //  Pointer to the cache request.  
    U32 requestID;   //  Identifier of the cache request.  
    U32 way;         //  Way in the L1 cache for the request data.  
    U32 line;        //  Line (inside way) in the L1 cache for the request data.  
};

/**
 *
 *  This class describes and implements the behaviour of the cache
 *  used to get texture data from memory.
 *  The texture cache is used in the Texture Unit GPU unit.
 *
 *  This class uses the Fetch Cache.
 *
 */

class TextureCacheL2 : public TextureCacheGen
{

private:

    //  Texture cache identification.  
    static U32 cacheCounter;     //  Class variable used to count and create identifiers for the created Texture Caches.  
    U32 cacheID;                 //  Identifier of the texture cache.  

    //  Texture cache parameters.  
    U32 lineSizeL0;      //  Size of a cache line/block in L0.  
    U32 portWidth;       //  Size of the cache read/write ports in bytes.  
    U32 inputRequestsL0; //  Number of pending read requests and input buffers for L0 cache.  
    U32 banks;           //  Number of supported banks.  
    U32 maxAccesses;     //  Number of accesses allowed to a bank in a cycle.  
    U32 bankWidth;       //  Width in bytes of each bank word.  
    U32 maxMisses;       //  Number of misses allowed per cycle.  
    U32 decomprLatency;  //  Cycles it takes to decompress a block.  
    U32 lineSizeL1;      //  L1 cache line size.  
    U32 inputRequestsL1; //  Number of pending read requests supported for L1 cache.  

    //  Texture cache structures.  
    FetchCache64 *cacheL0;  //  Pointer to the L0 Texture Cache fetch cache.  
    FetchCache *cacheL1;    //  Pointer to the L1 Texture Cache fetch cache.  

    //  Texture cache state.  
    bool resetMode;                 //  Reset mode flag.  
    Cache64Request *cacheRequestL0; //  Last cache request for L0.  
    CacheRequest *cacheRequestL1;   //  Last cache request for L1.  
    U32 requestIDL0;             //  Last cache request identifier for L0.  
    U32 requestIDL1;             //  Last cache request identifier for L1.  
    MemState memoryState;           //  Current state of the memory controller.  
    U32 lastSize;                //  Size of the last memory read transaction received.  
    U32 readTicket;              //  Ticket of the memory read transaction being received.  
    bool memoryRead;                //  There is a memory read transaction in progress.  
    MemoryTransaction *nextTransaction;     //  Stores the pointer to the new generated memory transaction.  
    bool writingLine;               //  A full line is being written to the cache.  
    bool wasLineFilled;             //  Stores if a line was filled in the current cycle.  
    U64 notifyTag;               //  Stores the next tag to notify as filled.  
    U64 lastFilledLineTag;       //  Stores the tag for the last line filled.  

    //  Memory Structures.  
    TextureCacheL2ReadRequest *readQueueL0; //  Read request queue for L0.  
    U32 uncompressedL0;      //  Number of compressed requests.  
    U32 nextUncompressedL0;  //  Pointer to the next compressed request.  
    U32 fetchInputsL0;       //  Number of L0 input requests waiting for L1 fetch.  
    U32 nextFetchInputL0;    //  Next L0 input request to be fetched from L1.  
    U32 waitReadInputsL0;    //  Number of L0 input requests waiting for L1 read.  
    U32 nextReadInputL0;     //  Next L0 input request to be read from L1.  
    U32 readInputsL0;        //  Number of L0 inputs already read from cache L1.  
    U32 nextReadL0;          //  Pointer to the next L0 read input.  
    U32 freeReadsL0;         //  Number of free entries in the L0 read queue.  
    U32 nextFreeReadL0;      //  Pointer to the next free L0 read queue entry.  
    U32 uncompressingL0;     //  Number of blocks being uncompressed.  
    U32 readsWritingL0;      //  Number of blocks being written to a cache line.  
    U08 **inputBufferL0;      //  Buffer for the cache line being written to the L0 cache.  
    U08 *comprBuffer;         //  Auxiliary buffer from where to decompress compressed texture lines.  

    U32 memoryRequest[MAX_MEMORY_TICKETS];   //  Associates memory tickets (identifiers) to a read queue entry.  
    U64 memRStartCycle[MAX_MEMORY_TICKETS];  //  Cycle at which a memory request with a given ticket (identifier) was sent to the memory controller.  
    tools::Queue<U32> ticketList;            //  List with the memory tickets available to generate requests to memory.  
    U32 freeTickets;                         //  Number of free memory tickets.  

    TextureCacheL2ReadRequest *readQueueL1; //  L1 read request queue.  
    U32 inputsL1;            //  Number of L1 input requests.  
    U32 nextInputL1;         //  Pointer to the next L1 request to be request from memory.  
    U32 readInputsL1;        //  Number of L1 requests already received from memory.  
    U32 inputsRequestedL1;   //  Number of L0 input requests requested to memory.  
    U32 nextReadInputL1;     //  Pointer to the next L1 request to fill L1 cache.  
    U32 nextFreeReadL1;      //  Pointer to the next free L1 read queue entry.  
    U32 freeReadsL1;         //  Number of free entries in the L1 read queue.  
    U08 **inputBufferL1;      //  Buffer the cache lines being written to the L1 cache.  

    //  Resource limits.  
    U32 *dataBankAccess;     //  Number of accesses in the current cycle to each texture cache memory bank.  
    U32 *tagBankAccess;      //  Number of accesses in the current cycle to each texture cache tag bank.  
    U32 cycleMisses;         //  Number of misses in the current cycle.  
    U64 **fetchAccess;       //  Stores the line addresses fetched in a cycle for each texture cache bank, used to eliminate redundant fetches.  
    U64 **readAccess;        //  Stores the addresses read in a cycle for each texture cache bank, used for data bypasses.  

    //  Precalculate values.  
    U32 lineShift;           //  Binary shift to obtain the line from an address.  
    U32 bankShift;           //  Binary shift to obtain the bank from an address.  
    U32 bankMask;            //  Binary mask to obtain the mask from an address.  
    U32 readPorts;           //  Number of read ports in the Texture Cache.  

    //  Timing.  
    U32 writeCycles;         //  Remaining cycles in the cache write port.  
    U32 *readCycles;         //  Remaining cycles the cache read port is reserved.  
    U32 memoryCycles;        //  Remaining cycles the memory bus is used/reserved.  
    U32 uncompressCycles;    //  Remaining cycles for the compression of the current block.  

    //  Debug.
    bool debugMode;             //  Flag that stores if debug output is enabled.  

    //  Statistics.  
    gpuStatistics::Statistic *fetchBankConflicts;   //  Counts bank conflicts when fetching.  
    gpuStatistics::Statistic *readBankConflicts;    //  Counts bank conflicts when reading.  
    gpuStatistics::Statistic *memRequests;          //  Memory requests served by the Memory Controller.  
    gpuStatistics::Statistic *memReqLatency;        //  Latency of the memory requests served by the Memory Controller.  
    gpuStatistics::Statistic *pendingRequests;      //  Number of memory requests issued to the Memory Controller and not served.  
    gpuStatistics::Statistic *pendingRequestsAvg;   //  Average number of memory requests issued to the Memory Controller and not served.  

    //  Private functions.  

    /**
     *
     *  Generates a memory transaction requesting data to the memory controller for the read queue entry.
     *
     *  @param readReq The read queue entry for which to generate the memory transaction.
     *
     *  @return The memory transaction generated.  If it could not be generated returns NULL.
     *
     */

    MemoryTransaction *requestBlock(U32 readReq);

    /**
     *
     *  Generates a write memory transaction for the write queue entry.
     *
     *  @param writeReq Write queue entry for which to generate the memory transaction.
     *
     *  @return If a memory transaction could be generated, the transaction generated, otherwise NULL.
     *
     */

    MemoryTransaction *writeBlock(U32 writeReq);


public:

    /**
     *
     *  Texture Cache constructor.
     *
     *  Creates and initializes a Texture Cache object.
     *
     *  @param numWaysL0 Number of ways in the L0 cache.
     *  @param numLinesL0 Number of lines in the L0 cache.
     *  @param lineSizeL0 Bytes per L0 cache line.
     *  @param portWidth Width in bytes of the write and read ports.
     *  @param reqQueueSize Input cache request to memory queue size.
     *  @param inputRequestsL0 Number of read requests and input buffers for L0 cache.
     *  @param numBanks Number of banks in the texture cache.
     *  @param maxAccess Maximum number of accesses to a bank in a cycle.
     *  @param bWidth Width in bytes of each cache bank.
     *  @param maxMiss Maximum number of misses in a cycle.
     *  @param decomprLat Decompression latency for texture compressed blocks.
     *  @param numWaysL1 Number of ways in the L1 cache.
     *  @param numLinesL1 Number of lines in the L1 cache.
     *  @param lineSizeL1 Bytes per L1 cache line.
     *  @param inputRequestsL1 Number of pending read requests for L1 cache.
     *  @param postfix The postfix for the texture cache statistics.
     *
     *  @return A new initialized cache object.
     *
     */

    TextureCacheL2(U32 numWaysL0, U32 numLinesL0, U32 lineSizeL0,
        U32 portWidth, U32 reqQueueSize, U32 inputRequests,
        U32 numBanks, U32 numAccess, U32 bWidth, U32 maxMiss,
        U32 decomprLat, U32 numWaysL1, U32 numLinesL1, U32 lineSizeL1,
        U32 inputRequestsL1, char *postfix);

    /**
     *
     *  Reserves and fetches (if the line is not already available) the cache line for the requested address.
     *
     *  @param address The address in GPU memory of the data to read.
     *  @param way Reference to a variable where to store the way in which the data for the
     *  fetched address will be stored.
     *  @param line Reference to a variable where to store the cache line where the fetched data will be stored.
     *  @param tag Reference to a variable where to store the line tag to wait for the fetched address.
     *  @param ready Reference to a variable where to store if the data for the address is already available.
     *  @param source Pointer to a DynamicObject that is the source of the fetch request.
     *
     *  @return If the line for the address could be reserved and fetched (ex. all line are already reserved).
     *
     */

    bool fetch(U64 address, U32 &way, U32 &line, U64 &tag, bool &ready, DynamicObject *source);

   /**
     *
     *  Reads texture data data from the texture cache.
     *  The line associated with the requested address must have been previously fetched, if not an error
     *  is produced.
     *
     *  @param address Address of the data in the texture cache.
     *  @param way Way where the data for the address is stored.
     *  @param line Cache line where the data for the address is stored.
     *  @param size Amount of bytes to read from the texture cache.
     *  @param data Pointer to an array where to store the read color data for the stamp.
     *
     *  @return If the read could be performed (ex. line not yet received from memory).
     *
     */

    bool read(U64 address, U32 way, U32 line, U32 size, U08 *data);

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
     *  Process a received memory transaction from the Memory Controller.
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
     *  @param filled Reference to a boolean variable where to store if a cache line
     *  was filled this cycle.
     *  @param tag Reference to a variable where to store the tag for the cache line
     *  filled in the current cycle.
     *
     *  @return A new memory transaction to be issued to the
     *  memory controller.
     *
     */

    MemoryTransaction *update(U64 cycle, MemState memoryState, bool &filled, U64 &tag);

    /**
     *
     *  Simulates a cycle of the texture cache.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void clock(U64 cycle);

    /**
     *
     *  Writes into a string a report about the stall condition of the mdu.
     *
     *  @param cycle Current simulation cycle.
     *  @param stallReport Reference to a string where to store the stall state report for the mdu.
     *
     */
     
    void stallReport(U64 cycle, std::string &stallReport);

    /**
     *
     *  Enables or disables debug output for the texture cache.
     *
     *  @param enable Boolean variable used to enable/disable debug output for the TextureCache.
     *
     */

    void setDebug(bool enable);
};

} // namespace cg1gpu

#endif
