/**************************************************************************
 *
 */

#ifndef GAL_TEXTURECUBEMAP_IMP
    #define GAL_TEXTURECUBEMAP_IMP

#include "GALTextureCubeMap.h"
#include "TextureMipmapChain.h"
#include "MemoryObject.h"
#include <set>

namespace libGAL
{

class GALTextureCubeMapImp : public GALTextureCubeMap, public MemoryObject
{
public:

    GALTextureCubeMapImp();

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

    /// Methods inherited from GALTextureCubeMap interface

    virtual gal_uint getWidth(GAL_CUBEMAP_FACE face, gal_uint mipmap) const;

    virtual gal_uint getHeight(GAL_CUBEMAP_FACE face, gal_uint mipmap) const;

    virtual GAL_FORMAT getFormat(GAL_CUBEMAP_FACE face, gal_uint mipmap) const;

    virtual gal_bool isMultisampled(GAL_CUBEMAP_FACE face, gal_uint mipLevel) const;

    virtual gal_uint getTexelSize(GAL_CUBEMAP_FACE face, gal_uint mipmap) const;

    virtual void setData( GAL_CUBEMAP_FACE face,
                          gal_uint mipLevel,
                          gal_uint width,
                          gal_uint height,
                          GAL_FORMAT format,
                          gal_uint rowPitch,
                          const gal_ubyte* srcTexelData,
                          gal_uint texSize,
                          gal_bool preloadData = false);

    virtual void updateData( GAL_CUBEMAP_FACE face,
                             gal_uint mipLevel,
                             gal_uint x,
                             gal_uint y,
                             gal_uint width,
                             gal_uint height,
                             GAL_FORMAT format,
                             gal_uint rowPitch,
                             const gal_ubyte* srcTexelData,
                             gal_bool preloadData = false);

    virtual gal_bool map( GAL_CUBEMAP_FACE face,
                          gal_uint mipLevel,
                          GAL_MAP mapType,
                          gal_ubyte*& pData,
                          gal_uint& dataRowPitch);

    virtual gal_bool unmap(GAL_CUBEMAP_FACE face, gal_uint mipLevel, gal_bool preloadData = false);

    /// Method required by MemoryObject derived classes
    virtual const gal_ubyte* memoryData(gal_uint region, gal_uint& memorySizeInBytes) const;

    const gal_char* stringType() const;


    /// Extended interface GALTextureCubeMapImp

    // Obtains the corresponding memoy region for a given face/mipmap pair
    static gal_uint translate2region(GAL_CUBEMAP_FACE face, gal_uint mipLevel);

    // Obtains the corresponding face/mipmap pair for a given memory region
    static void translate2faceMipmap(gal_uint mipRegion, GAL_CUBEMAP_FACE& face, gal_uint& mipLevel);

    void dumpMipmap(GAL_CUBEMAP_FACE face, gal_uint region, gal_ubyte* mipName);

    const gal_ubyte* getData(GAL_CUBEMAP_FACE face, gal_uint mipLevel, gal_uint& memorySizeInBytes, gal_uint& rowPitch) const;

private:

    const TextureMipmap* _getMipmap(GAL_CUBEMAP_FACE, gal_uint mipLevel, const gal_char* methodStr) const;

    gal_uint _baseLevel;
    gal_uint _maxLevel;
    std::set<gal_uint> _mappedMips[6]; // Mapped mipmaps
    TextureMipmapChain _mips[6]; // Define 6 Mipmap chains

    GAL_MEMORY_LAYOUT layout;
};

}

#endif // GAL_TEXTURECUBEMAP_IMP
