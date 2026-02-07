/**************************************************************************
 *
 */

#include "MemoryObject.h"
#include "support.h"

using namespace libGAL;
using namespace std;


MemoryObject::MemoryObject() : _globalReallocs(0), _preferredMemory(MT_LocalMemory)
{}

void MemoryObject::setPreferredMemory(MemoryType mem)
{
    _preferredMemory = mem;
}

MemoryObject::MemoryType MemoryObject::getPreferredMemory() const
{
    return _preferredMemory;
}

void MemoryObject::getUpdateRange(gal_uint region, gal_uint& startByte, gal_uint& lastByte)
{
    map<gal_uint, MemoryObjectRegion>::const_iterator it = mor.find(region);
    if ( it == mor.end() )
        CG_ASSERT("Region not found/defined");

    startByte = it->second.firstByteToUpdate;
    lastByte = it->second.lastByteToUpdate;
}

vector<gal_uint> MemoryObject::getDefinedRegions() const
{
    vector<gal_uint> regs;//(mor.size());

    map<gal_uint, MemoryObjectRegion>::const_iterator it = mor.begin();
    map<gal_uint, MemoryObjectRegion>::const_iterator end = mor.end();

    for ( ; it != end; ++it )
        regs.push_back(it->first);

    return regs;
}

gal_uint MemoryObject::trackRealloc(gal_uint region) const
{
    map<gal_uint, MemoryObjectRegion>::const_iterator it = mor.find(region);
    if ( it == mor.end() )
        CG_ASSERT("Tracking reallocation of an undefined region");

    return it->second.reallocs;
}

void MemoryObject::lock(gal_uint region, gal_bool lock)
{
	map<gal_uint, MemoryObjectRegion>::iterator it = mor.find(region);
	if ( it == mor.end() )
		CG_ASSERT("Searching an undefined region");

	it->second.locked = lock;
}

gal_bool MemoryObject::isLocked(gal_uint region) const
{
	map<gal_uint, MemoryObjectRegion>::const_iterator it = mor.find(region);
	if ( it == mor.end() )
		CG_ASSERT("Searching if it is lock an undefined region");

	return it->second.locked;
}

void MemoryObject::preload(gal_uint region, gal_bool preloadData)
{
	map<gal_uint, MemoryObjectRegion>::iterator it = mor.find(region);
	if ( it == mor.end() )
		CG_ASSERT("Searching an undefined region");

	it->second.preload = preloadData;
}

gal_bool MemoryObject::isPreload(gal_uint region) const
{
	map<gal_uint, MemoryObjectRegion>::const_iterator it = mor.find(region);
	if ( it == mor.end() )
		CG_ASSERT("Searching if it is lock an undefined region");

	return it->second.preload;
}

gal_uint MemoryObject::trackRealloc() const
{
    return _globalReallocs;
}

void MemoryObject::defineRegion(gal_uint region)
{
    // Discard previous region definition (if exists) and define a new region
    map<gal_uint, MemoryObjectRegion>::iterator it = mor.find(region);
    if ( it == mor.end() )
    {
        MemoryObjectRegion info;
        info.state = MOS_ReAlloc;
        info.reallocs = 0;
		info.locked = false;
		info.preload = false;
        mor.insert(make_pair(region,info));
    }
    else
    {
        if ( it->second.state != MOS_ReAlloc ) 
        {
            it->second.state = MOS_ReAlloc;
            it->second.reallocs++;
			it->second.locked = false;
			it->second.preload = false;
            _globalReallocs++;
        }
    }
}

void MemoryObject::_postUpdate(gal_uint moID, MemoryObjectRegion& moRegion, gal_uint startByte, gal_uint lastByte)
{ 
    if ( lastByte == 0 ) { // Update from start to last region byte
        memoryData(moID, lastByte); // Get memory object region size in 'lastByte' parameter
        --lastByte;
    }

    switch ( moRegion.state )
    {
        case MOS_ReAlloc:
            return ; // ignore partial update, this region will be reallocated completely
        case MOS_Blit:
            //CG_ASSERT("Blitted memory regions can not be updated (not implemented)");
            break;
        case MOS_RenderBuffer:
            //CG_ASSERT("Render buffer regions can not be updated (not implemented).");            
            break;
        case MOS_Sync:
            moRegion.state = MOS_NotSync;
            moRegion.firstByteToUpdate = startByte;
            moRegion.lastByteToUpdate = lastByte;
            break;
        case MOS_NotSync:
            // Enlarge "update range" if required
            if ( moRegion.firstByteToUpdate > startByte )
                moRegion.firstByteToUpdate = startByte;
            if ( moRegion.lastByteToUpdate < lastByte )
                moRegion.lastByteToUpdate = lastByte;
            break;
        default:
            CG_ASSERT("Unknown memory object state");
    }
}

