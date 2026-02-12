/**************************************************************************
 *
 * Fetch Cache class definition file.  Version for 64 bit addresses.
 *
 */

/**
 *
 *  @file cmFetchCache64.h
 *
 *  Defines the Fetch Cache class.  This class defines a type of
 *  cache supporting fetch access to request cache lines before
 *  using them to memory.  Version for 64 bit addresses.
 *
 */


#ifndef _FETCHCACHE64_

#define _FETCHCACHE64_

#include "GPUType.h"
#include "CacheTemplate.h"
#include "cmStatisticsManager.h"
#include "DynamicObject.h"

namespace arch
{

/**
 *
 *  Defines an entry of the memory request queue.
 *
 */
struct Cache64Request
{
    U64 inAddress;   //  Address of the first byte of the line requested into the cache.  
    U64 outAddress;  //  Address of the first byte of the line requested out of the cache.  
    U32 line;        //  Fetch cache line for the line.  
    U32 way;         //  Fetch cache way for the line.  
    bool spill;         //  If the line is to be written back to memory.  
    bool fill;          //  If the line is going to be read from memory.  
    bool masked;        //  If it will use a masked write.  
    bool free;          //  If the entry is free.  
    DynamicObject *source;  //  Pointer to a Dynamic Object that is the source of the cache request.  
};


/**
 *
 *  This class describes and implements the behaviour of a cache
 *  that admits to fetch (or request) cache lines before using
 *  them.  Version for 64 bit addresses.
 *
 *  The fetch cache can be used for caches that can be benefited
 *  from this fetch feature.  A texture cache, a Z cache and a
 *  color cache are among those where this kind of cache would
 *  be useful.
 *
 *  This class inherits from the basic Cache class.
 *
 */

class FetchCache64 : private bmoCacheTemplate<U64>
{

private:

    //  Constants.  
    enum {
        MAX_LRU = 4     //  Defines the number of accesses remembered by the pseudo LRU replacement policy. 
    };


    //  Fetch cache parameters.  
    U32 requestQueueSize;//  Size of the memory request queue.  
    char *name;             //  Name/prefix/postfix for the fetch cache.  

    // Fetch cache structures.  
    U32 **reserve;       //  Fetch cache line reserve counter.  
    U32 **victim;        //  Victim lists per line.  
    bool **replaceLine;     //  The line is being replaced.  
    bool **dirty;           //  Line dirty bit.  Stores if the line has been modified.  
    bool **masked;          //  Masked line.  Stores if the line uses the line write mask.  
    bool ***writeMask;      //  Write mask for the bytes in a line.  
    U32 maxLRU;          //  Number of entries used in the pseudo LRU victim list (takes into account number of ways).   
    U32 firstWay;        //  First way from which to start searching a victim.  

    //  Memory request queue.  
    Cache64Request *requestQueue;   //  Memory request queue.  
    U32 freeRequests;    //  Number of free requests.  
    U32 activeRequests;  //  Number of active requests entries.  
    U32 nextFreeRequest; //  Pointer to the next free request.  
    U32 nextRequest;     //  Pointer to the next memory request.  
    U32 *freeRequestList;//  List of free memory request entries.  
    U32 *activeList;     //  List of active memory request entries.  

    //  Debug.
    bool debugMode;         //  Flag that stores if debug output is enabled.  

    //  Statistics.  
    gpuStatistics::Statistic *fetchMisses;      //  Misses produced by fetch operations.  
    gpuStatistics::Statistic *fetchHits;        //  Hits produced by fetch operations.  
    gpuStatistics::Statistic *fetchMissOK;      //  Misses produced by fetch operations that were started/queued when produced.  
    gpuStatistics::Statistic *fetchMissFail;    //  Misses produced by fetch operations that were not started/queued when produced.  
    gpuStatistics::Statistic *fetchMissFailReqQ;//  Misses produced by fetch operations that were not started/queued when produced because the cache request queue was full.  
    gpuStatistics::Statistic *fetchMissFailRes; //  Misses produced by fetch operations that were not started/queued when produced because there was no available cache line.  
    gpuStatistics::Statistic *fetchMissFailMiss;//  Misses produced by fetch operations that were not started/queued when produced because no misses were allowed.  
    gpuStatistics::Statistic *allocMisses;      //  Misses produced by alloc operations.  
    gpuStatistics::Statistic *allocHits;        //  Hits produced by alloc operations.  
    gpuStatistics::Statistic *allocMissOK;      //  Misses produced by alloc operations that were started/queued when produced.  
    gpuStatistics::Statistic *allocMissFail;    //  Misses produced by alloc operations that were not started/queued when produced.  
    gpuStatistics::Statistic *allocMissFailReqQ;//  Misses produced by alloc operations that were not started/queued when produced because the cache request queue was full.  
    gpuStatistics::Statistic *allocMissFailRes; //  Misses produced by alloc operations that were not started/queued when produced because there was no available cache line.  
    gpuStatistics::Statistic *allocMissFailMiss; //  Misses produced by alloc operations that were not started/queued when produced because no misses were allowed.  
    gpuStatistics::Statistic *readOK;           //  Succesful read operations.  
    gpuStatistics::Statistic *readFail;         //  Failed read operations.  
    gpuStatistics::Statistic *writeOK;          //  Succesful write operations.  
    gpuStatistics::Statistic *writeFail;        //  Failed write operations.  
    gpuStatistics::Statistic *readBytes;        //  Total bytes read.  
    gpuStatistics::Statistic *writeBytes;       //  Total bytes written.  
    gpuStatistics::Statistic *unreserves;       //  Unreserve operations.  

