/**************************************************************************
 *
 */

#ifndef GAL_BUFFER_IMP
    #define GAL_BUFFER_IMP

#include "GALBuffer.h"
#include "MemoryObject.h"
#include "GALStream.h"
#include <iostream>
#include <fstream>

using namespace std;
namespace libGAL
{

/**
 * GALBuffer interface is the common interface to declare API independent data buffers.
 * API-dependant vertex and index buffers are mapped onto this interface
 *
 * This objects are created from ResourceManager factory
 *
 * @author Carlos González Rodríguez (cgonzale@ac.upc.edu)
 * @version 1.0
 * @date 01/19/2007
 */
class GALBufferImp : public GALBuffer, public MemoryObject
{
public:

    GALBufferImp(gal_uint size, const gal_ubyte* data);
    ~GALBufferImp ();

    void dumpBuffer (ofstream& out, GAL_STREAM_DATA type, gal_uint components, gal_uint stride, gal_uint offset);


    gal_uint getSize (GAL_STREAM_DATA type);

    ///////////////////////////////////////////////////////
    /// GALResource methods that required implemenation ///
    ///////////////////////////////////////////////////////

    virtual void setUsage(GAL_USAGE usage);

    virtual GAL_USAGE getUsage() const;

    virtual GAL_RESOURCE_TYPE getType() const;

    virtual void setMemoryLayout(GAL_MEMORY_LAYOUT layout);
    
    virtual GAL_MEMORY_LAYOUT getMemoryLayout() const;

    virtual void setPriority(gal_uint prio);

    virtual gal_uint getPriority() const;

    virtual gal_uint getSubresources() const;

    virtual gal_bool wellDefined() const;

    /////////////////////////////////////////////////////////
    /// GALBuffer methods that require implementationfrom ///
    /////////////////////////////////////////////////////////

    virtual const gal_ubyte* getData() const;

    virtual gal_uint getSize() const;

    virtual void resize(gal_uint size, bool discard);

    virtual void clear();

    virtual void pushData(const gal_void* data, gal_uint size);

    virtual void updateData( gal_uint offset, gal_uint size, const gal_ubyte* data, gal_bool preload = false);
 
    ////////////////////////////////////////////////////////
    /// MemoryObject methods that require implementation ///
    ////////////////////////////////////////////////////////
    virtual const gal_ubyte* memoryData(gal_uint region, gal_uint& memorySizeInBytes) const;

    virtual const gal_char* stringType() const;


    ///////////////////////////////////////////////////
    /// Methods only available for GAL implementors ///
    ///////////////////////////////////////////////////

    /**
     * Reallocates the buffer to use the minimum required space
     */
    virtual void shrink();

    /**
     * Gets the capacity of the buffer to grow without reallocate new memory
     */
    virtual gal_uint capacity() const;

    virtual gal_uint getUID ();

private:

    static const gal_float GROWTH_FACTOR;
    static gal_uint _nextUID; // Debug info

    const gal_uint _UID;

    gal_uint _size;
    gal_uint _capacity;
    gal_ubyte* _data;
    GAL_USAGE _usage;

    GAL_MEMORY_LAYOUT layout;
};

} // namespace libGAL

#endif // GAL_BUFFER_IMP
