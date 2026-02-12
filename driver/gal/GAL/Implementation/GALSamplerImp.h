/**************************************************************************
 *
 */

#ifndef GAL_SAMPLER_IMP
    #define GAL_SAMPLER_IMP

#include "GALSampler.h"
#include "StateComponent.h"
#include "StateItem.h"
#include "HAL.h"
#include "GALResource.h"
#include "TextureMipmap.h"
#include "GPUReg.h"

namespace libGAL
{

class GALDeviceImp;

class GALSamplerImp : public GALSampler, public StateComponent
{
public:

    GALSamplerImp(GALDeviceImp* device, HAL* driver, gal_uint samplerID);

    virtual void setEnabled(gal_bool enable);

    virtual gal_bool isEnabled() const;

    virtual void setTexture(GALTexture* texture);

    virtual void setTextureAddressMode(GAL_TEXTURE_COORD coord, GAL_TEXTURE_ADDR_MODE mode);

    virtual void setNonNormalizedCoordinates(gal_bool enable);
    
    virtual void setMinFilter(GAL_TEXTURE_FILTER minFilter);

    virtual void setMagFilter(GAL_TEXTURE_FILTER magFilter);
    
    virtual void setEnableComparison(gal_bool enable);
    
    virtual void setComparisonFunction(GAL_TEXTURE_COMPARISON function);

    virtual void setSRGBConversion(gal_bool enable);

    virtual void setMinLOD(gal_float minLOD);

    virtual void setMaxLOD(gal_float maxLOD);

    virtual void setMaxAnisotropy(gal_uint maxAnisotropy);

    virtual void setLODBias(gal_float lodBias);

    virtual void setUnitLODBias(gal_float unitLodBias);

    virtual void setMinLevel(gal_uint minLevel);
    
    virtual GAL_TEXTURE_FILTER getMagFilter() const;

    virtual GALTexture* getTexture() const;

    virtual GAL_TEXTURE_FILTER getMinFilter() const;

    virtual std::string getInternalState() const;

    virtual void sync();

    virtual void forceSync();

    void performBlitOperation2D(gal_uint xoffset, gal_uint yoffset, gal_uint x, gal_uint y, gal_uint width, gal_uint height, gal_uint textureWidth, GAL_FORMAT internalFormat, GALTexture2D* texture, gal_uint level);
    
    const StoredStateItem* createStoredStateItem(GAL_STORED_ITEM_ID stateId) const;

    void restoreStoredStateItem(const StoredStateItem* ssi);

private:


    static const gal_uint _MAX_TEXTURE_MIPMAPS = arch::MAX_TEXTURE_SIZE;

    GALDeviceImp* _device;
    HAL* _driver;
    gal_bool _requiredSync;

    // Maximum number of 2D images per texture
    std::vector<gal_uint> _gpuMemTrack; // MIPMAPS * CUBEMAP_FACES
    
    const gal_uint _SAMPLER_ID;

    GALTexture* _texture;

    /// Last texture-dependant values used in this sampler
    StateItem<GAL_RESOURCE_TYPE> _textureType;
    StateItem<GAL_FORMAT> _textureFormat;
    StateItem<gal_uint> _minLevel;
    StateItem<gal_uint> _baseLevel;
    StateItem<gal_uint> _maxLevel;
    StateItem<gal_uint> _textureWidth;
    StateItem<gal_uint> _textureHeight;
    StateItem<gal_uint> _textureDepth;
    StateItem<gal_uint> _textureWidth2;
    StateItem<gal_uint> _textureHeight2;
    StateItem<gal_uint> _textureDepth2;
    StateItem<GAL_MEMORY_LAYOUT> _memoryLayout;
    
    // Sampler dependant values
    StateItem<gal_bool> _enabled;
    StateItem<GAL_TEXTURE_ADDR_MODE> _sCoord;
    StateItem<GAL_TEXTURE_ADDR_MODE> _tCoord;
    StateItem<GAL_TEXTURE_ADDR_MODE> _rCoord;
    StateItem<gal_bool> _nonNormalizedCoords;
    StateItem<GAL_TEXTURE_FILTER> _minFilter;
    StateItem<GAL_TEXTURE_FILTER> _magFilter;
    StateItem<gal_bool> _enableComparison;
    StateItem<GAL_TEXTURE_COMPARISON> _comparisonFunction;
    StateItem<gal_bool> _sRGBConversion;
    StateItem<gal_float> _minLOD;
    StateItem<gal_float> _maxLOD;
    StateItem<gal_uint> _maxAniso;
    StateItem<gal_float> _lodBias;
    StateItem<gal_float> _unitLodBias;

    // Blit dependent values
    StateItem<gal_int> _blitXoffset;
    StateItem<gal_int> _blitYoffset;
    StateItem<gal_int >_blitIniX;
    StateItem<gal_int >_blitIniY;
    StateItem<gal_int> _blitHeight;
    StateItem<gal_int> _blitWidth;
    StateItem<gal_int> _blitWidth2;
    StateItem<GAL_FORMAT> _blitFormat;
    StateItem<GAL_MEMORY_LAYOUT> _blitTextureBlocking;

    static void _getGPUTextureFormat(GAL_FORMAT textureFormat, 
                                         arch::GPURegData* dataFormat,
                                         arch::GPURegData* dataCompression,
                                         arch::GPURegData* dataInverted);

    static void _getGPUClampMode(GAL_TEXTURE_ADDR_MODE mode, arch::GPURegData* data);
    static void _getGPUTexFilter(GAL_TEXTURE_FILTER, arch::GPURegData* data);
    static void _getGPUTexComp(GAL_TEXTURE_COMPARISON, arch::GPURegData* data);
    static void _getGPUTextureMode(GAL_RESOURCE_TYPE, arch::GPURegData* data);
    static void _getGPUTexMemoryLayout(GAL_MEMORY_LAYOUT memLayout, arch::GPURegData* data);

    static const char* _getFormatString(GAL_FORMAT format);

    void _syncTexture();



    // print help classes

    BoolPrint boolPrint;

    class AddressModePrint: public PrintFunc<GAL_TEXTURE_ADDR_MODE>
    {
    public:

        virtual const char* print(const GAL_TEXTURE_ADDR_MODE& var) const;
    }
    addressModePrint;

    class FilterPrint: public PrintFunc<GAL_TEXTURE_FILTER>
    {
    public:

        virtual const char* print(const GAL_TEXTURE_FILTER& var) const;
    }
    filterPrint;

    class CompFuncPrint: public PrintFunc<GAL_TEXTURE_COMPARISON>
    {
    public:
        virtual const char* print(const GAL_TEXTURE_COMPARISON& var) const;
    }
    compFuncPrint;
    
};


}

#endif // GAL_SAMPLER_IMP