    /**
     *
     *  Updates the victim selection policy with a new access.
     *
     *  @param way Way accessed.
     *  @param line Line accessed.
     *
     */

    void access(U32 way, U32 line);

    /**
     *
     *  Returns the next victim selected by the victim policy.
     *
     *  @return The next victim way for a given line.
     *
     */

    U32 nextVictim(U32 line);


public:

    /**
     *
     *  Fetch Cache constructor.
     *
     *  Creates and initializes a Fetch Cache object.
     *
     *  @param numWays Number of ways in the cache.
     *  @param numLines Number of lines in the cache.
     *  @param lineBytes Number of bytes per cache line.
     *  @param reqQueueSize Fetch cache memory request queue size.
     *  @param name Name/prefix/postfix of the fetch cache.
     *
     *  @return A new initialized fetch cache object.
     *
     */

    FetchCache64(U32 numWays, U32 numLines, U32 lineBytes, U32 reqQueueSize,
        char *name = NULL);

    /**
     *
     *  Reserves and requests to memory (if the line is not already
     *  available) the cache line for the requested address.
     *
     *  @param address Address inside a cache line to fetch.
     *  @param way Reference to variable where to store the
     *  way where the fetched line will be found.
     *  @param line Reference to a variable where to store
     *  the line where the fetched line will be found.
     *  @param source Pointer to a Dynamic Object that is the source of the request.
     *
     *  @return If the line for the address could be reserved and
     *  fetched returns TRUE, otherwise returns FALSE
     *  (ex. all lines are already reserved).
     *
     */

    bool fetch(U64 address, U32 &way, U32 &line, DynamicObject *source);

    /**
     *
     *  Reserves and requests to memory (if the line is not already available) the cache line
     *  for the requested address.
     *
     *  @param address Address inside a cache line to fetch.
     *  @param way Reference to variable where to store the way where the fetched line will be found.
     *  @param line Reference to a variable where to store the line where the fetched line will be found.
     *  @param miss Reference to a variable storing if the Fetch Cache is allowed to fetch
     *  a line on a miss.  If the value is TRUE no fetch on miss is allowed.  The variable is
     *  then used to return if the fetch produced a miss.
     *  @param source Pointer to a Dynamic Object that is the source of the request.
     *
     *  @return If the line for the address could be reserved and fetched returns TRUE, otherwise returns FALSE
     *  (ex. all lines are already reserved).
     *
     */

    bool fetch(U64 address, U32 &way, U32 &line, bool &miss, DynamicObject *source);

    /**
     *
     *  Reserves and requests to memory (if the line is not already available) the cache line
     *  for the requested address.  This version returns the line tag and a flag telling if
     *  the data is already available in the cache for out of order read.
     *
     *  @param address Address inside a cache line to fetch.
     *  @param way Reference to variable where to store the way where the fetched line will be found.
     *  @param line Reference to a variable where to store the line where the fetched line will be found.
     *  @param miss Reference to a variable storing if the Fetch Cache is allowed to fetch
     *  a line on a miss.  If the value is TRUE no fetch on miss is allowed.  The variable is
     *  then used to return if the fetch produced a miss.
     *  @param ready Returns if the data for the address is ready to be read.
     *  @param source Pointer to a Dynamic Object that is the source of the request.
     *
     *  @return If the line for the address could be reserved and fetched returns TRUE, otherwise returns FALSE
     *  (ex. all lines are already reserved).
     *
     */

    bool fetch(U64 address, U32 &way, U32 &line, bool &miss, bool &ready, DynamicObject *source);

    /**
     *
     *  Reserves and allocates a cache line for the requested address.
     *  If the cache line had valid data a request to memory to
     *  copy back the cache line data is added to the Memory Request
     *  Queue.  Used to implement a write buffer mode.
     *
     *  @param address Address inside a cache line to fetch.
     *  @param way Reference to variable where to store the
     *  way where the fetched line will be found.
     *  @param line Reference to a variable where to store
     *  the line where the fetched line will be found.
     *  @param source Pointer to a Dynamic Object that is the source of the request.
     *
     *  @return If the line for the address could be reserved and
     *  allocated returns TRUE, otherwise returns FALSE
     *  (ex. all lines are already reserved).
     *
     */

