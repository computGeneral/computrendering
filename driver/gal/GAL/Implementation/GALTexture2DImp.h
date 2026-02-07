/**************************************************************************
 *
 */

#ifndef GAL_TEXTURE2D_IMP
    #define GAL_TEXTURE2D_IMP

#include "GALTexture2D.h"
#include "TextureMipmapChain.h"
#include "TextureMipmap.h"
#include "MemoryObject.h"
#include <set>

namespace libGAL
{

class GALTexture2DImp : public GALTexture2D, public MemoryObject
{
public:

    GALTexture2DImp();

    /// Methods inherited from GALResource interface

    virtual void setUsage(GAL_USAGE usage); // Afegit

    virtual GAL_USAGE getUsage() const; // Afegit

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

    virtual GAL_FORMAT getFormat(gal_uint mipmap) const;

    virtual gal_bool isMultisampled(gal_uint mipmap) const;
    
    virtual gal_uint getSamples(gal_uint mipmap) const;
    
    virtual gal_uint getTexelSize(gal_uint mipmap) const;

    virtual void copyData( gal_uint srcMipLevel, gal_uint dstMipLevel, GALTexture2D* destTexture);

    virtual void setData( gal_uint mipLevel,
                          gal_uint width,
                          gal_uint height,
                          GAL_FORMAT format,
                          gal_uint rowPitch,
                          const gal_ubyte* srcTexelData,
                          gal_uint texSize,
                          gal_bool preloadData = false);

    virtual void setData(gal_uint mipLevel,
                         gal_uint width,
                         gal_uint height,
                         gal_bool multisampling,
                         gal_uint samples,
                         GAL_FORMAT format);

    virtual void updateData( gal_uint mipLevel, const gal_ubyte* srcTexelData, gal_bool preloadData);

    virtual void updateData( gal_uint mipLevel,
                             gal_uint x,
                             gal_uint y,
                             gal_uint width,
                             gal_uint height,
                             GAL_FORMAT format,
                             gal_uint rowPitch,
                             const gal_ubyte* srcTexelData,
                             gal_bool preloadData = false);

    virtual gal_bool map( gal_uint mipLevel,
                      GAL_MAP mapType,
                      gal_ubyte*& pData,
                      gal_uint& dataRowPitch );

    virtual gal_bool unmap(gal_uint mipLevel, gal_bool preloadData = false);

    /// Method required by MemoryObject derived classes
    virtual const gal_ubyte* memoryData(gal_uint region, gal_uint& memorySizeInBytes) const;

    virtual const gal_char* stringType() const;

    void dumpMipmap(gal_uint region, gal_ubyte* mipName);

    const gal_ubyte* getData(gal_uint mipLevel, gal_uint& memorySizeInBytes, gal_uint& rowPitch) const;

    static gal_ubyte* compressTexture(GAL_FORMAT originalFormat, GAL_FORMAT compressFormat, gal_uint width, gal_uint height, gal_ubyte* originalData);

    static gal_uint compressionDifference(gal_uint texel, gal_uint compare, gal_uint rowSize, gal_ubyte* data);

    static gal_ushort convertR5G6B5(gal_uint originalRGB);

    static gal_uint getTexelSize(GAL_FORMAT format) ;

private:

    const TextureMipmap* _getMipmap(gal_uint mipLevel, const gal_char* methodStr) const;

    gal_uint _baseLevel;
    gal_uint _maxLevel;
    TextureMipmapChain _mips;
    std::set<gal_uint> _mappedMips;
    
    GAL_MEMORY_LAYOUT layout;
    
}; // class GALTexture2DImp


}

#endif // GAL_TEXTURE2D_IMP
