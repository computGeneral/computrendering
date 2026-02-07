/**************************************************************************
 *
 * Color Cache class definition file.
 *
 */

/**
 *
 *  @file cmColorCache.h
 *
 *  Defines the Color Cache class.  This class defines the cache used
 *  to access the color buffer in a GPU.
 *
 */


#ifndef _COLORCACHE_

#define _COLORCACHE_

#include "GPUType.h"
#include "cmFetchCache.h"
#include "cmROPCache.h"
#include "cmMemoryTransaction.h"

namespace cg1gpu
{

//  Defines a block memory write request.  
struct WriteRequest
{
    U32 address;     //  Address to write to.  
    U32 block;       //  Block for the address.  
    U32 size;        //  Size of the write request.  
    U32 written;     //  Bytes already written to memory.  
    CacheRequest *request;  //  Pointer to the cache request.  
    U32 requestID;   //  Identifier of the cache request.  
    U32 readWaiting; //  Read queue entry waiting for the write queue to read from the same cache line.  
    bool isReadWaiting; //  Stores if a read queue entry is waiting for this write to finish.  
};

//  Defines a block read request to memory.  
struct ReadRequest
{
    U32 address;     //  Address to read from.  
    U32 block;       //  Block for the address.  
    U32 size;        //  Size of the read request.  
    U32 requested;   //  Bytes requested to memory.  
    U32 received;    //  Bytes received from memory.  
    CacheRequest *request;  //  Pointer to the cache request.  
    U32 requestID;   //  Identifier of the cache request.  
    bool writeWait;     //  The read request must wait for a write request to read the cache line before replacing the cache line itself.  
};

//  Defines the states of a color buffer block.  
enum ColorBlockState
{
    COLOR_BLOCK_CLEAR,  //  The block is in clear state (use clear color).  
    COLOR_BLOCK_COMPRESSED_BEST,    //  The block is in best compression level.  
    COLOR_BLOCK_COMPRESSED_NORMAL,  //  The block is in normal compression level.  
    COLOR_BLOCK_UNCOMPRESSED        //  The block is uncompressed.  
};


/**
 *
 *  This class describes and implements the behaviour of the cache
 *  used to access the color buffer in a GPU.
 *  The color cache is used in the Color Write GPU unit.
 *
 *  This classes derives from the ROPCache interface class.
 *
 *  This class uses the Fetch Cache.
 *
 */

class ColorCache : public ROPCache
{

private:

    //  Color cache identification.  
    static U32 cacheCounter;     //  Class variable used to count and create identifiers for the created Color Caches.  
    U32 cacheID;                 //  Identifier of the object color cache.  

    //  Color cache parameters.  
    U32 stampBytes;      //  Color bytes per stamp.  
    U32 lineStamps;      //  Stamps per cache line.  
    U32 readPorts;       //  Number of read ports.  
    U32 writePorts;      //  Number of write ports.  
    U32 portWidth;       //  Size of the cache read/write ports in bytes.  
    U32 inputRequests;   //  Number of read requests and input buffers.  
    U32 outputRequests;  //  Number of write requests and output buffers.  
    bool disableCompr;      //  Disables color compression.  
    U32 maxBlocks;       //  Maximun number of supported blocks (cache lines) in the color buffer.  
    U32 blocksCycle;     //  Blocks modified (cleared) per cycle.  
    U32 comprLatency;    //  Cycles it takes to compress a block.  
    U32 decomprLatency;  //  Cycles it takes to decompress a block.  

    //  Color cache derived parameters.  
    U32 lineSize;        //  Size of a cache line/block.  
    U32 blockShift;      //  Number of bits to removed from a color buffer address to retrieve the block number.  

    //  Color cache registers.  
    U32 colorBufferAddress;  //  Start address of the color buffer in video memory.  
    U32 clearColor;          //  The current clear color (in RGBA format).  

    //  Color cache structures.  
    FetchCache *cache;      //  Pointer to the Color Cache fetch cache.  
    ColorBlockState *blockState;    //  Stores the current state of the blocks in the color buffer.  

    //  Color cache state.  
    bool flushRequest;      //  Flag for starting the flush.  
    bool flushMode;         //  If the cache is in flush mode.  
    bool resetMode;         //  Reset mode flag.  
    bool clearMode;         //  Clear mode flag.  
    CacheRequest *cacheRequest; //  Last cache request received.  
    U32 requestID;       //  Current cache request identifier.  
    MemState memoryState;   //  Current state of the memory controller.  
    U32 lastSize;        //  Size of the last memory read transaction received.  
    U32 readTicket;      //  Ticket of the memory read transaction being received.  
    bool memoryRead;        //  There is a memory read transaction in progress.  
    bool memoryWrite;       //  There is a memory write transaction in progress.  
    MemoryTransaction *nextTransaction;     //  Stores the pointer to the new generated memory transaction.  
    bool writingLine;       //  A full line is being written to the cache.  
    bool readingLine;       //  A full line is being read from the cache.  
    bool fetchPerformed;    //  Stores if a fetch/allocate operation was performed in the current cycle.  
    U32 writeLinePort;   //  Port being used to write a cache line.  
    U32 readLinePort;    //  Port being used to read a cache line.  

