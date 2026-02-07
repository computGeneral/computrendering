/**************************************************************************
 *
 */

#ifndef MEMORYOBJECTALLOCATOR
    #define MEMORYOBJECTALLOCATOR

#include "GALTypes.h"
#include <map>
#include <vector>

class HAL;

namespace libGAL
{

class MemoryObject;

/**
 * This object implements a layer between MemoryObjects and low-level driver memory interface
 * to support MemoryObject synchronization/allocation/deallocation in the GPU/system memory
 *
 * @author Carlos Gonz�lez Rodr�guez (cgonzale@ac.upc.edu)
 * @date 02/15/2007
 */
class MemoryObjectAllocator
{
public:

    /**
     * Creates a memory object allocator
     *
     * @param driver The driver interface used to access directly to the GPU local memory
     */
    MemoryObjectAllocator(HAL* driver);

    /**
     * Synchronizes/Allocates the memory object data (all subregions) from CPU memory to GPU local memory
     *
     * @param mo The memory object to synchronize/allocate
     *
     * @note After calling this method memory descriptors identifiers previously obtained
     *       with md() method may be invalid. So, after any sync() method call
     *       you must call again md() is you need the memory descriptors for any purpose.
     */
    void syncGPU(MemoryObject* mo);

    /**
     * Synchronize/Allocates a subregion of the memory object data from CPU memory to GPU local memory
     *
     * @param mo The memory object from where the subregion is to be synchronized/allocated
     * @param region The region to be synchronized/allocated
     *
     * @see syncGPU()
     */
    gal_bool syncGPU(MemoryObject* mo, gal_uint region);

    /**
     *
     *  Assigns an already defined memory descriptor to a subregion of the the memory object.
     *
     *  @param mo The memory object for which to define the memory descriptor.
     *  @param region The region of the memory object for which to define the memory descriptor.
     *  @param md The memory descriptor to be assigned to the memory object region.
     * 
     */
     
    void assign(MemoryObject *mo, gal_uint region, gal_uint md);
    
    /**
     * Deallocates all the subregions of a memory object from GPU local memory
     *
     * @param mo The memory object to deallocate
     */
    gal_bool deallocate(MemoryObject* mo);

    /**
     * Deallocates a specified subregion of a memory object from GPU local memory
     *
     * @param mo The memory object 
     */
    gal_bool deallocate(MemoryObject* mo, gal_uint subregion);

    /**
     * Gets one of the memory descriptors used to allocate the memory object data (system/gpu)
     *
     * @param mo The memory object queried
     * @param region the region memory descriptor reclaimed
     */
    gal_uint md(MemoryObject* mo, gal_uint region = 0);

    /**
     * Returns if the memory object has any md
     *
     * @param mo The memory object queried
     * @param region
     */
    gal_bool hasMD(MemoryObject* mo, gal_uint region);

	/**
     * Set to unlock all the memory regions that have been used in the previous frame
     *
     */
	void realeaseLockedMemoryRegions();

private:

    // moi[region].md --> memory descriptor
    // moi[region].size --> memory size
    struct RegionInfo
    {
        gal_uint md;
        gal_uint size;
        RegionInfo() {} // default required for STL containers
        RegionInfo(gal_uint md, gal_uint size) : md(md), size(size) {}
    };

    // Define the memory object info type as a map among region values and attached region information
    typedef std::map<gal_uint, RegionInfo> MemoryObjectInfo;
    
    // MemoryObjectInfo's are created and inserted into maps with _createMOI call
    typedef std::map<MemoryObject*, MemoryObjectInfo> MemoryObjectMap;

    // Contains information of memory objects used by this class (as parameter) 
    MemoryObjectMap maps;

	struct lockedRegion
	{
		MemoryObject* mo;
		gal_int region;

		lockedRegion(MemoryObject* mo, gal_uint region): mo(mo), region(region) {}
	};

	std::vector<lockedRegion> lockedMemory[2];
	std::vector<lockedRegion>* lockOutMem;
	std::vector<lockedRegion>* lockInMem;
	
    
    // Memory object information object helper functions
    MemoryObjectInfo* _createMOI(MemoryObject* mo);
    void _addRegionToMOI(MemoryObjectInfo* moi, gal_uint region, gal_uint md, gal_uint size);
    MemoryObjectInfo* _findMOI(MemoryObject* bo);
    
    // Pointer to the driver interface
    HAL* _driver;
    
    // Primitives to manage local GPU memory, the public methods are built on top this functions
    void _update(MemoryObject* mo, MemoryObjectInfo* moi, gal_uint region);
    void _dealloc(MemoryObject* mo, MemoryObjectInfo* moi, gal_uint region);
    void _alloc(MemoryObject* mo, MemoryObjectInfo* moi, gal_uint region);

};

}

#endif // MEMORYOBJECTALLOCATOR
