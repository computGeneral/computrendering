/**************************************************************************
 *
 */

#include <iostream>
#include <sstream>
#include <vector>

#include "GALTexture3DImp.h"
#include "support.h"
#include "GALMacros.h"
#include "GALMath.h"

#include "GlobalProfiler.h"

using namespace libGAL;

using std::vector;
using std::cout;
using std::endl;
using std::stringstream;

GALTexture3DImp::GALTexture3DImp() : 
    _baseLevel(0), _maxLevel(0), layout(GAL_LAYOUT_TEXTURE)
{
    _mips.destroyMipmaps();
}

GAL_RESOURCE_TYPE GALTexture3DImp::getType() const
{
    return GAL_RESOURCE_TEXTURE3D;
}

void GALTexture3DImp::setUsage(GAL_USAGE usage)
{
    cout << "GALTexture3DImp::setUsage() -> Not implemented yet" << endl;
}

GAL_USAGE GALTexture3DImp::getUsage() const
{
    cout << "GALTexture3DImp::getUsage() -> Not implemented yet" << endl;
    return GAL_USAGE_STATIC;
}

void GALTexture3DImp::setMemoryLayout(GAL_MEMORY_LAYOUT _layout)
{
    layout = _layout;
}

GAL_MEMORY_LAYOUT GALTexture3DImp::getMemoryLayout() const
{
    return layout;
}

void GALTexture3DImp::setPriority(gal_uint prio)
{
    cout << "GALTexture3DImp::setPriority() -> Not implemented yet" << endl;
}

gal_uint GALTexture3DImp::getPriority() const
{
    cout << "GALTexture3DImp::getPriority() -> Not implemented yet" << endl;
    return 0;
}

gal_uint GALTexture3DImp::getSubresources() const
{
    GAL_ASSERT(
        if ( _baseLevel > _maxLevel )
            CG_ASSERT("Base mipmap level > Max mipmap level");
    )
    return _maxLevel - _baseLevel + 1;
}

gal_bool GALTexture3DImp::wellDefined() const
{
    GLOBAL_PROFILER_ENTER_REGION("GALTexture3DImp", "", "")
    
    /// Check texture completeness ///
    
    if ( _baseLevel > _maxLevel ) {
        CG_ASSERT("Base mipmap level > Max mipmap level");
        return false;
    }

    gal_uint wNext, hNext, dNext;

    gal_uint baseLevel = _baseLevel;

    const TextureMipmap* baseMip = _mips.find(baseLevel);
    if ( baseMip == 0 ) 
    {
        stringstream ss;
        ss << "Mipmap " << baseLevel << " is not defined";
        CG_ASSERT(ss.str().c_str());
        return false;
    }
    
    vector<const TextureMipmap*> mips = _mips.getMipmaps(_baseLevel, _maxLevel);
   
    //Check if all required mipmaps are defined
    if ( _maxLevel - _baseLevel + 1 != mips.size() )
        CG_ASSERT("Some required mipmaps are not defined");

    wNext = baseMip->getWidth();
    hNext = baseMip->getHeight();
    dNext = baseMip->getDepth();
    GAL_FORMAT format = baseMip->getFormat();

    // These three variables are defined just to be able to call getData()
    const gal_ubyte* dummyPData;
    gal_uint dummyRowPitch;
    gal_uint dummyPlanePitch;

    if ( baseMip->getData(dummyPData, dummyRowPitch, dummyPlanePitch) == 0 ) 
    {
        stringstream ss;
        ss << "Baselevel mipmap " << baseLevel << " has not defined data";
        CG_ASSERT(ss.str().c_str());
        return false;
    }

    if ( _mappedMips.count(baseLevel) != 0 ) 
    {
        stringstream ss;
        ss << "Baselevel mipmap " << baseLevel << " is mapped";
        CG_ASSERT(ss.str().c_str());
        return false;
    }

    for ( gal_uint i = 0; i < mips.size(); ++i ) 
    {
        const TextureMipmap& mip = *mips[i]; // create a reference alias

       /* if ( _mappedMips.count(i+baseLevel) != 0 ) 
        {
            stringstream ss;
            ss << "Mipmap " << i + baseLevel << " is mapped";
            CG_ASSERT(ss.str().c_str());
            return false;
        }

        // Check data
        if ( mip.getData(dummyPData, dummyRowPitch, dummyPlanePitch) == 0 ) 
        {
            stringstream ss;
            ss << "Mipmap " << i + baseLevel << " has not defined data";
            CG_ASSERT(ss.str().c_str());
            return false;
        }*/

        // Check format
        /*if ( mip.getFormat() != format ) 
        {
            stringstream ss;
            ss << "Mipmap " << i << " format is different from base level format";
            CG_ASSERT(ss.str().c_str());
            return false;
        }*/

        // Check width & height & depth
        /*if ( mip.getWidth() != wNext ) 
        {
            stringstream ss;
            ss << "Mipmap " << i << " width is " << mips[i]->getWidth() << " and " << wNext << " was expected";
            CG_ASSERT(ss.str().c_str());
            return false;
        }

        if ( mip.getHeight() != hNext ) 
        {
            stringstream ss;
            ss << "Mipmap " << i << " height is " << mips[i]->getHeight() << " and " << hNext << " was expected";
            CG_ASSERT(ss.str().c_str());
            return false;
        }

        if ( mip.getDepth() != dNext ) 
        {
            stringstream ss;
            ss << "Mipmap " << i << " height is " << mips[i]->getDepth() << " and " << dNext << " was expected";
            CG_ASSERT(ss.str().c_str());
            return false;
        }
*/
        // compute next expected mipmap size
        hNext = libGAL::max(static_cast<gal_uint>(1), hNext/2);
        wNext = libGAL::max(static_cast<gal_uint>(1), wNext/2);
        dNext = libGAL::max(static_cast<gal_uint>(1), dNext/2);
    }

    GLOBAL_PROFILER_EXIT_REGION()
    return true;
}

