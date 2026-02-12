/**************************************************************************
 *
 */
#include "DynamicMemoryOpt.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ostream>
#include <iostream>
#include "support.h"
#include <fstream>
#include <map>
#include <string>
#include <sstream>
#include <list>
#include <typeinfo>

using namespace std;
using namespace arch;

#ifndef CG_LOG2
#define CG_LOG2(x) (log(F64(x))/log(2.0))
#endif

#ifndef CG_CEIL
#define CG_CEIL(x) ceil(x)
#endif

#define FAST_NEW_DELETE

DynamicMemoryOpt::Bucket DynamicMemoryOpt::bucket[NUM_BUCKETS]; //  Allocate the bucket structures.

bool DynamicMemoryOpt::wasCalled = false;             // one initialize call allowed only
U32 DynamicMemoryOpt::freqSize[24];
U64 DynamicMemoryOpt::timeStamp = 0;

using namespace std;
using namespace arch;

void DynamicMemoryOpt::Bucket::init(U32 maxObjectSize, U32 maxObjects)
{
    maxUsage = 0;
    _maxObjects = maxObjects;
    _chunkSzLg2 = (U32) CG_CEIL(CG_LOG2(maxObjectSize));
    _chunkSize  = 1 << _chunkSzLg2;
    _memSize    = _chunkSize * _maxObjects;

    mem = (char*)malloc( _maxObjects * _chunkSize );
    CG_ASSERT_COND((mem != 0),"Error allocating dynamic memory.");
   
    map = (U32*)malloc( _maxObjects * sizeof(U32) ); //  Allocate memory for the free dynamic memory list.
    CG_ASSERT_COND((map !=0), "Error allocating dynamic memory free list."); //  Check free list allocation.

    sizes = (U32*)malloc( _maxObjects * sizeof(U32) ); // Allocate usage of each chunk of dynamic memory.
    CG_ASSERT_COND((sizes != NULL), "Error allocating used dynamic memory map."); // Check usage of each chunk map allocation. 

    memset(mem, 0, _memSize); // Clear dynamic memory.

    //  Initialize the free list and the used memory map.
    for ( U32 j = 0; j < _maxObjects; j++ ) {
        map[j] = j; // all maps available
        sizes[j] = 0; // no ocupation in any chunk
    }

    nextFree = 0; // setting first free chunk //  Set free list first free pointer to 0.  
    _maxMemAddress = mem + _memSize;  // precompute max address
}


// { P: chunkSize is power of two && realObject is always lower than chunkSize }

void DynamicMemoryOpt::initialize( U32 maxObjectSize1, U32 capacity1,
                                         U32 maxObjectSize2, U32 capacity2,
                                         U32 maxObjectSize3, U32 capacity3 )
{
    // integrity test
    GPU_ASSERT(
        if ( capacity1 <= 0 || maxObjectSize1 <= 0 || capacity2 <= 0 || maxObjectSize2 <= 0 ||
             capacity3 <= 0 || maxObjectSize3 <= 0)
            CG_ASSERT("Object size and object capacity can not be 0.");
        if ( wasCalled )
            CG_ASSERT("Dynamic memory system already initialized.");
    )

#ifdef AM_FAST_NEW_DELETE
        if (maxObjectSize1 >= maxObjectSize2)
            CG_ASSERT("Object sizes for the buckets must be ordered for fast new/delete.");
#endif

    wasCalled = true;

    bucket[0].init(maxObjectSize1, capacity1);
    bucket[1].init(maxObjectSize2, capacity2);
    bucket[2].init(maxObjectSize3, capacity3);

#ifdef FAST_NEW_DELETE 
        CG_INFO("FAST_NEW_DELETE enabled.  Ignoring third bucket!");
#endif

    for(U32 i = 0; i < 24; i++)
        freqSize[i] = 0;
}

