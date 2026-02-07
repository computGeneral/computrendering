/**************************************************************************
 *
 */

#ifndef GAL_TEXTURE3D_IMP
    #define GAL_TEXTURE3D_IMP

#include "GALTexture3D.h"
#include "TextureMipmapChain.h"
#include "TextureMipmap.h"
#include "MemoryObject.h"
#include <set>

namespace libGAL
{

class GALTexture3DImp : public GALTexture3D, public MemoryObject
{
public:

    GALTexture3DImp();

    /// Methods inherited from GALResource interface

    virtual void setUsage(GAL_USAGE usage);

    virtual GAL_USAGE getUsage() const;

    virtual void setMemoryLayout(GAL_MEMORY_LAYOUT layout);
    
    virtual GAL_MEMORY_LAYOUT getMemoryLayout() const;
    
    virtual GAL_RESOURCE_TYPE getType() const;

    virtual void setPriority(gal_uint prio);

    virtual gal_uint getPriority() const;

    virtual gal_uint getSubresources() const;

    virtual gal_bool wellDefined() const;

    /// Methods inherited form GALTexture interface

    virtual gal_uint getBaseLevel() const;

    virtual gal_uint getMaxLevel() const;

    virtual void setBaseLevel(gal_uint minMipLevel);

    virtual void setMaxLevel(gal_uint maxMipLevel);

	virtual gal_uint getSettedMipmaps();

    /// Methods inherited from GALTexture2D interface

    virtual gal_uint getWidth(gal_uint mipmap) const;

    virtual gal_uint getHeight(gal_uint mipmap) const;
    
    virtual gal_uint getDepth(gal_uint mipLevel) const;

    virtual GAL_FORMAT getFormat(gal_uint mipmap) const;

    virtual gal_bool isMultisampled(gal_uint mipmap) const;
    
    virtual gal_uint getSamples(gal_uint mipmap) const;
    
    virtual gal_uint getTexelSize(gal_uint mipmap) const;

    virtual void setData( gal_uint mipLevel,
                          gal_uint width,
                          gal_uint height,
                          gal_uint depth,
                          GAL_FORMAT format,
                          gal_uint rowPitch,
                          const gal_ubyte* srcTexelData,
                          gal_uint texSize,
                          gal_bool preloadData = false);

    virtual void setData(gal_uint mipLevel,
                         gal_uint width,
                         gal_uint height,
                         gal_uint depth,
                         gal_bool multisampling,
                         gal_uint samples,
                         GAL_FORMAT format);

    virtual void updateData( gal_uint mipLevel,
                             gal_uint x,
                             gal_uint y,
                             gal_uint z,
                             gal_uint width,
                             gal_uint height,
                             gal_uint depth,
                             GAL_FORMAT format,
                             gal_uint rowPitch,
                             const gal_ubyte* srcTexelData,
                             gal_bool preloadData = false);

    virtual gal_bool map( gal_uint mipLevel,
                      GAL_MAP mapType,
                      gal_ubyte*& pData,
                      gal_uint& dataRowPitch,
                      gal_uint& dataPlanePitch);

    virtual gal_bool unmap(gal_uint mipLevel, gal_bool preloadData = false);

    /// Method required by MemoryObject derived classes
    virtual const gal_ubyte* memoryData(gal_uint region, gal_uint& memorySizeInBytes) const;

    virtual const gal_char* stringType() const;

    void dumpMipmap(gal_uint region, gal_ubyte* mipName);

    const gal_ubyte* getData(gal_uint mipLevel, gal_uint& memorySizeInBytes, gal_uint& rowPitch, gal_uint& planePitch) const;


private:

    const TextureMipmap* _getMipmap(gal_uint mipLevel, const gal_char* methodStr) const;

    gal_uint _baseLevel;
    gal_uint _maxLevel;
    TextureMipmapChain _mips;
    std::set<gal_uint> _mappedMips;
    
    GAL_MEMORY_LAYOUT layout;
    
}; // class GALTexture3DImp


}

#endif // GAL_TEXTURE3D_IMP
