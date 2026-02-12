/**************************************************************************
 *
 * Texture Cache class definition file.
 *
 */

/**
 *
 *  @file cmTextureCache.h
 *
 *  Defines the Texture Cache class.  This class defines the cache used
 *  to access texture data.
 *
 */


#ifndef _TEXTURECACHE_

#define _TEXTURECACHE_

#include "GPUType.h"
#include "cmFetchCache64.h"
//#include "cmMemoryController.h"
#include "cmMemoryTransaction.h"
#include "cmStatisticsManager.h"
#include "cmTextureCacheGen.h"
#include "cmtoolsQueue.h"
#include <string>

namespace arch
{

//  Defines a block read request to memory.  
struct TextureCacheReadRequest
{
    U64 address;     //  Address to read from in the texture address space.  
    U32 memAddress;  //  Address in the GPU memory address space.  
    U32 size;        //  Size of the read request.  
    U32 requested;   //  Bytes requested to memory.  
    U32 received;    //  Bytes received from memory.  
    Cache64Request *request;    //  Pointer to the cache request.  
    U32 requestID;   //  Identifier of the cache request.  
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

class TextureCache : public TextureCacheGen
{

private:

    //  Texture cache identification.  
    static U32 cacheCounter;     //  Class variable used to count and create identifiers for the created Texture Caches.  
    U32 cacheID;                 //  Identifier of the texture cache.  

    //  Texture cache parameters.  
    U32 lineSize;        //  Size of a cache line/block.  
    U32 portWidth;       //  Size of the cache read/write ports in bytes.  
    U32 inputRequests;   //  Number of read requests and input buffers.  
    U32 banks;           //  Number of supported banks.  
    U32 maxAccesses;     //  Number of accesses allowed to a bank in a cycle.  
    U32 bankWidth;       //  Width in bytes of each bank word.  
    U32 maxMisses;       //  Number of misses allowed per cycle.  
    U32 decomprLatency;  //  Cycles it takes to decompress a block.  

    //  Texture cache structures.  
    FetchCache64 *cache;    //  Pointer to the Texture Cache fetch cache.  

    //  Texture cache state.  
    bool resetMode;         //  Reset mode flag.  
    Cache64Request *cacheRequest;   //  Last cache request received.  
    U32 requestID;       //  Current cache request identifier.  
    MemState memoryState;   //  Current state of the memory controller.  
    U32 lastSize;        //  Size of the last memory read transaction received.  
    U32 readTicket;      //  Ticket of the memory read transaction being received.  
    bool memoryRead;        //  There is a memory read transaction in progress.  
    MemoryTransaction *nextTransaction;     //  Stores the pointer to the new generated memory transaction.  
    bool writingLine;       //  A full line is being written to the cache.  
    bool wasLineFilled;     //  Stores if a line was filled in the current cycle.  
    U64 notifyTag;       //  Stores the next tag to notify as filled.  
    U64 lastFilledLineTag;   //  Stores the tag for the last line filled.  

    //  Memory Structures.  
    TextureCacheReadRequest *readQueue; //  Read request queue.  
    U32 uncompressed;        //  Number of compressed requests.  
    U32 nextUncompressed;    //  Pointer to the next compressed request.  
    U32 inputs;              //  Number of input requests.  
    U32 nextInput;           //  Next input request.  
    U32 readInputs;          //  Number of inputs already read from the cache.  
    U32 nextRead;            //  Pointer to the next read input.  
    U32 freeReads;           //  Number of free entries in the read queue.  
    U32 nextFreeRead;        //  Pointer to the next free read queue entry.  
    U32 inputsRequested;     //  Number of input requests requested to memory.  
    U32 uncompressing;       //  Number of blocks being uncompressed.  
    U32 readsWriting;        //  Number of blocks being written to a cache line.  
    U08 **inputBuffer;        //  Buffer for the cache line being written to the cache.  
    U32 memoryRequest[MAX_MEMORY_TICKETS];   //  Associates memory tickets (identifiers) to a read queue entry.  
    U64 memRStartCycle[MAX_MEMORY_TICKETS];  //  Cycle at which a memory request with a given ticket (identifier) was sent to the memory controller.  
    tools::Queue<U32> ticketList;            //  List with the memory tickets available to generate requests to memory.  
    U32 freeTickets;                         //  Number of free memory tickets.  
    U08 *comprBuffer;         //  Auxiliary buffer from where to decompress compressed texture lines.  

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
     *  @param numWays Number of ways in the cache.
     *  @param numLines Number of lines in the cache.
     *  @param lineSize Bytes per cache line.
     *  @param portWidth Width in bytes of the write and read ports.
     *  @param reqQueueSize Input cache request to memory queue size.
     *  @param inputRequests Number of read requests and input buffers.
     *  @param numBanks Number of banks in the texture cache.
     *  @param maxAccess Maximum number of accesses to a bank in a cycle.
     *  @param bWidth Width in bytes of each cache bank.
     *  @param maxMiss Maximum number of misses in a cycle.
     *  @param decomprLat Decompression latency for texture compressed blocks.
     *  @param postfix The postfix for the texture cache statistics.
     *
     *  @return A new initialized cache object.
     *
     */

    TextureCache(U32 numWays, U32 numLines, U32 lineSize,
        U32 portWidth, U32 reqQueueSize, U32 inputRequests,
        U32 numBanks, U32 numAccess, U32 bWidth, U32 maxMiss,
        U32 decomprLat, char *postfix);

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

} // namespace arch

#endif