void* DynamicMemoryOpt::operator new( size_t objectSize ) throw()
{

#ifdef FAST_NEW_DELETE

    U32 b;
    U32 *p;

    //  Check the requested object size against the chunk ranges.  
    b = ((objectSize + 16) <= bucket[0].ChunkSize())?0:((objectSize + 16) <= bucket[1].ChunkSize())?1:2;

    if ((b == 2) || (bucket[b].nextFree == bucket[b].MaxObjects()))
    {
        if ( b == 2 ) 
            cout << "(b == 2)\n";

        if (bucket[b].nextFree == bucket[b].MaxObjects())
            cout << "bucket[b].nextFree == bucket[b].MaxObjects()\n";

        cout << "Bucket = " << b << " nextFree = " << bucket[b].nextFree << " MaxObjects() = " << bucket[b].MaxObjects() << "\n";
        CG_ASSERT("Error allocating object.");
    }

    p = (U32 *) &bucket[b].mem[bucket[b].map[bucket[b].nextFree] << bucket[b].ChunkSzLg2()];

    p[0] = b;
    p[1] = bucket[b].map[bucket[b].nextFree];
    //p[2] = 0xCCCCDDDD;

    //printf("p %p b %d pos %d\n", p, b, bucket[b].map[bucket[b].nextFree]);

    bucket[b].nextFree++;
    return &p[4];

#else // !FAST_NEW_DELETE
    
    U32 b;
    U32 currentSize;
    U32 i;

    GPU_ASSERT(
        if ( !wasCalled )
            CG_ASSERT("Initialization must be done before any allocation occurs.");
        if (( objectSize > bucket[0].ChunkSize() ) && ( objectSize > bucket[1].ChunkSize() ) && ( objectSize > bucket[2].ChunkSize() ))
        {
            printf("objectSize %d\n", objectSize);
            CG_ASSERT("Object larger than maximum dynamic object size.");
        }
    )

    //  Select the smaller bucket required for the object.  
    for(i = 0, currentSize = U32(-1); i < NUM_BUCKETS; i++)
    {
        //  Check if the object can be stored in the bucket and the chunk size is the smaller selected until now.  
        if (((objectSize + 16) <= bucket[i].ChunkSize()) && (currentSize > bucket[i].ChunkSize()))
        {
            b = i;
            currentSize = bucket[i].ChunkSize();
        }
    }

    // param ignored, size is fixed in initialization
    // find available map
    if ( bucket[b].nextFree == bucket[b].MaxObjects() )
    {
        stringstream ss;
        ss << "No free objects available in bucket " << b << " (numObjects " << bucket[b].MaxObjects()
           << " ChunkSize() " << bucket[b].ChunkSize() << ")";
        CG_ASSERT(ss.str().c_str());            
    }

    freqSize[static_cast<U32>(ceil(CG_LOG2(objectSize)))]++;

    if ((bucket[b].nextFree + 1) > bucket[b].maxUsage)
        bucket[b].maxUsage = bucket[b].nextFree + 1;

    bucket[b].sizes[bucket[b].map[bucket[b].nextFree]] = static_cast<U32>(objectSize); // set used memory map entry with this object size

    timeStamp++;

    return &bucket[b].mem[bucket[b].map[bucket[b].nextFree++]*bucket[b].ChunkSize()]; // fast allocation ( product is a shift )

#endif // FAST_NEW_DELETE  
}

void DynamicMemoryOpt::operator delete ( void* obj )
{
    if ( !obj ) // Standard behaviour
        return ;
#ifdef FAST_NEW_DELETE

    U32 b;
    U32 idx;

    //if (*(((U32*) obj) - 2) != 0xCCCCDDDD)
    //    CG_ASSERT("Error in delete.");
    //printf("obj %p b %d pos %d\n", obj, b, idx);

    b = *(((U32*) obj) - 4);
    idx = *(((U32*) obj) - 3);

    Bucket& bu = bucket[b]; // alias to avoid multiple computations of bucket[b]
    bu.map[--bu.nextFree] = idx; // release map, new available chunk

#else // !FAST_NEW_DELETE

    U32 b;

    //  Check DynamicMemoryOpt initialization. 
    CG_ASSERT_COND(!( !wasCalled ), "Initialization must be done before any de-allocation occurs.");
    // Check null object.  
    GPU_ASSERT(
        if ( !obj ) // delete of null pointer
            CG_ASSERT("Trying to delete a null pointer.");
    )

    // Convenient cast to clarify code
    const char* const objptr = reinterpret_cast<const char* const>(obj);

    // Select bucket for the object to delete.
    if ( objptr >= bucket[0].mem && objptr <= bucket[0].MaxMemAddress() )
        b = 0;
    else if ( objptr >= bucket[1].mem && objptr <= bucket[1].MaxMemAddress() )
        b = 1;
    else if ( objptr >= bucket[2].mem && objptr <= bucket[2].MaxMemAddress() )
        b = 2;
    else
        CG_ASSERT("Object address outside dynamic memory range.");

    // Assume it was created in the pool of dynamic memory
    // calculate which block number is
    U32 iMap = static_cast<U32>(objptr - bucket[b].mem);

    CG_ASSERT_COND(!( iMap % bucket[b].ChunkSize() != 0 ), "Object address not correctly aligned.");
    // if needed in DEBUG mode, test if the obj owns to the heap
    iMap /= bucket[b].ChunkSize(); // ( div is a shift )

    //  Check that the object chunk hasn't been already deleted. 
    GPU_ASSERT(
        if(bucket[b].sizes[iMap] == 0)
        {
            printf("Block %d address %x allocated %d bytes tag %s timeStamp %llx\n", iMap,
                    reinterpret_cast<U64>(bucket[b].mem + iMap*bucket[b].ChunkSize()),
                    bucket[b].sizes[iMap],
                    &(((char *) (bucket[b].mem + iMap * bucket[b].ChunkSize()))[bucket[b].ChunkSize() - 16]),
                    ((U64 *) (bucket[b].mem + iMap * bucket[b].ChunkSize()))[(bucket[b].ChunkSize() >> 3) - 1]
                    );

            CG_ASSERT("Deleting an already deleted chunk.\n");
        }

        if (bucket[b].sizes[iMap] > bucket[b].ChunkSize())
        {
            CG_ASSERT("Deleting a chunk from the wrong bucket.\n");
        }
    )

    bucket[b].nextFree--;

    bucket[b].sizes[iMap] = 0; // free chunk, no consumed space then

    bucket[b].map[bucket[b].nextFree] = iMap; // release map, new available chunk

#endif // FAST_NEW_DELETE
    
}


