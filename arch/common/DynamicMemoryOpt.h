#ifndef __OPTIMIZED_DYNAMIC_MEMORY_H__
#define __OPTIMIZED_DYNAMIC_MEMORY_H__

#include "GPUType.h"
#include <cstddef> // size_t definition
#include <new> // bad_alloc definition
#include <string>

namespace cg1gpu
{

/**
 * @b Optimized class is a fast controller of dynamic memory.
 *
 * @b files: DynamicMemoryOpt.h DynamicMemoryOpt.cpp
 *
 * DynamicMemoryOpt class :
 * - Implements a memory manager for fast allocation and deallocation ( overloads new and delete operators )
 * - to use this feature you must inherit from this class. All methods are static.
 *
 * @note It is MANDATORY to call initialize method before using new and delete operators of wichever class
 *       devired from DynamicMemoryOpt.
 *
 * Example of use:
 *
 *    @code
 *       // suppose VSInstruction is a derived class of DynamicMemoryOpt
 *
 *       // Capacity for 1000 derived objects of DynamicMemoryOpt, max size of every object 256 bytes
 *       // we assume sizeof( VSInstruction ) is equal or lower than 256
 *       DynamicMemoryOpt::initialize( 256, 1000 );
 *       // now you can use new and delete as ever
 *
 *       // using DynamicMemoryOpt new operator
 *       // if VSInstruction size were greater than 256 a bad_alloc() exception would be thrown
 *       VSInstruction* vsi1 = new VSInstruction( ... );
 *
 *       // dumps memory including contents in raw mode
 *       VSInstruction::dumpDynamicMemoryState( true );
 *
 *       delete vsi1; // using DynamicMemoryOpt delete operator
 *    @endcode
 */
class DynamicMemoryOpt
{
private:
    struct Bucket //*  Defines an optimized memory bucket.
    {
    private:
        U32   _maxObjects;  // maximum number of allocations allowed
        U32   _chunkSize;   // size of a chunk of memory ( is a power of 2 )
        U32   _chunkSzLg2;  //  Log 2 of the size of the chunk.  
        char* _maxMemAddress;
        U32   _memSize;
    public:
        char* mem;          // mem available ( raw memory )
        U32*  sizes;        // tracing of sizes ( real ocupation within a chunk )
        U32*  map;          // fifo with the available identifiers of free chunks
        U32   nextFree;     // next block free ( number of current maps also )
        U32   maxUsage;     // Stores the largest number of chunks used. 

        void init(U32 maxObjectSize, U32 maxObjects);

        // These methods are as fast as accessing the contents directly, they are provided to
        // expose some bucket attributes as constants
        inline U32 MaxObjects() const { return _maxObjects; }
        inline U32 ChunkSize() const { return _chunkSize; }
        inline U32 ChunkSzLg2() const { return _chunkSzLg2; }
        inline U32 MemSize() const { return _memSize; }
        inline const char* const MaxMemAddress() const { return _maxMemAddress; }
    }; // struct Bucket

    static const U32 NUM_BUCKETS = 3;   // Defines the number of available memory buckets.
    static Bucket bucket[NUM_BUCKETS];  // Buckets of fixed sized blocks for memory allocation.  
    static bool wasCalled;              // controls that only one call to initialize is performed in the life of the class
    static U64 timeStamp;               // Timestamp counter.  
    static U32 freqSize[];              // Stores how frequent are different object sizes (power of 2).  

    /**
     * Check if a chunk is already ocupied ( used for print memory status )
     *
     * @param bucket to be tested
     * @param chunk chunk to be tested
     * @return true if chunk is ocupied, false is chunk is available ( free )
     */
    static bool isOcupied( U32 bucket, U32 chunk );

    DynamicMemoryOpt( const DynamicMemoryOpt& );

protected:
    void setTag(const char *tag);

    // Inherit classes call it implicitly, so It can not be private
    // It has an empty definition
    DynamicMemoryOpt() {}

public:

    /**
     * Converts a class into its name based on RTTI information kept by C++
     */
    virtual std::string toString() const;

    /**
     * Obtains a string representation of the name of the class object
     */
    std::string getClass() const;

    /**
     * Mandatory ( only once at the beginning )
     *
     *
     * @param maxObjectSize1 maximum size of wichever object who inherits from this class (for first bucket)
     * @param capacity1 maximum capacity of objects that can be allocated at the same time (for first bucket)
     * @param maxObjectSize2 maximum size of wichever object who inherits from this class (for second bucket)
     * @param capacity2 maximum capacity of objects that can be allocated at the same time (for second bucket)
     * @param maxObjectSize3 maximum size of wichever object who inherits from this class (for third bucket)
     * @param capacity3 maximum capacity of objects that can be allocated at the same time (for third bucket)
     *
     * @return true if all goes well, false otherwise ( a second call or any param is equal or lower than 0 )
     *
     * @note if maxObjectSize is not power of 2 then maxObjectSize is set as the lower power of two greater
     * than maxObjectSize
     */
    static void initialize( U32 maxObjectSize1, U32 capacity1, U32 maxObjectSize2, U32 capacity2,
        U32 maxObjectSize3, U32 capacity3 );

    /**
     * Called by the compiler when new operator is used, size_t param is discarded
     *
     * @param size discarded in our implementation since size is fixed at the beginning
     */
    void* operator new( size_t size) throw();

    /**
     * Called by the compiler when delete operator is performed
     *
     * @param obj passed by the compiler
     */
    void operator delete( void *obj );

    /**
     * Dumps debug information about the usage of dynamic memory
     *
     * @param dumpMemoryContentsToo contents of memory are dumpped
     * @param cooked if dumpMemoryContentsToo is true it is possible to specify a cooked or raw mode for dumping
     *
     * @note formated printing of memory contents is implemented packing 4 bytes in a U32
     */
    static void dumpDynamicMemoryState( bool dumpMemoryContentsToo = false, bool cooked = false );

    static void usage(); // *  Dumps usage information about the two dynamic memory buckets.

    // static void printNotDeletedObjects();
}; // class DynamicMemoryOpt

} // namespace cg1gpu

#endif
