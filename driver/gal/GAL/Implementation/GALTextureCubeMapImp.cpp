/**************************************************************************
 *
 */

#include "GALTextureCubeMapImp.h"
#include "GALMacros.h"
#include "GALMath.h"
#include "support.h"
#include <iostream>
#include <sstream>


using namespace std;
using namespace libGAL;

GALTextureCubeMapImp::GALTextureCubeMapImp() : _baseLevel(0), _maxLevel(0), layout(GAL_LAYOUT_TEXTURE)
{

}

GAL_RESOURCE_TYPE GALTextureCubeMapImp::getType() const
{
    return GAL_RESOURCE_TEXTURECUBEMAP;
}

void GALTextureCubeMapImp::setUsage(GAL_USAGE usage)
{
    cout << "GALTexture2DImp::setUsage() -> Not implemented yet" << endl;
}

GAL_USAGE GALTextureCubeMapImp::getUsage() const
{
    cout << "GALTexture2DImp::getUsage() -> Not implemented yet" << endl;
    return GAL_USAGE_STATIC;
}

void GALTextureCubeMapImp::setMemoryLayout(GAL_MEMORY_LAYOUT _layout)
{
    layout = _layout;
}

GAL_MEMORY_LAYOUT GALTextureCubeMapImp::getMemoryLayout() const
{
    return layout;
}

void GALTextureCubeMapImp::setPriority(gal_uint prio)
{
    cout << "GALTextureCubeMapImp::setPriority() -> Not implemented yet" << endl;
}

gal_uint GALTextureCubeMapImp::getPriority() const
{
    cout << "GALTextureCubeMapImp::getPriority() -> Not implemented yet" << endl;
    return 0;
}

gal_bool GALTextureCubeMapImp::wellDefined() const
{
    /// Check texture completeness ///
    
    if ( _baseLevel > _maxLevel ) 
    {
        CG_ASSERT("Base mipmap level > Max mipmap level");
        return false;
    }

    /// Get BaseLevel mipmap of face 0
    const TextureMipmap* baseMip = _mips[0].find(_baseLevel);
    if ( baseMip == 0 ) 
    {
        stringstream ss;
        ss << "BaseLevel mipmap " << _baseLevel << " of face 0 is not defined";
        CG_ASSERT(ss.str().c_str());
        return false;
    }

    gal_uint wNext = baseMip->getWidth();
    gal_uint hNext = baseMip->getHeight();
    GAL_FORMAT format = baseMip->getFormat();

    // These three variables are defined just to be able to call getData()
    const gal_ubyte* dummyPData;
    gal_uint dummyRowPitch;
    gal_uint dummyPlanePitch;

    // Compare the BaseLevel mipmap of each cubemap face
    for ( gal_uint i = 0; i < 6; ++i ) 
    {
        const TextureMipmap* tm = _mips[i].find(_baseLevel);
        if ( tm == 0 ) 
        {
            stringstream ss;
            ss << "BaseLevel mipmap " << _baseLevel << " of face " << i << " is not defined";
            CG_ASSERT(ss.str().c_str());
            return false;
        }

        if ( wNext != tm->getWidth() )
            CG_ASSERT("BaseLevel mipmap of all faces have not the same width");

        if ( hNext != tm->getHeight() )
            CG_ASSERT("BaseLevel mipmap of all faces have not the same height");

        if ( format != tm->getFormat() )
            CG_ASSERT("BaseLevel mipmap of all faces have not the same format");

        if ( tm->getData(dummyPData, dummyRowPitch, dummyPlanePitch) == 0 ) 
        {
            stringstream ss;
            ss << "Baselevel mipmap " << _baseLevel << " has not defined data";
            CG_ASSERT(ss.str().c_str());
            return false;
        }

        if ( _mappedMips[i].count(_baseLevel) != 0 ) {
            stringstream ss;
            ss << "Baselevel mipmap " << _baseLevel << " is mapped";
            CG_ASSERT(ss.str().c_str());
            return false;
        }
    }

    /// Check mipmap chain size of each cubemap face
    vector<const TextureMipmap*> mips[6];
    for ( gal_uint i = 0; i < 6; ++i ) 
    {
        mips[i] = _mips[i].getMipmaps(_baseLevel, _maxLevel);

        if ( _maxLevel - _baseLevel + 1 != mips[i].size() )
            CG_ASSERT("Some required mipmaps are not defined");
    }

    for ( gal_uint i = 0; i < mips[0].size(); ++i ) 
    {
        for ( gal_uint face = 0; face < 6; ++face ) 
        {
            const TextureMipmap& mip = *(mips[face][i]); // create a reference alias
            if ( _mappedMips[face].count(i+_baseLevel) != 0 ) 
            {
                stringstream ss;
                ss << "Mipmap " << i + _baseLevel << " of face " << face << " is mapped";
                CG_ASSERT(ss.str().c_str());
                return false;
            }

            if ( mip.getData(dummyPData, dummyRowPitch, dummyPlanePitch) == 0 ) {
                stringstream ss;
                ss << "Mipmap " << i + _baseLevel << " has not defined data";
                CG_ASSERT(ss.str().c_str());
                return false;
            }

            // Check format
            if ( mip.getFormat() != format ) 
            {
                stringstream ss;
                ss << "Mipmap " << i << " format is different from base level format";
                CG_ASSERT(ss.str().c_str());
                return false;
            }

            // Check width & height
            if ( mip.getWidth() != wNext ) 
            {
                stringstream ss;
                ss << "Mipmap " << i << " width is " << mips[face][i]->getWidth() << " and " << wNext << " was expected";
                CG_ASSERT(ss.str().c_str());
                return false;
            }

            if ( mip.getHeight() != hNext ) 
            {
                stringstream ss;
                ss << "Mipmap " << i << " height is " << mips[face][i]->getHeight() << " and " << hNext << " was expected";
                CG_ASSERT(ss.str().c_str());
                return false;
            }
        }

        // compute next expected mipmap size
        hNext = libGAL::max(static_cast<gal_uint>(1), hNext/2);
        wNext = libGAL::max(static_cast<gal_uint>(1), wNext/2);
    }

    return true;
}