    bool allocate(U64 address, U32 &way, U32 &line, DynamicObject *source);

    /**
     *
     *  Reads a given amount of data (not larger than a cache line)
     *  data from the fetch cache.
     *
     *  The line associated with the requested address
     *  must have been previously fetched, if not an error
     *  is produced and the program exits.
     *
     *  @param address Address of the data to read.
     *  @param way Line way which is going to be read.
     *  @param line Line inside the way which is going to be read.
     *  @param size Amount of data to read from the requested cache line.
     *  Must be a multiple of 4 bytes.
     *  @param data Pointer to an array where to store the read
     *  data.
     *
     *  @return If the read could be performed returns TRUE,
     *  in any other case returns FALSE (ex. line not yet received from
     *  video memory).
     *
     */

    bool read(U64 address, U32 way, U32 line, U32 size, U08 *data);

    /**
     *
     *  Writes a given amount of data to the fetch cache and unreserves
     *  (if it was still reserved) the associated fetch cache line.
     *
     *  @param address Address of the data to write.
     *  @param way Line way which is going to be read.
     *  @param line Line inside the way which is going to be read.
     *  @param size Amount of data to write (not larger than a cache
     *  line).  Must be a multiple of 4 bytes.
     *  @param data Pointer to a buffer where the data to write is stored.
     *
     *  @return If the write could be performed returns TRUE,
     *  otherwise returns FALSE (ex. resource conflict using the write bus).
     *
     */

    bool write(U64 address, U32 way, U32 line, U32 size, U08 *data);

    /**
     *
     *  Reads a full cache line.
     *
     *  @param way Way from which to read.
     *  @param line Line to read.
     *  @param linedata Pointer to the buffer where to store the line.
     *
     *  @return If the line could be read returns TRUE, if the
     *  cache write port is busy return FALSE.
     *
     */

    bool readLine(U32 way, U32 line, U08 *linedata);

    /**
     *
     *  Reads the cache line mask.
     *
     *  @param way Way from which to get the line mask.
     *  @param line Line from which to get the line mask.
     *  @param mask Pointer to a buffer where to store the mask.
     *
     */

    void readMask(U32 way, U32 line, U32 *mask);

    /**
     *
     *  Reset the cache line mask.
     *
     *  @param way Way for which to reset the line mask.
     *  @param line Line for which to reset the line mask.
     *
     */

    void resetMask(U32 way, U32 line);

    /**
     *
     *  Writes and replaces a full cache line.
     *
     *  @param way Way from which to read.
     *  @param line Line to read.
     *  @param linedata Pointer to the buffer where to store the line.
     *  @param tag Reference to a variable where to store the tag of the written line.
     *
     *  @return If the line could be written returns TRUE, if the
     *  cache read port is busy return FALSE.
     *
     */

    bool writeLine(U32 way, U32 line, U08 *linedata, U64 &tag);

    /**
     *
     *  Masked data write to the fetch cache. Writes the data (using
     *  the mask) and liberates (if it was still reserved) the
     *  associated fetch cache line.
     *
     *  @param address Address of data to write.
     *  @param way Line way which is going to be read.
     *  @param line Line inside the way which is going to be read.
     *  @param size Amount of data to write (not larger than a cache
     *  line).
     *  @param data Pointer to a buffer where the data to write is stored.
     *  @param mask Write mask (per byte).
     *
     *  @return If the write could be performed returns TRUE,
     *  otherwise returns FALSE (ex. resource conflict using the write bus).
     *
     */

    bool write(U64 address, U32 way, U32 line, U32 size,
        U08 *data, bool *mask);

    /**
     *
     *  Liberate a fetch cache line.
     *
     *  @param way Line way that is going to be liberated/unreserved.
     *  @param line Line inside the way that is going to be liberated/unreserved.
     *
     *
     */

    void unreserve(U32 way, U32 line);

    /**
     *
     *  Resets the Fetch Cache structures.
     *
     *  WARNING:  REDEFINED NON VIRTUAL!!!.
     *
     */

    void reset();

    /**
     *
     *  Flushes the valid fetch cache lines back to memory.
     *
    *  @return If all the valid lines have been written
     *  to memory returns TRUE.  If not returns FALSE.
     *
     */

    bool flush();

    /**
     *
     *  Gets the next cache request in the queue.
     *
     *  @param requestID The cache request identifier.
     *
     *  @return A pointer to the next cache request entry.  If there
     *  are no requests returns NULL;
     *
     */

    Cache64Request *getRequest(U32 &requestID);

    /**
     *
     *  Liberates a cache request from the request queue.
     *
     *  @param request Request entry number.
     *  @param freeSpill If the spill request is liberated.
     *  @param freeFill If the fill request is liberated.
     *
     */

    void freeRequest(U32 request, bool freeSpill, bool freeFill);

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
