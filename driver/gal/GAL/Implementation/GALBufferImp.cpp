/**************************************************************************
 *
 */

#include "GALBufferImp.h"
#include "GALMacros.h"
#include "support.h"
#include <iostream>
#include <fstream>
#include <cstring>

using namespace std;
using namespace libGAL;

const gal_float GALBufferImp::GROWTH_FACTOR = 1.10f;
gal_uint GALBufferImp::_nextUID = 0;

#define BURST_SIZE 32

GALBufferImp::GALBufferImp(gal_uint size, const gal_ubyte* data) : _UID(_nextUID)
{
    _nextUID++;

    // Start buffer memory tracking
    defineRegion(0);

    if ( size == 0 ) {
        GAL_ASSERT(
            if ( data != 0 )
                CG_ASSERT("Data pointer NOT null with size == 0");
        )
        _data = 0;
        _size = 0;
        _capacity = 0;
    }
    else {
        _size = _capacity = size;
        _data = new gal_ubyte[size];

        if ( data != 0 )
            memcpy(_data, data, size);

        // Notify reallocate requeriment
        postReallocate(0);
    }
}

GALBufferImp::~GALBufferImp () {
    delete[] _data;
}

void GALBufferImp::setUsage(GAL_USAGE usage)
{
    GAL_ASSERT(
        if ( _data )
            CG_ASSERT("Buffer with defined contents cannot redefine its usage");
    )

    _usage = usage;

    if (usage == GAL_USAGE_STATIC)
        setPreferredMemory(MemoryObject::MT_LocalMemory);
    else
        setPreferredMemory(MemoryObject::MT_SystemMemory);
}

GAL_USAGE GALBufferImp::getUsage() const
{
    return _usage;
}

void GALBufferImp::setMemoryLayout(GAL_MEMORY_LAYOUT _layout)
{
    layout = _layout;
}

GAL_MEMORY_LAYOUT GALBufferImp::getMemoryLayout() const
{
    return layout;
}

GAL_RESOURCE_TYPE GALBufferImp::getType() const
{
    return GAL_RESOURCE_BUFFER;
}

void GALBufferImp::setPriority(gal_uint prio)
{
    cout << "GALBufferImp::setPriority() -> Not implemented yet" << endl;
}

gal_uint GALBufferImp::getPriority() const
{
    cout << "CDBufferImp::getPriority() -> Not implemented yet" << endl;
    return 0;
}

gal_uint GALBufferImp::getSubresources() const
{
    return 1;
}

gal_bool GALBufferImp::wellDefined() const
{
    return (_data != 0 && _size != 0 );
}

const gal_ubyte* GALBufferImp::getData() const
{
    return _data;
}

gal_uint GALBufferImp::getSize() const
{
    return _size;
}

void GALBufferImp::resize(gal_uint size, bool discard)
{
    gal_uint originalCapacity = _capacity;

    if ( size > _capacity ) {
        gal_ubyte* chunk = new gal_ubyte[size];
        if ( !discard )
            memcpy(chunk, _data, _size);
        delete[] _data;
        _data = chunk;
        _capacity = _size = size; // updates new capacity and size
    }
    else // size <= _capacity
        _size = size;

    // Reallocation required in GPU memory
    if ( discard )
        postReallocate(0);
    else
        postUpdate(0, originalCapacity, size);
}

void GALBufferImp::clear()
{
    // Implementando clear de los buffers para soportard glgal::discarBuffers()
    delete[] _data;

    _size = 0;
    _capacity = 0;
    _data = 0;

    // Next synchronization will release previous buffer contents on GPU memory or system memory
    postReallocate(0);
}

void GALBufferImp::pushData(const gal_void* data, gal_uint size)
{
    if ( _size + size > _capacity ) { // buffer growth is required
        if ( _size + size > static_cast<gal_uint>(_size * GROWTH_FACTOR) )
            _capacity = _size + size;
        else
            _capacity = static_cast<gal_uint>(_size * GROWTH_FACTOR);
        // growth is required
        gal_ubyte* tmp = new gal_ubyte[_capacity];
        memcpy(tmp, _data, _size); // Copy previous contents
        delete[] _data; // Delete old contents
        _data = tmp; // reassign data pointer
    }

    // Push new data at the end of the buffer
    memcpy(_data + _size, data, size);
    _size += size;

            
    postReallocate(0);
}

void GALBufferImp::updateData( gal_uint offset, gal_uint size, const gal_ubyte* data, gal_bool preloadData)
{
    GAL_ASSERT
    (
        if ( offset + size > _size )
            CG_ASSERT("Buffer overflow offset + size is greater than total buffer size");
    )

    memcpy(_data + offset, data, size); 

    
    if ( (offset % BURST_SIZE) != 0) // Check if buffer realloc is properly aligned
    {
        size = size + (offset % BURST_SIZE);
        offset = offset - (offset % BURST_SIZE);
    }
    
    // Mark the memory range as dirty
    postUpdate(0, offset, offset + size - 1);
    preload(0, preloadData);
}

const gal_ubyte* GALBufferImp::memoryData(gal_uint region, gal_uint& memorySizeInBytes) const
{
    // Buffers are placed equally in CPU and GPU memory (linear), so no translation is required

    GAL_ASSERT(
        if ( _data == 0 || _size == 0 )
            CG_ASSERT("Memory data or size are NULL");
    )

    memorySizeInBytes = _size;
    return _data;
}