    //  Memory Structures.  
    WriteRequest *writeQueue;   //  Write request queue.  
    ReadRequest *readQueue;     //  Read request queue.  
    U32 compressed;          //  Number of compressed requests.  
    U32 uncompressed;        //  Number of uncompressed requests.  
    U32 nextCompressed;      //  Pointer to the next compressed request.  
    U32 nextUncompressed;    //  Pointer to the next uncompressed request.  
    U32 inputs;              //  Number of input requests.  
    U32 outputs;             //  Number of outputs requests.  
    U32 nextInput;           //  Next input request.  
    U32 nextOutput;          //  Next output request.  
    U32 readInputs;          //  Number of inputs already read from the cache.  
    U32 writeOutputs;        //  Number of outputs to write to cache.  
    U32 nextRead;            //  Pointer to the next read input.  
    U32 nextWrite;           //  Pointer to the next write output.  
    U32 freeWrites;          //  Number of free entries in the write queue.  
    U32 freeReads;           //  Number of free entries in the read queue.  
    U32 nextFreeRead;        //  Pointer to the next free read queue entry.  
    U32 nextFreeWrite;       //  Pointet to the next free write queue entry.  
    U32 inputsRequested;     //  Number of input requests requested to memory.  
    U32 uncompressing;       //  Number of blocks being uncompressed.  
    U32 readsWriting;        //  Number of blocks being written to a cache line.  
    U08 **inputBuffer;        //  Buffer for the cache line being written to the cache.  
    U08 **outputBuffer;       //  Buffer for the cache line being read out of the cache.  
    U32 **maskBuffer;        //  Mask buffer for masked cache lines.  
    U32 memoryRequest[MAX_MEMORY_TICKETS];   //  Associates memory tickets (identifiers) to a read queue entry.  
    U32 nextTicket;          //  Next memory ticket.  
    U32 freeTickets;         //  Number of free memory tickets.  


    //  Timing.  
    U32 nextReadPort;    //  Pointer to the next read port to be used.  
    U32 nextWritePort;   //  Pointer to the next write port to be used.  
    U32 *writeCycles;    //  Remaining cycles in the cache write port.  
    U32 *readCycles;     //  Remaining cycles the cache read port is reserved.  
    U32 memoryCycles;    //  Remaining cycles the memory bus is used/reserved.  
    U32 compressCycles;  //  Remaining cycles for the compression of the current block.  
    U32 uncompressCycles;//  Remaining cycles for the compression of the current block.  
    U32 clearCycles;     //  Remaining cycles for the clear of the block state memory.  

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

    /**
     *
     *  Translates a cache line address to a block address in the
     *  block state memory of the Color Cache.
     *
     *  @param address Address of the cache line to translate.
     *
     *  @return The address in the state memory for the block.
     *
     */

    U32 address2block(U32 address);

public:

	//  Defines the size of a compressed block using best compression level.  
	static const U32 COMPRESSED_BLOCK_SIZE_BEST = 64;

	//  Defines the size of a compressed block using normal compression level.  
	static const U32 COMPRESSED_BLOCK_SIZE_NORMAL = 128;

	//  Defines the size of an uncompressed block.  
	static const U32 UNCOMPRESSED_BLOCK_SIZE = 256;

	//  Defines compression masks and shift values.  
	static const U32 COMPRESSION_HIMASK_NORMAL = 0xffffe000L;
	static const U32 COMPRESSION_HIMASK_BEST = 0xffffffe0L;
	static const U32 COMPRESSION_LOSHIFT_NORMAL = 13;
	static const U32 COMPRESSION_LOSHIFT_BEST = 5;
	static const U32 COMPRESSION_LOMASK_NORMAL = 0x00001fffL;
	static const U32 COMPRESSION_LOMASK_BEST = 0x0000001fL;


    /**
     *
     *  Color Cache constructor.
     *
     *  Creates and initializes a Color Cache object.
     *
     *  @param numWays Number of ways in the cache.
     *  @param numLines Number of lines in the cache.
     *  @param stampsLine Number of stamps per cache line.
     *  @param bytesStamp Number of bytes per stamp.
     *  @param readPorts Number of read ports.
     *  @param writePorts Number of write ports.
     *  @param portWidth Width in bytes of the write and read ports.
     *  @param reqQueueSize Color cache request to memory queue size.
     *  @param inputRequests Number of read requests and input buffers.
     *  @param outputRequests Number of write requests and output buffers.
     *  @param disableCompr Disables color compression.
     *  @param maxBlocks Maximum number of sopported color blocks in the color buffer.
     *  @param blocksCycle Number of state block entries that can be modified (cleared) per cycle.
     *  @param compCycles Compression cycles.
     *  @param decompCycles Decompression cycles.
     *  @param postfix The postfix for the color cache statistics.
     *
     *  @return A new initialized cache object.
     *
     */