gal_uint GALTextureCubeMapImp::getWidth(GAL_CUBEMAP_FACE face, gal_uint mipLevel) const
{
    return _getMipmap(face, mipLevel, "getWidth")->getWidth();
}

gal_uint GALTextureCubeMapImp::getHeight(GAL_CUBEMAP_FACE face, gal_uint mipLevel) const
{
    return _getMipmap(face, mipLevel, "getHeight")->getHeight();
}

gal_uint GALTextureCubeMapImp::getTexelSize(GAL_CUBEMAP_FACE face, gal_uint mipLevel) const
{

    return _getMipmap(face, mipLevel, "getTexelSize")->getTexelSize();
}

GAL_FORMAT GALTextureCubeMapImp::getFormat(GAL_CUBEMAP_FACE face, gal_uint mipLevel) const
{
    return _getMipmap(face, mipLevel, "getFormat")->getFormat();
}

gal_bool GALTextureCubeMapImp::isMultisampled(GAL_CUBEMAP_FACE face, gal_uint mipLevel) const
{
    return _getMipmap(face, mipLevel, "isMultisampled")->isMultisampled();
}

const TextureMipmap* GALTextureCubeMapImp::_getMipmap(GAL_CUBEMAP_FACE face, gal_uint mipLevel, 
                                                      const gal_char* methodStr) const
{
    const TextureMipmap* mipmap = _mips[face].find(mipLevel);
    if ( mipmap == 0 ) {
        stringstream ss;
        ss << "Mipmap level " << mipLevel << " of face " << static_cast<gal_uint>(face) << " is not defined";
        panic("GALTextureCubeMapImp", methodStr, ss.str().c_str());
    }
    return mipmap;
}

const gal_ubyte* GALTextureCubeMapImp::memoryData(gal_uint region, gal_uint& memorySizeInBytes) const
{
    gal_uint mipLevel;
    GAL_CUBEMAP_FACE face;

    // Obtain the mipmap face & level identified by region
    translate2faceMipmap(region, face, mipLevel);

    const TextureMipmap* mip = _mips[face].find(mipLevel);
    if ( mip == 0 ) {
        stringstream ss;
        ss << "Mipmap " << region << " not found (not defined) - face=" << static_cast<gal_uint>(face)
           << " mipLevel = " << mipLevel << "\n";
        CG_ASSERT(ss.str().c_str());
    }

    return mip->getDataInMortonOrder(memorySizeInBytes);
}

