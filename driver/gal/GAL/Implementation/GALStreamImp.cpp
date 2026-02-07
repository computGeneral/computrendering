/**************************************************************************
 *
 */

#include "GALStreamImp.h"
#include "GALDeviceImp.h"
#include "GALBufferImp.h"
#include "GALMacros.h"
#include <sstream>
#include "StateItemUtils.h"

#include "GlobalProfiler.h"

using namespace std;
using namespace libGAL;


GALStreamImp::GALStreamImp(GALDeviceImp* device, HAL* driver, gal_uint streamID) : 
    _driver(driver), _device(device), _STREAM_ID(streamID), _requiredSync(true), _gpuMemTrack(0),
    _buffer(0),
    _offset(0),
    _components(0),
    _componentsType(GAL_SD_UINT8),
    _stride(0),
    _frequency(0),
    _invertStreamType(false)
{
    forceSync(); // init GPU StreamController(i) state registers    
}

void GALStreamImp::set( GALBuffer* buffer, const GAL_STREAM_DESC& desc )
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    
    GALBufferImp* buf = static_cast<GALBufferImp*>(buffer);
    _buffer = buf;
    _gpuMemTrack = 0; // Init buffer track

    _offset = desc.offset;
    _components = desc.components;
    _componentsType = desc.type;
    _stride = desc.stride;
    _frequency = desc.frequency;

    switch(_componentsType)
    {
        case GAL_SD_UNORM8:
        case GAL_SD_SNORM8:
        case GAL_SD_UNORM16:
        case GAL_SD_SNORM16:
        case GAL_SD_UNORM32:
        case GAL_SD_SNORM32:
        case GAL_SD_FLOAT16:
        case GAL_SD_FLOAT32:
        case GAL_SD_UINT8:
        case GAL_SD_SINT8:
        case GAL_SD_UINT16:
        case GAL_SD_SINT16:
        case GAL_SD_UINT32:
        case GAL_SD_SINT32:
            _invertStreamType = false;
            break;
        case GAL_SD_INV_UNORM8:
        case GAL_SD_INV_SNORM8:
        case GAL_SD_INV_UNORM16:
        case GAL_SD_INV_SNORM16:
        case GAL_SD_INV_UNORM32:
        case GAL_SD_INV_SNORM32:
        case GAL_SD_INV_FLOAT16:
        case GAL_SD_INV_FLOAT32:
        case GAL_SD_INV_UINT8:
        case GAL_SD_INV_SINT8:
        case GAL_SD_INV_UINT16:
        case GAL_SD_INV_SINT16:
        case GAL_SD_INV_UINT32:
        case GAL_SD_INV_SINT32:
            _invertStreamType = true;
            break;
        default:
            CG_ASSERT("Undefined stream data format.");            
        
    }
        
    GLOBAL_PROFILER_EXIT_REGION()
}

void GALStreamImp::get( GALBuffer*& buffer,
                        gal_uint& offset,
                        gal_uint& components,
                        GAL_STREAM_DATA& componentsType,
                        gal_uint& stride,
                        gal_uint &frequency) const
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    buffer = _buffer;
    offset = _offset;
    components = _components;
    componentsType = _componentsType;
    stride = _stride;
    frequency = _frequency;
    GLOBAL_PROFILER_EXIT_REGION()
}

void GALStreamImp::setBuffer(GALBuffer* buffer) 
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    GALBufferImp* buf = static_cast<GALBufferImp*>(buffer);
    _buffer = buf;
    _gpuMemTrack = 0; // Init buffer track
    GLOBAL_PROFILER_EXIT_REGION()
}

void GALStreamImp::setOffset(gal_uint offset)
{
    _offset = offset;
}

void GALStreamImp::setComponents(gal_uint components)
{
_components = components;
}

void GALStreamImp::setType(GAL_STREAM_DATA componentsType)
{
    _componentsType = componentsType;
}
    
void GALStreamImp::setStride(gal_uint stride)
{
    _stride = stride;
}

void GALStreamImp::setFrequency(gal_uint frequency)
{
    _frequency = frequency;
}

GALBuffer* GALStreamImp::getBuffer() const
{
    return _buffer;
}
    
gal_uint GALStreamImp::getOffset() const
{
    return _offset;
}

gal_uint GALStreamImp::getComponents() const
{
    return _components;
}
    
GAL_STREAM_DATA GALStreamImp::getType() const
{
    return _componentsType;
}
    
gal_uint GALStreamImp::getStride() const
{
    return _stride;
}

gal_uint GALStreamImp::getFrequency() const
{
    return _frequency;
}

