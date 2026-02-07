/**************************************************************************
 *
 */

#include "GPUMemory.h"
#include "HAL.h"
#include <iostream>
#include "support.h"

using namespace std;
using namespace libgl;

GPUMemory::GPUMemory(HAL* driver) : driver(driver)
{    
    // empty
}

GPUMemory::BaseObjectInfo* GPUMemory::_find(BaseObject* bo)
{
    map<BaseObject*, BaseObjectInfo>::iterator it = maps.find(bo);
    if ( it != maps.end() )
    {
        return &(it->second);
    }    
    return 0;
}

void GPUMemory::_update(BaseObject* bo, BaseObjectInfo* boi)
{    
    for ( GLuint i = 0; i < bo->getNumberOfPortions(); i++ )
    {
        vector<pair<GLuint, GLuint> > modifRanges = bo->getModifiedRanges();
        U08* binData = (U08*)bo->binaryData(i);
        U32 binSize = bo->binarySize(i);
        
        for ( GLuint j = 0; j < modifRanges.size(); j++ ) // update regions
        {
            GLuint offset = modifRanges[j].first;
            GLuint size = modifRanges[j].second;
            // check memory bounds
            if ( offset + size > binSize )
                CG_ASSERT("Trying to write farther from memory bounds");
            driver->writeMemory(boi->md[i], offset, binData, size);            
        }
    }
    bo->setSynchronized();
    //bo->setState(BaseObject::Sync);
}


void GPUMemory::_alloc(BaseObject* bo)
{
    //cout << "start\n _alloc: " << bo->toString() << endl;
    BaseObjectInfo boi;
    GLuint portions = bo->getNumberOfPortions();
    
    for ( GLuint i = 0; i < portions; i++ )
    {
        //cout << "Sync portion: " << i;        
        cout.flush();
        boi.size.push_back(bo->binarySize(i));
        U32 desc;
        if ( bo->getPreferredMemoryHint() == BaseObject::GPUMemory ) // Try to use GPUMemory for this base object
            desc = driver->obtainMemory(boi.size[i], HAL::GPUMemoryFirst);
        else
            desc = driver->obtainMemory(boi.size[i], HAL::SystemMemoryFirst); // Try to use System memory
        boi.md.push_back(desc);
        driver->writeMemory(boi.md[i], bo->binaryData(i), boi.size[i]);
        //cout << " OK" << endl;
    }
    maps.insert(make_pair(bo, boi));
    //bo->setState(BaseObject::Sync);
    bo->setSynchronized();
    //cout << "end of _alloc" << endl;
}

void GPUMemory::_dealloc(BaseObject* bo, BaseObjectInfo* boi)
{
    vector<U32>::iterator it = boi->md.begin();
    for ( ; it != boi->md.end(); it++ )
        driver->releaseMemory(*it);  
    maps.erase(bo);
    //bo->setState(BaseObject::ReAlloc); // object not allocated (must be allocated to be used)
    bo->forceRealloc();
}


bool GPUMemory::allocate(BaseObject& bo)
{
    GPUMemory::BaseObjectInfo* boInfo = _find(&bo);    
    BaseObject::State state = bo.getState();
    if ( boInfo )
    {
        if ( state == BaseObject::ReAlloc )
        {
            //  reallocate
            _dealloc(&bo, boInfo);
            _alloc(&bo);
        }
        else if ( state == BaseObject::NotSync )
        {
            _update(&bo, boInfo);
        }
        else // { ( state == BaseObject::Sync ) -> already synchronized } ||
            //  { ( state == BaseObject::Blit ) -> updated data is already and exclusively in memory  }
        {
            return false;
        }
    }
    else
    {
        if ( state != BaseObject::ReAlloc )
            CG_ASSERT("Should not happen."
                                           "GPUMemory internal info not found for an allocated BaseObject");
        _alloc(&bo);
    }
    return true;
}

bool GPUMemory::deallocate(BaseObject& bo)
{
    BaseObjectInfo* boInfo = _find(&bo);
    BaseObject::State state = bo.getState();

    if ( boInfo )
        _dealloc(&bo, boInfo);
    else
    {
        CG_ASSERT("Trying to deallocate a not allocated object");
        return false;
    }
            
    return true;
}


bool GPUMemory::isPresent(BaseObject& bo)
{
    return ( _find(&bo) != 0 );
}

U32 GPUMemory::md(BaseObject& bo, GLuint portion)
{
    if ( !bo.isSynchronized() )
    {
        CG_ASSERT("Base object must be synchronized before calling this method");
        return 0;
    }
    
    BaseObjectInfo* boi = _find(&bo);
    if ( !boi )
    {
        CG_ASSERT("The object is not currently allocated into GPU Memory");
        return 0;
    }
    
    if ( portion >= boi->md.size() )
    {
        char msg[256];
        sprintf(msg, "It does not exist the portion %d in this BaseObject", portion);
        CG_ASSERT(msg);
    }
    return boi->md[portion];
}


void GPUMemory::dump() const
{
    cout << "------ GPUMemory allocated objects ------\n\n";
    map<BaseObject*,BaseObjectInfo>::const_iterator it = maps.begin();
    for ( ; it != maps.end(); it++ )
    {
        BaseObject* bo = it->first;
        BaseObjectInfo boi = it->second;

        cout << "State: ";
        
        BaseObject::State state = bo->getState();
        switch(state)
        {
            case BaseObject::ReAlloc: cout << "ReAlloc"; break;
            case BaseObject::NotSync: cout << "NotSync"; break;
            case BaseObject::Sync: cout << "Sync"; break;
            case BaseObject::Blit: cout << "Blit"; break;
            default:
                CG_ASSERT("Unknown state");
        }
        
        cout << endl;
        
        //cout << "ID_STRING: " << bo->toString().c_str() << " :\n";
        cout << "   md's: ";
        
        for (U32 i = 0; i < boi.md.size(); i++ )
        {
            
            cout << boi.md[i] << endl;
            
            driver->dumpMD(boi.md[i]);
            
            if ( i < boi.md.size() - 1 )
                cout << ", ";
        }
        cout << "\n   size's: ";
        for (U32 i = 0; i < boi.size.size(); i++ )
        {
            cout << boi.size[i];
            if ( i < boi.size.size() - 1 )
                cout << ", ";
        }
        cout << "\n   Number of portions: " << bo->getNumberOfPortions() << endl << endl;
        
    }
    
    //driver->dumpMemoryAllocation();
    
    cout << "\n-----------------------------------------" << endl;
}