void DynamicMemoryOpt::setTag(const char *tag)
{
#ifdef FAST_NEW_DELETE
        return ;
#endif

    U32 b;

    const char* const objptr = reinterpret_cast<const char* const>(this);

    //  Select bucket for the object to delete.  
    if (objptr >= bucket[0].mem && objptr <= bucket[0].MaxMemAddress())
    {
        b = 0;
    }
    if (objptr >= bucket[1].mem && objptr <= bucket[1].MaxMemAddress())
    {
        b = 1;
    }
    if (objptr >= bucket[2].mem && objptr <= bucket[2].MaxMemAddress())
    {
        b = 2;
    }
    else
    {
        return;
        CG_ASSERT("Object address outside dynamic memory range.");
    }

    ((U64 *) this)[(bucket[b].ChunkSize() >> 3) - 2] = *((U64 *) tag);
    ((U64 *) this)[(bucket[b].ChunkSize() >> 3) - 1] = timeStamp;
}


std::string DynamicMemoryOpt::getClass() const
{
    return string(typeid(*this).name());
}

string DynamicMemoryOpt::toString() const
{
    return getClass();
}

void DynamicMemoryOpt::dumpDynamicMemoryState( bool dumpMemoryContentsToo, bool cooked )
{
    for(U32 i = 0; i < NUM_BUCKETS; i++)
    {
        cout << "Dump mem Statistics...  " << endl;
        cout << "True size ocuppied by an object ( ChunkSize() ): " << bucket[i].ChunkSize() << " bytes" << endl;
        cout << "Max number of objects allowed DynMem: " << bucket[i].MaxObjects() << endl;
        cout << "Objects allocated in DynMem (outstanding deletes): " << bucket[i].nextFree << endl;
        cout << "Available storage (in objects): " << bucket[i].MaxObjects() - bucket[i].nextFree << endl;
        cout << "Available storage (in bytes): " << bucket[i].ChunkSize()*( bucket[i].MaxObjects() - bucket[i].nextFree ) <<
            " bytes" << endl;

        cout << "Mapping representation ( free blocks ): " << endl;
        if ( bucket[i].nextFree == bucket[i].MaxObjects() )
            cout << "No free maps available..." << endl;
        else {
            for ( U32 j = bucket[i].nextFree; j < bucket[i].MaxObjects(); j++ ) {
                if ( j == bucket[i].nextFree )
                    cout << "i: " << j << "   chunkIndex: " << bucket[i].map[j] << "  <-- nextFree" << endl;
                else
                    cout << "i: " << j << "   chunkIndex: " << bucket[i].map[j] << endl;
            }
        }

        printf("Allocated blocks.  Last time stamp: %llx.\n", timeStamp);

        //  Print used blocks.  
        for(U32 j = 0; j < bucket[i].MaxObjects(); j++)
        {
            if (bucket[i].sizes[j] != 0)
                printf("Block %d address %p allocated %d bytes tag %s timeStamp %llx\n", j,
                    bucket[i].mem+j*bucket[i].ChunkSize(),
                    bucket[i].sizes[j],
                    &(((char *) (bucket[i].mem + j * bucket[i].ChunkSize()))[bucket[i].ChunkSize() - 16]),
                    ((U64 *) (bucket[i].mem + j * bucket[i].ChunkSize()))[(bucket[i].ChunkSize() >> 3) - 1]
                    );
        }
    }

printf("Frequencies:\n");
for(U32 i = 0; i < 24; i++)
printf("Size %d -> %d\n", U32(pow(2.0, double(i))), freqSize[i]);

    if ( !dumpMemoryContentsToo )
        return ;

    U32 inc = sizeof( char );
    if ( cooked ) // cooked means grouping 4 bytes in an U32
        inc = sizeof( U32 );

    for (U32 i = 0; i < NUM_BUCKETS; i++)
    {
        for ( U32 j = 0; j < bucket[i].ChunkSize()*bucket[i].MaxObjects(); j+=inc ) {
            if ( j % bucket[i].ChunkSize() == 0 ) {
                if ( isOcupied(i, j/bucket[i].ChunkSize() ) )
                    cout << "--- Reserved block ( real ocupation: " << bucket[i].sizes[j/bucket[i].ChunkSize()]
                        << " bytes ) ------------------------" << endl;
                else
                    cout << "--- Free block -------------------------------------------------------" << endl;
            }
            printf( "%04i: ", j );
            if ( cooked )
                cout << *(U32*)(bucket[i].mem+j)  << endl;
            else
                cout << bucket[i].mem[i] << endl;
        }
    }
}