void GALStreamImp::sync()
{
    cg1gpu::GPURegData data;
    
    // Buffer synchronization is always required
    GALBufferImp* buf = _buffer;

    if ( buf != 0 ) {
        _device->allocator().syncGPU(buf); // syncronize buffer contents

        //if ( _buffer.changed() || (_buffer.initial()->getUID()!=buf->getUID()) || _offset.changed() || _gpuMemTrack != buf->trackRealloc(0) || _requiredSync ) {
            // Stream address requires to be recomputed
            gal_uint md = _device->allocator().md(buf);
            _driver->writeGPUAddrRegister(cg1gpu::GPU_STREAM_ADDRESS, _STREAM_ID, md, _offset);
            // reset IF conditions
            _buffer.restart();
            _offset.restart();
            _gpuMemTrack = buf->trackRealloc(0); // restart buffer tracking
        //}

        #ifdef GAL_DUMP_STREAMS
            GALDumpStream();
        #endif
    }
  
    if ( _components.changed() || _requiredSync )
    {
        data.uintVal = _components;
        _driver->writeGPURegister(cg1gpu::GPU_STREAM_ELEMENTS, _STREAM_ID, data);
        _components.restart();
    }

    if (_frequency.changed() || _requiredSync)
    {
        data.uintVal = _frequency;
        _driver->writeGPURegister(cg1gpu::GPU_STREAM_FREQUENCY, _STREAM_ID, data);
        _frequency.restart();
    }
    
    if ( _componentsType.changed() || _requiredSync )
    {
        _convertToGPUStreamData(_componentsType, &data);
        _driver->writeGPURegister(cg1gpu::GPU_STREAM_DATA, _STREAM_ID, data);
        _componentsType.restart();
    }

    if ( _invertStreamType.changed() || _requiredSync )
    {
        data.booleanVal = _invertStreamType;
        _driver->writeGPURegister(cg1gpu::GPU_D3D9_COLOR_STREAM, _STREAM_ID, data);
        _invertStreamType.restart();
    }

    if ( _stride.changed() || _requiredSync )
    {
        data.uintVal = _stride;
        _driver->writeGPURegister(cg1gpu::GPU_STREAM_STRIDE, _STREAM_ID, data);
        _stride.restart();
    }
}

const StoredStateItem* GALStreamImp::createStoredStateItem(GAL_STORED_ITEM_ID stateId) const
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    GALStoredStateItem* ret;
    gal_uint rTarget;

    if (stateId >= GAL_STREAM_FIRST_ID && stateId < GAL_STREAM_LAST)
    {    
        // It�s a blending state
        switch(stateId)
        {
            case GAL_STREAM_ELEMENTS:       ret = new GALSingleEnumStoredStateItem((gal_enum)_components);          break;
            case GAL_STREAM_FREQUENCY:      ret = new GALSingleEnumStoredStateItem((gal_enum) _frequency);          break;
            case GAL_STREAM_DATA_TYPE:      ret = new GALSingleEnumStoredStateItem((gal_enum)_componentsType);      break;
            case GAL_STREAM_D3D9_COLOR:     ret = new GALSingleEnumStoredStateItem((gal_enum)_invertStreamType);    break;
            case GAL_STREAM_STRIDE:         ret = new GALSingleEnumStoredStateItem((gal_enum)_stride);              break;
            case GAL_STREAM_BUFFER:         ret = new GALSingleVoidStoredStateItem(_buffer);                        break;
            // case GAL_STREAM_... <- add here future additional blending states.

            default:
                CG_ASSERT("Unexpected blending state id");
        }          
        // else if (... <- write here for future additional defined blending states.
    }
    else
        CG_ASSERT("Unexpected blending state id");

    ret->setItemId(stateId);

    GLOBAL_PROFILER_EXIT_REGION()

    return ret;
}

#define CAST_TO_UINT(X)         *(static_cast<const GALSingleUintStoredStateItem*>(X))
#define CAST_TO_BOOL(X)         *(static_cast<const GALSingleBoolStoredStateItem*>(X))
#define CAST_TO_VOID(X)         static_cast<void *>(*(static_cast<const GALSingleVoidStoredStateItem*> (X)))
#define STREAM_DATA(DST,X)      { const GALSingleEnumStoredStateItem* aux = static_cast<const GALSingleEnumStoredStateItem*>(X); gal_enum value = *aux; DST = static_cast<GAL_STREAM_DATA>(value); }

