/**************************************************************************
 *
 */

#ifndef GAL_STREAM_IMP
    #define GAL_STREAM_IMP

#include "GALStream.h"
#include "StateItem.h"
#include "StateComponent.h"
#include "HAL.h"
#include "StateItemUtils.h"
#include "GALStoredStateImp.h"
#include "GPUReg.h"

namespace libGAL
{
class GALDeviceImp;
class GALBufferImp;

class GALStreamImp : public GALStream, public StateComponent
{
public:

    GALStreamImp(GALDeviceImp* device, HAL* driver, gal_uint streamID);

    virtual void set( GALBuffer* buffer, const GAL_STREAM_DESC& desc );

    virtual void get( GALBuffer*& buffer,
                      gal_uint& offset,
                      gal_uint& components,
                      GAL_STREAM_DATA& componentsType,
                      gal_uint& stride,
                      gal_uint& frequency) const;

    virtual void setBuffer(GALBuffer* buffer);

    virtual void setOffset(gal_uint offset);

    virtual void setComponents(gal_uint components);

    virtual void setType(GAL_STREAM_DATA componentsType);
    
    virtual void setStride(gal_uint stride);
    
    virtual void setFrequency(gal_uint stride);
    
    virtual GALBuffer* getBuffer() const;
    
    virtual gal_uint getOffset() const;
    
    virtual gal_uint getComponents() const;
    
    virtual GAL_STREAM_DATA getType() const;
    
    virtual gal_uint getStride() const;
    
    virtual gal_uint getFrequency() const;

    virtual std::string DBG_getState() const;

    void sync();

    void forceSync();

    std::string getInternalState() const;

    void GALDumpStream();

    const StoredStateItem* createStoredStateItem(GAL_STORED_ITEM_ID stateId) const;

    void restoreStoredStateItem(const StoredStateItem* ssi);

    GALStoredState* saveAllState() const;

    void restoreAllState(const GALStoredState* ssi);

private:

    const gal_uint _STREAM_ID;

    HAL* _driver;
    GALDeviceImp* _device;

    gal_bool _requiredSync;
    gal_uint _gpuMemTrack;
    
    StateItem<GALBufferImp*> _buffer;
    StateItem<gal_uint> _offset;
    StateItem<gal_uint> _components;
    StateItem<GAL_STREAM_DATA> _componentsType;
    StateItem<gal_uint> _stride;
    StateItem<gal_uint> _frequency;

    StateItem<gal_bool> _invertStreamType;

    static void _convertToGPUStreamData(GAL_STREAM_DATA componentsType, arch::GPURegData* data);

    // print help classes
    
    class ComponentTypePrint: public PrintFunc<GAL_STREAM_DATA>
    {
    public:

        virtual const char* print(const GAL_STREAM_DATA& var) const;        
    }
    componentTypePrint;
};

} // namespace libGAL


#endif // GAL_STREAM_IMP