void DynamicMemoryOpt::usage()
{
#ifdef FAST_NEW_DELETE
    printf("Bucket 0: Size %d Last %d Max %d | Bucket 1: Size %d Last %d Max %d | Bucket 2: Size %d Last %d Max %d\n",
            bucket[0].MaxObjects(), bucket[0].nextFree, bucket[0].maxUsage,
            bucket[1].MaxObjects(), bucket[1].nextFree, bucket[1].maxUsage,
            bucket[2].MaxObjects(), bucket[2].nextFree, bucket[2].maxUsage
          );
#else // !FAST_NEW_DELETE
    cout << "\n-----------------------------------------------------------------\n";
    cout << "------------------ OPTIMIZED DYNAMIC MEMORY USAGE ---------------\n";
    cout << "-----------------------------------------------------------------\n";
    for ( U32 i = 0; i < NUM_BUCKETS; ++i ) {
        const Bucket& b = bucket[i];
        cout << "Bucket " << i << ": Size " << b.MaxObjects() << " Last " << b.nextFree << " Max " << b.maxUsage 
            << " (object size "  <<  b.ChunkSize() <<  " bytes)\n";
    }
    cout << "-----------------------------------------------------------------\n";
    for ( U32 i = 0; i < NUM_BUCKETS; ++i ) {
        map<string,U32> mapCounts;
        cout << "Objects in BUCKET " << i << " -> ALLOCATIONS" << endl;
        U32 accum = 0;
        for(U32 j = 0; j < bucket[i].MaxObjects(); ++j) 
        {
            if ( bucket[i].sizes[j] != 0 ) {
                ++accum;
                char* address = bucket[i].mem+j*bucket[i].ChunkSize();
                DynamicMemoryOpt* dynObj = reinterpret_cast<DynamicMemoryOpt*>(address);
                string className = typeid(*dynObj).name();
                map<string,U32>::iterator it = mapCounts.find(className);
                if ( it == mapCounts.end() )
                    mapCounts.insert(make_pair(className,0));
                ++mapCounts[className];
            }
        }
        list<pair<U32,string> > pendingList;
        for ( map<string,U32>::iterator it = mapCounts.begin(); it != mapCounts.end(); ++it ) {
            pendingList.push_back(make_pair(it->second, it->first));
            // cout << "  " << it->first << " -> " << it->second << "\n";
        }
        pendingList.sort(); // Sort by number of allocatations
        for ( list<pair<U32,string> >::iterator it = pendingList.begin(); it != pendingList.end(); ++it ) {
            cout << "  " << it->second << " -> " << it->first << "\n";
        }

        cout << "  TOTAL -> " << accum << " Allocated objects in BUCKET " << i << endl;
    }
    cout << "-----------------------------------------------------------------\n";
#endif // FAST_NEW_DELETE
}

bool DynamicMemoryOpt::isOcupied( U32 b, U32 chunk )
{
    for ( U32 i = bucket[b].nextFree; i < bucket[b].MaxObjects(); i++ ) {
        if ( bucket[b].map[i] == chunk )
            return false;
    }
    return true;
}