gal_uint GALTexture3DImp::getWidth(gal_uint mipLevel) const 
{ 
    return _getMipmap(mipLevel, "getWidth")->getWidth();
}

gal_uint GALTexture3DImp::getHeight(gal_uint mipLevel) const 
{
    return _getMipmap(mipLevel, "getHeight")->getHeight();
}

gal_uint GALTexture3DImp::getDepth(gal_uint mipLevel) const
{
    return _getMipmap(mipLevel, "getDepth")->getDepth();
}

gal_uint GALTexture3DImp::getTexelSize(gal_uint mipLevel) const 
{
    return _getMipmap(mipLevel, "getTexelSize")->getTexelSize();
}

GAL_FORMAT GALTexture3DImp::getFormat(gal_uint mipLevel) const
{
    return _getMipmap(mipLevel, "getFormat")->getFormat();
}

gal_bool GALTexture3DImp::isMultisampled(gal_uint mipLevel) const
{
    return _getMipmap(mipLevel, "isMultisampled")->isMultisampled();
}

gal_uint GALTexture3DImp::getSamples(gal_uint mipLevel) const
{
    return _getMipmap(mipLevel, "getSamples")->getSamples();
}

const TextureMipmap* GALTexture3DImp::_getMipmap(gal_uint mipLevel, const gal_char* methodStr) const
{
    GLOBAL_PROFILER_ENTER_REGION("GALTexture3DImp", "", "")

    const TextureMipmap* mip = _mips.find(mipLevel);

    if ( mip == 0 ) 
    {
        stringstream ss;
        ss << "Mipmap " << mipLevel << " not found (not defined)";
        panic("GALTexture3DImp", methodStr, ss.str().c_str());
    }

    GLOBAL_PROFILER_EXIT_REGION()
    return mip;
}

const gal_ubyte* GALTexture3DImp::memoryData(gal_uint region, gal_uint& memorySizeInBytes) const
{
    const TextureMipmap* mip = _mips.find(region);

    if ( mip == 0 ) 
    {
        stringstream ss;
        ss << "Mipmap " << region << " not found (not defined)";
        CG_ASSERT(ss.str().c_str());
    }

    return mip->getDataInMortonOrder(memorySizeInBytes);
}