const gal_char* GALBufferImp::stringType() const
{
    return "MEMORY_OBJECT_BUFFER";
}

void GALBufferImp::shrink()
{
    // Shrink does not require to POST any message, it has only CPU effects (not GPU)

    if ( _size == _capacity )
        return ;
    gal_ubyte* chunk = new gal_ubyte[_size];
    memcpy(chunk, _data, _size);
    delete[] _data;
    _data = chunk;
}

gal_uint GALBufferImp::capacity() const
{
    return _capacity;
}


gal_uint GALBufferImp::getUID ()
{
    return _UID;
}

void GALBufferImp::dumpBuffer (ofstream& out, GAL_STREAM_DATA type, gal_uint components, gal_uint stride, gal_uint offset)
{
    gal_uint numElem = _size/getSize(type);
    gal_uint i = offset/getSize(type);
    gal_uint strideElem = stride/getSize(type);

    out << "{";
    switch ( type )
    {
        case GAL_SD_UNORM8:
        case GAL_SD_SNORM8:
        case GAL_SD_UINT8:
        case GAL_SD_SINT8:
        case GAL_SD_INV_UNORM8:
        case GAL_SD_INV_SNORM8:
        case GAL_SD_INV_UINT8:
        case GAL_SD_INV_SINT8:
            for ( ; i < numElem; )
            {
                out << "(";
                for (int j = 0; j < components; j++)
                {
                    if (j != 0) out << ",";

                    out << ((gal_ubyte*)_data)[i];

                    i++;
                }

                if (strideElem != 0)
                    i = i + (strideElem - components);

                out << ")";

                if(i<numElem) out << ",";

            }
        case GAL_SD_UNORM16:
        case GAL_SD_SNORM16:
        case GAL_SD_UINT16:
        case GAL_SD_SINT16:
        case GAL_SD_FLOAT16:
        case GAL_SD_INV_UNORM16:
        case GAL_SD_INV_SNORM16:
        case GAL_SD_INV_UINT16:
        case GAL_SD_INV_SINT16:
        case GAL_SD_INV_FLOAT16:
            for ( ; i < numElem; )
            {
                out << "(";
                for (int j = 0; j < components; j++)
                {
                    if (j != 0) out << ",";

                    out << ((gal_ushort*)_data)[i];

                    i++;
                }

                if (strideElem != 0)
                    i = i + (strideElem - components);

                out << ")";

                if(i<numElem) out << ",";

            }
        case GAL_SD_UNORM32:
        case GAL_SD_SNORM32:
        case GAL_SD_UINT32:
        case GAL_SD_SINT32:
        case GAL_SD_INV_UNORM32:
        case GAL_SD_INV_SNORM32:
        case GAL_SD_INV_UINT32:
        case GAL_SD_INV_SINT32:
            for ( ; i < numElem; )
            {
                out << "(";
                for (int j = 0; j < components; j++)
                {
                    if (j != 0) out << ",";

                    out << ((gal_uint*)_data)[i];

                    i++;
                }

                if (strideElem != 0)
                    i = i + (strideElem - components);

                out << ")";

                if(i<numElem) out << ",";

            }
        case GAL_SD_FLOAT32:
        case GAL_SD_INV_FLOAT32:
            for ( ; i < numElem; )
            {
                out << "(";
                for (int j = 0; j < components; j++)
                {
                    if (j != 0) out << ",";

                    out << ((gal_float*)_data)[i];

                    i++;
                }

                if (strideElem != 0)
                    i = i + (strideElem - components);

                out << ")";

                if(i<numElem) out << ",";

            }
            break;
        default:
            CG_ASSERT("Unsupported indices type");
    }
    out << "}" << endl;

}

gal_uint GALBufferImp::getSize (GAL_STREAM_DATA type)
{
    switch ( type )
    {
        case GAL_SD_UNORM8:
        case GAL_SD_SNORM8:
        case GAL_SD_UINT8:
        case GAL_SD_SINT8:
        case GAL_SD_INV_UNORM8:
        case GAL_SD_INV_SNORM8:
        case GAL_SD_INV_UINT8:
        case GAL_SD_INV_SINT8:
            return sizeof(gal_ubyte);
            break;

        case GAL_SD_UNORM16:
        case GAL_SD_SNORM16:
        case GAL_SD_UINT16:
        case GAL_SD_SINT16:
        case GAL_SD_FLOAT16:
        case GAL_SD_INV_UNORM16:
        case GAL_SD_INV_SNORM16:
        case GAL_SD_INV_UINT16:
        case GAL_SD_INV_SINT16:
        case GAL_SD_INV_FLOAT16:
            return sizeof(gal_ushort);
            break;

        case GAL_SD_UNORM32:
        case GAL_SD_SNORM32:
        case GAL_SD_UINT32:
        case GAL_SD_SINT32:
        case GAL_SD_INV_UNORM32:
        case GAL_SD_INV_SNORM32:
        case GAL_SD_INV_UINT32:
        case GAL_SD_INV_SINT32:
            return sizeof(gal_uint);
            break;

        case GAL_SD_FLOAT32:
        case GAL_SD_INV_FLOAT32:
            return sizeof(gal_float);
            break;

        default:
            CG_ASSERT("Type size not available");
            return 0;  // To remove compiler warning;
            break;
    }
}