void GALStreamImp::restoreStoredStateItem(const StoredStateItem* ssi)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")
    gal_uint rTarget;

    const GALStoredStateItem* galssi = static_cast<const GALStoredStateItem*>(ssi);

    GAL_STORED_ITEM_ID stateId = galssi->getItemId();

    if (stateId >= GAL_STREAM_FIRST_ID && stateId < GAL_STREAM_LAST)
    {    
        // It�s a blending state
        switch(stateId)
        {
            case GAL_STREAM_ELEMENTS:       _components = CAST_TO_UINT(galssi);                             break;
            case GAL_STREAM_FREQUENCY:      _frequency = CAST_TO_UINT(galssi);                              break;
            case GAL_STREAM_DATA_TYPE:      STREAM_DATA(_componentsType, galssi);                           break;
            case GAL_STREAM_D3D9_COLOR:     _invertStreamType = CAST_TO_BOOL(galssi);                       break;
            case GAL_STREAM_STRIDE:         _stride = CAST_TO_UINT(galssi);                                 break;
            case GAL_STREAM_BUFFER:         _buffer = static_cast<GALBufferImp*>(CAST_TO_VOID(galssi));     break;

            // case GAL_STREAM_... <- add here future additional blending states.
            default:
                CG_ASSERT("Unexpected blending state id");
        }           
        // else if (... <- write here for future additional defined blending states.
    }
    else
        CG_ASSERT("Unexpected blending state id");

    GLOBAL_PROFILER_EXIT_REGION()
}

#undef CAST_TO_UINT
#undef CAST_TO_BOOL
#undef CAST_TO_VOID
#undef STREAM_DATA

GALStoredState* GALStreamImp::saveAllState() const
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")    
    GALStoredStateImp* ret = new GALStoredStateImp();

    for (gal_uint i = 0; i < GAL_LAST_STATE; i++)
    {
        if (i < GAL_LAST_STATE)
        {
            // It�s a rasterization stage
            ret->addStoredStateItem(createStoredStateItem(GAL_STORED_ITEM_ID(i)));
        }
        else
            CG_ASSERT("Unexpected Stored Item Id");
    }

    GLOBAL_PROFILER_EXIT_REGION()
    
    return ret;
}

void GALStreamImp::restoreAllState(const GALStoredState* state)
{
    GLOBAL_PROFILER_ENTER_REGION("GAL", "", "")    

    const GALStoredStateImp* ssi = static_cast<const GALStoredStateImp*>(state);

    std::list<const StoredStateItem*> ssiList = ssi->getSSIList();

    std::list<const StoredStateItem*>::const_iterator iter = ssiList.begin();

    while ( iter != ssiList.end() )
    {
        const GALStoredStateItem* galssi = static_cast<const GALStoredStateItem*>(*iter);

        if (galssi->getItemId() < GAL_LAST_STATE)
        {
            // It�s a blending stage
            restoreStoredStateItem(galssi);
        }
        else
            CG_ASSERT("Unexpected Stored Item Id");

        iter++;
    }

    GLOBAL_PROFILER_EXIT_REGION()
}

void GALStreamImp::GALDumpStream()
{

    ofstream out;
    
    out.open("GALDumpStream.txt",ios::app);

    if ( !out.is_open() )
        CG_ASSERT("Dump failed (output file could not be opened)");


    out << " Dumping Stream " << _STREAM_ID << endl;
    out << "\t Stride: " << _stride << endl;
    out << "\t Data type: " << _componentsType << endl;
    out << "\t Num. elemets: " << _components << endl;
    out << "\t Frequency: " << _frequency << endl;
    out << "\t Offset: " << _offset << endl;
    out << "\t Dumping stream content: " << endl;

    ((GALBufferImp*)_buffer)->dumpBuffer(out, _componentsType, _components, _stride, _offset);

    out.close();

    

}
string GALStreamImp::DBG_getState() const
{
    return getInternalState();
}

void GALStreamImp::_convertToGPUStreamData(GAL_STREAM_DATA componentsType, cg1gpu::GPURegData* data)
{
    switch ( componentsType )
    {
        case GAL_SD_UNORM8:
        case GAL_SD_INV_UNORM8:
            data->streamData = cg1gpu::SD_UNORM8;
            break;
        case GAL_SD_SNORM8:
        case GAL_SD_INV_SNORM8:
            data->streamData = cg1gpu::SD_SNORM8;
            break;
        case GAL_SD_UNORM16:
        case GAL_SD_INV_UNORM16:
            data->streamData = cg1gpu::SD_UNORM16;
            break;
        case GAL_SD_SNORM16:
        case GAL_SD_INV_SNORM16:
            data->streamData = cg1gpu::SD_SNORM16;
            break;
        case GAL_SD_UNORM32:
        case GAL_SD_INV_UNORM32:
            data->streamData = cg1gpu::SD_UNORM32;
            break;
        case GAL_SD_SNORM32:
        case GAL_SD_INV_SNORM32:
            data->streamData = cg1gpu::SD_SNORM32;
            break;
        case GAL_SD_FLOAT16:
        case GAL_SD_INV_FLOAT16:
            data->streamData = cg1gpu::SD_FLOAT16;
            break;
        case GAL_SD_FLOAT32:
        case GAL_SD_INV_FLOAT32:
            data->streamData = cg1gpu::SD_FLOAT32;
            break;
        case GAL_SD_UINT8:
        case GAL_SD_INV_UINT8:
            data->streamData = cg1gpu::SD_UINT8;
            break;
        case GAL_SD_SINT8:
        case GAL_SD_INV_SINT8:
            data->streamData = cg1gpu::SD_SINT8;
            break;
        case GAL_SD_UINT16:
        case GAL_SD_INV_UINT16:
            data->streamData = cg1gpu::SD_UINT16;
            break;
        case GAL_SD_SINT16:
        case GAL_SD_INV_SINT16:
            data->streamData = cg1gpu::SD_SINT16;
            break;
        case GAL_SD_UINT32:
        case GAL_SD_INV_UINT32:
            data->streamData = cg1gpu::SD_UINT32;
            break;
        case GAL_SD_SINT32:
        case GAL_SD_INV_SINT32:
            data->streamData = cg1gpu::SD_SINT32;
            break;                        
        default:
            CG_ASSERT("Unknown GAL_STREAM_DATA value");
    }
}