const gal_char* GALTexture3DImp::stringType() const
{
    return "TEXTURE_3D_OBJECT";
}

void GALTexture3DImp::dumpMipmap(gal_uint region, gal_ubyte* mipName)
{
    TextureMipmap* mip = _mips.find(region);

    if ( mip == 0 ) {
        stringstream ss;
        ss << "Mipmap " << region << " not found (not defined)";
        CG_ASSERT(ss.str().c_str());
    }
    
    mip->dump2PPM(mipName);

}

void GALTexture3DImp::setData( gal_uint mipLevel,
                               gal_uint width,
                               gal_uint height,
                               gal_uint depth,
                               GAL_FORMAT format,
                               gal_uint rowPitch,
                               const gal_ubyte* srcTexelData,
                               gal_uint texSize,
                               gal_bool preloadData)
{
    GLOBAL_PROFILER_ENTER_REGION("GALTexture3DImp", "", "")
    
    if ( _mappedMips.count(mipLevel) != 0 )
        CG_ASSERT("Cannot call setData on a mapped mipmap");

    //  Adjust the base (minimum) mip level as new mip levels are added to the texture.
    if (_baseLevel > mipLevel)
        _baseLevel = mipLevel;

    //  Adjust the top (maximum) mip level as new mip levels are added to the texture.
    if (mipLevel > _maxLevel)
        _maxLevel = mipLevel;

    // Create or redefine the mipmap level
    TextureMipmap* mip = _mips.create(mipLevel);
    
    // Set texel data into the mipmap
    mip->setData(width, height, depth, format, rowPitch, srcTexelData, texSize);

    // Add the mipmap to the tracking mechanism supported by all GAL Resources
    defineRegion(mipLevel);

    // New contents defined, texture must be reallocated before being used by the GPU
    postReallocate(mipLevel);

    GLOBAL_PROFILER_EXIT_REGION()
}

void GALTexture3DImp::setData ( gal_uint mipLevel, 
                                gal_uint width, 
                                gal_uint height,
                                gal_uint depth,
                                gal_bool multisampling, 
                                gal_uint samples, 
                                GAL_FORMAT format)
{
    GLOBAL_PROFILER_ENTER_REGION("GALTexture3DImp", "", "")

    if ( _mappedMips.count(mipLevel) != 0 )
        CG_ASSERT("Cannot call setData on a mapped mipmap");

    //  Adjust the base (minimum) mip level as new mip levels are added to the texture.
    if (_baseLevel > mipLevel)
        _baseLevel = mipLevel;

    //  Adjust the top (maximum) mip level as new mip levels are added to the texture.
    if (mipLevel > _maxLevel)
        _maxLevel = mipLevel;
    
    // Create or redefine the mipmap level
    TextureMipmap* mip = _mips.create(mipLevel);
    
    // Set texel data into the mipmap
    mip->setData(width, height, depth, multisampling, samples, format);

    // Add the mipmap to the tracking mechanism supported by all GAL Resources
    defineRegion(mipLevel);

    GLOBAL_PROFILER_EXIT_REGION()
}

void GALTexture3DImp::updateData(   gal_uint mipLevel,
                                    gal_uint x,
                                    gal_uint y,
                                    gal_uint z,
                                    gal_uint width,
                                    gal_uint height,
                                    gal_uint depth,
                                    GAL_FORMAT format,
                                    gal_uint rowPitch,
                                    const gal_ubyte* srcTexelData,
                                    gal_bool preloadData)
{
    GLOBAL_PROFILER_ENTER_REGION("GALTexture3DImp", "", "")

    if ( _mappedMips.count(mipLevel) != 0 )
        CG_ASSERT("Cannot call updateData on a mapped mipmap");
    
    // Create or redefine the mipmap level
    TextureMipmap* mip = _mips.find(mipLevel);

    GAL_ASSERT(
        if ( mip == 0 )
            CG_ASSERT("Trying to update an undefined mipmap");
    )

    // Update texel data into the mipmap
    mip->updateData(x, y, z, width, height, depth, format, rowPitch, srcTexelData);

    // Mark all the texture mipmap as pendent to be updated
    postUpdate(mipLevel, 0, mip->getTexelSize() * mip->getWidth() * mip->getHeight() - 1);

    preload(mipLevel, preloadData);

    GLOBAL_PROFILER_EXIT_REGION()
}