const gal_char* GALTextureCubeMapImp::stringType() const
{
    return "TEXTURE_CUBEMAP_OBJECT";
}

gal_uint GALTextureCubeMapImp::getSubresources() const
{
    GAL_ASSERT(
        if ( _baseLevel > _maxLevel )
            CG_ASSERT("Base mipmap level > Max mipmap level");
    )
    return 6*(_maxLevel - _baseLevel + 1);
}

void GALTextureCubeMapImp::setData( GAL_CUBEMAP_FACE face,
                                    gal_uint mipLevel,
                                    gal_uint width,
                                    gal_uint height,
                                    GAL_FORMAT format,
                                    gal_uint rowPitch,
                                    const gal_ubyte* srcTexelData,
                                    gal_uint texSize,
                                    gal_bool preloadData)
{
    GAL_ASSERT(
        if ( face > GAL_CUBEMAP_NEGATIVE_Z )
            CG_ASSERT("Invalid face");
    )

    if ( _mappedMips[face].count(mipLevel) != 0 )
        CG_ASSERT("Cannot call setData on a mapped mipmap");

    //  Adjust the base (minimum) mip level as new mip levels are added to the texture.
    if (_baseLevel > mipLevel)
        _baseLevel = mipLevel;

    //  Adjust the top (maximum) mip level as new mip levels are added to the texture.
    if (mipLevel > _maxLevel)
        _maxLevel = mipLevel;

    // Create or redefine the mipmap level
    TextureMipmap* mip = _mips[face].create(mipLevel);
    
    // Set texel data into the mipmap
    mip->setData(width, height, 1, format, rowPitch, srcTexelData, texSize);

    // Convert the mipmap/face identifier to a unique region identifier 
    gal_uint mipRegion = translate2region(face, mipLevel);

    // Add the mipmap to the tracking mechanism supported by all GAL Resources
    defineRegion(mipRegion);

    // New contents defined, texture must be reallocated before being used by the GPU
    postReallocate(mipRegion);

    preload(mipRegion, preloadData);
}

void GALTextureCubeMapImp::updateData( GAL_CUBEMAP_FACE face,
                                       gal_uint mipLevel,
                                       gal_uint x,
                                       gal_uint y,
                                       gal_uint width,
                                       gal_uint height,
                                       GAL_FORMAT format,
                                       gal_uint rowPitch,
                                       const gal_ubyte* srcTexelData,
                                       gal_bool preloadData)
{
    GAL_ASSERT(
        if ( face > GAL_CUBEMAP_NEGATIVE_Z )
            CG_ASSERT("Invalid face");
    )

    if ( _mappedMips[face].count(mipLevel) != 0 )
        CG_ASSERT("Cannot call updateData on a mapped mipmap");

    // Create or redefine the mipmap level
    //TextureMipmap* mip = _mips[face].create(mipLevel);
    TextureMipmap* mip = _mips[face].find(mipLevel);

    GAL_ASSERT(
        if ( mip == 0 )
            CG_ASSERT("Trying to update an undefined mipmap");
    )

    // Update texel data into the mipmap
    mip->updateData(x, y, 0, width, height, 1, format, rowPitch, srcTexelData);

    // Convert the mipmap/face identifier to a unique region identifier 
    gal_uint mipRegion = translate2region(face, mipLevel);

    // Mark all the texture mipmap as pendent to be updated
    postUpdate(mipRegion, 0, mip->getTexelSize() * mip->getWidth() * mip->getHeight() - 1);

    preload(mipRegion, preloadData);
}

gal_bool GALTextureCubeMapImp::map( GAL_CUBEMAP_FACE face,
                                    gal_uint mipLevel,
                                    GAL_MAP mapType,
                                    gal_ubyte*& pData,
                                    gal_uint& dataRowPitch )
{
    GAL_ASSERT(
        if ( face > GAL_CUBEMAP_NEGATIVE_Z )
            CG_ASSERT("Invalid face");
        if ( _mappedMips[face].count(mipLevel) != 0 )
            CG_ASSERT("Cannot map an already mapped mipmap");
    )

    TextureMipmap* mip = _mips[face].find(mipLevel);

    if ( mip == 0 ) {
        stringstream ss;
        ss << "Mipmap " << mipLevel << " not defined";
        CG_ASSERT(ss.str().c_str());
    }

    gal_uint mipRegion = translate2region(face, mipLevel);

    if ( getState(mipRegion) == MOS_Blit && (mapType == GAL_MAP_READ || GAL_MAP_READ_WRITE) )
        CG_ASSERT("Map for reading not supported with memory object region in BLIT state");

    // Mark this mipmap as mapped
    _mappedMips[face].insert(mipLevel);

    gal_uint dataPlanePitch;

    return mip->getData(pData, dataRowPitch, dataPlanePitch);
}