void GALStreamImp::forceSync()
{
    _requiredSync = true;
    sync();
    _requiredSync = false;
}

string GALStreamImp::getInternalState() const
{
    stringstream ss, ssAux;

    ssAux << "STREAM" << _STREAM_ID;

    ss << ssAux.str() << "_GPU_MEM_TRACK = " << _gpuMemTrack << "\n";
    ss << ssAux.str() << "_BUFFER = " << hex << static_cast<GALBufferImp*>(_buffer) << dec;
    
    if ( _requiredSync )
        ss << " NotSync (?)\n";
    else {
        if ( _buffer.changed() )
            ss << " NotSync (" << hex << static_cast<GALBufferImp*>(_buffer.initial()) << dec << ")\n";
        else
            ss << " Sync\n";
    }

    ss << ssAux.str() << stateItemString(_offset,"_OFFSET",false);

    ss << ssAux.str() << stateItemString(_components,"_COMPONENTS",false);

    ss << ssAux.str() << stateItemString(_componentsType,"_COMPONENTS_TYPE",false,&componentTypePrint);
    
    ss << ssAux.str() << stateItemString(_stride,"_STRIDE",false);

    ss << ssAux.str() << stateItemString(_frequency,"_FREQUENCY",false);

    return ss.str();
}

const char* GALStreamImp::ComponentTypePrint::print(const GAL_STREAM_DATA& var) const
{
    switch ( var ) 
    {
        case GAL_SD_UNORM8:
            return "SD_UNORM8"; break;
        case GAL_SD_SNORM8:
            return "SD_SNORM8"; break;
        case GAL_SD_UNORM16:
            return "SD_UNORM16"; break;
        case GAL_SD_SNORM16:
            return "SD_SNORM16"; break;
        case GAL_SD_UNORM32:
            return "SD_UNORM32"; break;
        case GAL_SD_SNORM32:
            return "SD_SNORM32"; break;
        case GAL_SD_FLOAT16:
            return "SD_FLOAT16"; break;
        case GAL_SD_FLOAT32:
            return "SD_FLOAT32"; break;
        case GAL_SD_UINT8:
            return "SD_UINT8"; break;
        case GAL_SD_SINT8:
            return "SD_SINT8"; break;
        case GAL_SD_UINT16:
            return "SD_UINT16"; break;
        case GAL_SD_SINT16:
            return "SD_SINT16"; break;
        case GAL_SD_UINT32:
            return "SD_UINT32"; break;
        case GAL_SD_SINT32:
            return "SD_SINT32"; break;
                                                
        case GAL_SD_INV_UNORM8:
            return "SD_INV_UNORM8"; break;
        case GAL_SD_INV_SNORM8:
            return "SD_INV_SNORM8"; break;
        case GAL_SD_INV_UNORM16:
            return "SD_INV_UNORM16"; break;
        case GAL_SD_INV_SNORM16:
            return "SD_INV_SNORM16"; break;
        case GAL_SD_INV_UNORM32:
            return "SD_INV_UNORM32"; break;
        case GAL_SD_INV_SNORM32:
            return "SD_INV_SNORM32"; break;
        case GAL_SD_INV_FLOAT16:
            return "SD_INV_FLOAT16"; break;
        case GAL_SD_INV_FLOAT32:
            return "SD_INV_FLOAT16"; break;
        case GAL_SD_INV_UINT8:
            return "SD_INV_UINT8"; break;
        case GAL_SD_INV_SINT8:
            return "SD_INV_SINT8"; break;
        case GAL_SD_INV_UINT16:
            return "SD_INV_UINT16"; break;
        case GAL_SD_INV_SINT16:
            return "SD_INV_SINT16"; break;
        case GAL_SD_INV_UINT32:
            return "SD_INV_UINT32"; break;
        case GAL_SD_INV_SINT32:
            return "SD_INV_SINT32"; break;

        default:
            return "UNKNOWN";
    }
}