void MemoryObject::postUpdate(gal_uint region, gal_uint startByte, gal_uint lastByte)
{
    map<gal_uint, MemoryObjectRegion>::iterator it = mor.find(region);
    if ( it == mor.end() )
        CG_ASSERT("Posting an update region in a region that does not exist");

    _postUpdate(it->first, it->second, startByte, lastByte);    
}

void MemoryObject::postUpdateAll()
{
    map<gal_uint, MemoryObjectRegion>::iterator it = mor.begin();
    const map<gal_uint, MemoryObjectRegion>::iterator itEnd = mor.end();
    for ( ; it != itEnd; ++it )
        _postUpdate(it->first, it->second , 0, 0);
}

void MemoryObject::postReallocate(gal_uint region)
{
    map<gal_uint, MemoryObjectRegion>::iterator it = mor.find(region);
    if ( it == mor.end() )
        CG_ASSERT("Posting ReAlloc in a region that does not exist");

    if ( it->second.state != MOS_ReAlloc ) {
        it->second.state = MOS_ReAlloc;
        it->second.reallocs++; // increment reallocs if the object/region was not already in state realloc
		it->second.locked = false;
        _globalReallocs++;
    }
}

void MemoryObject::postReallocateAll()
{
    map<gal_uint, MemoryObjectRegion>::iterator it = mor.begin();
    const map<gal_uint, MemoryObjectRegion>::iterator itEnd = mor.end();
    for ( ; it != itEnd; ++it ) {
        if ( it->second.state != MOS_ReAlloc ) {
            it->second.state = MOS_ReAlloc;
            it->second.reallocs++; // increment reallocs if the object/region was not already in state realloc
			it->second.locked = false;
            _globalReallocs++;
        }
    }
}

void MemoryObject::postBlit(gal_uint region)
{
    map<gal_uint, MemoryObjectRegion>::iterator it = mor.find(region);
    if ( it == mor.end() )
        CG_ASSERT("Posting Blit in a region that does not exist");

    it->second.state = MOS_Blit;
}

void MemoryObject::postBlitAll()
{
    map<gal_uint, MemoryObjectRegion>::iterator it = mor.begin();
    const map<gal_uint, MemoryObjectRegion>::iterator itEnd = mor.end();
    for ( ; it != itEnd; ++it )
        it->second.state = MOS_Blit;
}

void MemoryObject::postRenderBuffer(gal_uint region)
{
    map<gal_uint, MemoryObjectRegion>::iterator it = mor.find(region);
    if ( it == mor.end() )
        CG_ASSERT("Posting Blit in a region that does not exist");

    it->second.state = MOS_RenderBuffer;
}

void MemoryObject::postRenderBufferAll()
{
    map<gal_uint, MemoryObjectRegion>::iterator it = mor.begin();
    const map<gal_uint, MemoryObjectRegion>::iterator itEnd = mor.end();
    for ( ; it != itEnd; ++it )
        it->second.state = MOS_RenderBuffer;
}

const gal_char* MemoryObject::stringType() const
{
    return "MEMORY_OBJECT_STRING_NOT_DEFINED";
}


MemoryObjectState MemoryObject::getState(gal_uint region) const
{
    map<gal_uint, MemoryObjectRegion>::const_iterator it = mor.find(region);
    
    if ( it == mor.end() )
        CG_ASSERT("Region not found/defined");

    return it->second.state;
}

void MemoryObject::changeState(gal_uint region, MemoryObjectState newState)
{
    map<gal_uint, MemoryObjectRegion>::iterator it = mor.find(region);
    
    if ( it == mor.end() )
        CG_ASSERT("Region not found/defined");

    it->second.state = newState;
}