    ColorCache(U32 numWays, U32 numLines, U32 stampsLine,
        U32 bytesStamp, U32 readPorts, U32 writePorts, U32 portWidth, U32 reqQueueSize,
        U32 inputRequests, U32 outputRequests, bool disableCompr, U32 maxBlocks,
        U32 blocksCycle, U32 compCycles, U32 decompCycles, char *postfix);

    /**
     *
     *  Reserves and fetches (if the line is not already available)
     *  the cache line for the requested stamp defined by the
     *  color buffer address.
     *
     *  @param address The address of the first byte of the stamp
     *  in the color buffer.
     *  @param way Reference to a variable where to store the
     *  way in which the data for the fetched address will be stored.
     *  @param line Reference to a variable where to store the
     *  cache line where the fetched data will be stored.
     *  @param source Pointer to a Dynamic Object source of the cache access.
     *
     *
     *  @return If the line for the stamp could be reserved and
     *  fetched (ex. all line are already reserved).
     *
     */

    bool fetch(U32 address, U32 &way, U32 &line, DynamicObject *source);

    /**
     *
     *  Reserves and fetches (if the line is not already available)
     *  the cache line for the requested stamp defined by the
     *  color buffer address.
     *
     *  @param address The address of the first byte of the stamp
     *  in the color buffer.
     *  @param way Reference to a variable where to store the
     *  way in which the data for the fetched address will be stored.
     *  @param line Reference to a variable where to store the
     *  cache line where the fetched data will be stored.
     *  @param source Pointer to a Dynamic Object source of the cache access.
     *
     *  @return If the line for the stamp could be reserved and
     *  fetched (ex. all line are already reserved).
     *
     */

    bool allocate(U32 address, U32 &way, U32 &line, DynamicObject *source);

    /**
     *
     *  Reads stamp color data from the color cache.
     *  The line associated with the requested address/stamp
     *  must have been previously fetched, if not an error
     *  is produced.
     *
     *  @param address Address of the stamp in the color buffer.
     *  @param way Way where the data for the address is stored.
     *  @param line Cache line where the data for the address
     *  is stored.
     *  @param data Pointer to an array where to store the read
     *  color data for the stamp.
     *
     *  @return If the read could be performed (ex. line not
     *  yet received from video memory).
     *
     */

    bool read(U32 address, U32 way, U32 line, U08 *data);

    /**
     *
     *  Writes with mask a stamp color data to the color cache and unreserves
     *  the associated color cache line.
     *
     *  @param address The address of the first byte of the stamp
     *  in the color buffer.
     *  @param way Way where the data for the address is stored.
     *  @param line Cache line where the data for the address
     *  is stored.
     *  @param data Pointer to a buffer where the stamp color data
     *  to write is stored.
     *  @param mask Write mask (per byte).
     *
     *  @return If the write could be performed (ex. conflict using
     *  the write bus).
     *
     */

    bool write(U32 address, U32 way, U32 line, U08 *data, bool *mask);

    /**
     *
     *  Writes a stamp color data to the color cache and unreserves
     *  the associated color cache line.
     *
     *  @param address The address of the first byte of the stamp
     *  in the color buffer.
     *  @param way Way where the data for the address is stored.
     *  @param line Cache line where the data for the address
     *  is stored.
     *  @param data Pointer to a buffer where the stamp color data
     *  to write is stored.
     *
     *  @return If the write could be performed (ex. conflict using
     *  the write bus).
     *
     */

    bool write(U32 address, U32 way, U32 line, U08 *data);

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
     *  Resets the Color Cache structures.
     *
     *
     */

    void reset();

    /**
     *
     *  Writes back to memory the valid color cache lines.
     *
     *  @return If all the valid lines have been written
     *  to memory.
     *
     */

    bool flush();

    /**
     *
     *  Clears the color buffer.
     *
     *  @param clearColor Color value with which to
     *  clear the color buffer.
     *
     *  @return If the clear process has finished.
     *
     */

    bool clear(U32 clearColor);

    /**
     *
     *  Signals a swap in the color buffer address.
     *
     *  @param address New address of the color buffer.
     *
     */

    void swap(U32 address);

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
     *  Simulates a cycle of the color cache.
     *
     *  @param cycle Current simulation cycle.
     *
     */

    void clock(U64 cycle);

    /**
     *
     *  Copies the block state memory.
     *
     *  @param buffer Pointer to a Color Block State buffer.
     *  @param blocks Number of blocks to copy.
     *
     */

    void copyBlockStateMemory(ColorBlockState *buffer, U32 blocks);

};

} // namespace cg1gpu

#endif