gal_bool GALTextureCubeMapImp::unmap(GAL_CUBEMAP_FACE face, gal_uint mipLevel, gal_bool preloadData)
{
    GAL_ASSERT(
        if ( face > GAL_CUBEMAP_NEGATIVE_Z )
            CG_ASSERT("Invalid face");
        if ( _mappedMips[face].count(mipLevel) != 0 )
            CG_ASSERT("Unmapping a not mapped texture mipmap");
    )

    // remove the mipmap level from the list of mapped mipmaps
    _mappedMips[face].erase(mipLevel);

    gal_uint mipRegion = translate2region(face, mipLevel);

    // Update all (conservative)
    postUpdate(mipRegion, 0, getTexelSize(face, mipLevel) * getWidth(face, mipLevel) * getHeight(face, mipLevel) - 1);
    
    preload(mipRegion, preloadData);
    
    return true;
}


gal_uint GALTextureCubeMapImp::getBaseLevel() const { return _baseLevel; }

gal_uint GALTextureCubeMapImp::getMaxLevel() const { return _maxLevel; }

void GALTextureCubeMapImp::setBaseLevel(gal_uint minMipLevel) 
{ 
    // Clamp if required
    _baseLevel = ( minMipLevel > GAL_MAX_TEXTURE_LEVEL ? GAL_MAX_TEXTURE_LEVEL : minMipLevel );
}

void GALTextureCubeMapImp::setMaxLevel(gal_uint maxMipLevel) 
{
    // Clamp if required
    _maxLevel = ( maxMipLevel > GAL_MAX_TEXTURE_LEVEL ? GAL_MAX_TEXTURE_LEVEL : maxMipLevel );
}

gal_uint GALTextureCubeMapImp::getSettedMipmaps()
{
    return _mips[0].size();
}

gal_uint GALTextureCubeMapImp::translate2region(GAL_CUBEMAP_FACE face, gal_uint mipLevel)
{
    return mipLevel + face * (GAL_MAX_TEXTURE_LEVEL + 1);
}

void GALTextureCubeMapImp::translate2faceMipmap(gal_uint mipRegion, GAL_CUBEMAP_FACE& face, gal_uint& mipLevel)
{
    gal_uint faceID = mipRegion / (GAL_MAX_TEXTURE_LEVEL + 1);
    
    GAL_ASSERT(
        if ( faceID > GAL_CUBEMAP_NEGATIVE_Z )
            CG_ASSERT("mipRegion face out of bounds");
    )

    face = static_cast<GAL_CUBEMAP_FACE>(faceID);
    mipLevel = mipRegion % (GAL_MAX_TEXTURE_LEVEL + 1);
}

void GALTextureCubeMapImp::dumpMipmap(GAL_CUBEMAP_FACE face, gal_uint region, gal_ubyte* mipName)
{
    TextureMipmap* mip = _mips[face].find(region);

    if ( mip == 0 ) {
        stringstream ss;
        ss << "Mipmap " << region << " not found (not defined)";
        CG_ASSERT(ss.str().c_str());
    }

    mip->dump2PPM(mipName);

}

const gal_ubyte* GALTextureCubeMapImp::getData(GAL_CUBEMAP_FACE face, gal_uint mipLevel, gal_uint& memorySizeInBytes, gal_uint& rowPitch) const
{
    gal_ubyte* data;
    gal_uint planePitch;
    TextureMipmap* mip = (TextureMipmap*)(_mips[face].find(mipLevel));
    mip->getData(data, rowPitch, planePitch);

    memorySizeInBytes = mip->getTexelSize() * mip->getHeight() * mip->getWidth();

    return data;
    
}