gal_bool GALTexture3DImp::map( gal_uint mipLevel,
                               GAL_MAP mapType,
                               gal_ubyte*& pData,
                               gal_uint& dataRowPitch,
                               gal_uint& dataPlanePitch)
{
    GLOBAL_PROFILER_ENTER_REGION("GALTexture3DImp", "", "")

    if ( _mappedMips.count(mipLevel) != 0 )
        CG_ASSERT("Cannot map an already mapped mipmap");

    TextureMipmap* mip = _mips.find(mipLevel);

    if ( mip == 0 ) {
        stringstream ss;
        ss << "Mipmap " << mipLevel << " not defined";
        CG_ASSERT(ss.str().c_str());
    }

    if ( getState(mipLevel) == MOS_Blit && (mapType == GAL_MAP_READ || GAL_MAP_READ_WRITE) )
        CG_ASSERT("Map for reading not supported with memory object region in BLIT state");
    
    // Mark this mipmap as mapped
    _mappedMips.insert(mipLevel);

    GLOBAL_PROFILER_EXIT_REGION()

    return mip->getData(pData, dataRowPitch, dataPlanePitch);
}

gal_bool GALTexture3DImp::unmap(gal_uint mipLevel, gal_bool preloadData)
{
    GLOBAL_PROFILER_ENTER_REGION("GALTexture3DImp", "", "")

    GAL_ASSERT
    (
        if ( _mappedMips.count(mipLevel) != 0 ) 
        {
            CG_ASSERT("Unmapping a not mapped texture mipmap");
        }
        GLOBAL_PROFILER_EXIT_REGION()
        return false;
    )
    
    // remove the mipmap level from the list of mapped mipmaps
    _mappedMips.erase(mipLevel);

    // Update all (conservative)
    postUpdate(mipLevel, 0, getTexelSize(mipLevel) * getWidth(mipLevel) * getHeight(mipLevel) * getDepth(mipLevel) - 1);

    preload(mipLevel, preloadData);

    GLOBAL_PROFILER_EXIT_REGION()
    
    return true;
}

gal_uint GALTexture3DImp::getBaseLevel() const { return _baseLevel; }

gal_uint GALTexture3DImp::getMaxLevel() const { return _maxLevel; }

void GALTexture3DImp::setBaseLevel(gal_uint minMipLevel) 
{ 
    // Clamp if required
    _baseLevel = ( minMipLevel > GAL_MAX_TEXTURE_LEVEL ? GAL_MAX_TEXTURE_LEVEL : minMipLevel );
}

void GALTexture3DImp::setMaxLevel(gal_uint maxMipLevel) 
{
    // Clamp if required
    _maxLevel = ( maxMipLevel > GAL_MAX_TEXTURE_LEVEL ? GAL_MAX_TEXTURE_LEVEL : maxMipLevel );
}

gal_uint GALTexture3DImp::getSettedMipmaps()
{
    return _mips.size();
}

const gal_ubyte* GALTexture3DImp::getData(gal_uint mipLevel, gal_uint& memorySizeInBytes, gal_uint& rowPitch, gal_uint& planePitch) const
{
    gal_ubyte* data;

    TextureMipmap* mip = (TextureMipmap*)(_mips.find(mipLevel));
    mip->getData(data, rowPitch, planePitch);

    memorySizeInBytes = mip->getTexelSize() * mip->getHeight() * mip->getWidth() * mip->getDepth();

    return data;
    
}

