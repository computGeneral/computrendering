/**************************************************************************
 *
 */

#ifndef TEXTUREADAPTER
    #define TEXTUREADAPTER

#include "GALTexture2DImp.h"
#include "GALTextureCubeMapImp.h"
#include "GALTexture3DImp.h"
#include "TextureMipmap.h"
#include "GALResource.h"
#include "MemoryObject.h"

namespace libGAL
{

/*
 * Texture adapter class adapts all texture types defined in GAL to a common interface
 * This allows to perform several query schemes independently of the texture type
 *
 * @note It is mandatory that the texture is "well-defined"
 */
class TextureAdapter
{
public:
    
    /**
     * MANDATORY: The texture to be "adapted" must be in state "well-defined"
     */
    TextureAdapter(GALTexture* tex);


    GALTexture* getTexture();


    const gal_ubyte* getData(GAL_CUBEMAP_FACE face, gal_uint mipmap, gal_uint& memorySizeInBytes, gal_uint& rowPitch) const;


    const gal_ubyte* getMemoryData(GAL_CUBEMAP_FACE face, gal_uint mipmap, gal_uint& memorySizeInBytes) const;


    gal_uint getWidth(GAL_CUBEMAP_FACE face, gal_uint mipmap) const;


    gal_uint getHeight(GAL_CUBEMAP_FACE face, gal_uint mipmap) const;


    gal_uint getDepth(GAL_CUBEMAP_FACE face, gal_uint mipmap) const;


    gal_uint getTexelSize(GAL_CUBEMAP_FACE face, gal_uint mipmap) const;


    GAL_FORMAT getFormat(GAL_CUBEMAP_FACE face, gal_uint mipmap) const;


    gal_bool getMultisampled(GAL_CUBEMAP_FACE face, gal_uint mipmap) const;


    gal_uint getSamples(GAL_CUBEMAP_FACE face, gal_uint mipmap) const;


    gal_uint region(GAL_CUBEMAP_FACE face, gal_uint mipmap) const;


    MemoryObjectState getState(GAL_CUBEMAP_FACE face, gal_uint mipmap) const;


    gal_bool getMultisampling(GAL_CUBEMAP_FACE face, gal_uint mipmap) const;

    void postRenderBuffer(GAL_CUBEMAP_FACE face, gal_uint mipLevel);


    void setMemoryLayout(GAL_MEMORY_LAYOUT layout);


    void updateMipmap(GAL_CUBEMAP_FACE face,
                             gal_uint mipLevel,
                             gal_uint x,
                             gal_uint y,
                             gal_uint z,
                             gal_uint width,
                             gal_uint height,
                             gal_uint depth,
                             GAL_FORMAT format,
                             gal_uint rowPitch,
                             const gal_ubyte* srcTexelData);

private:

    GAL_RESOURCE_TYPE _type;

    GALTexture2DImp* _tex2D;

    GALTexture3DImp* _tex3D;

    GALTextureCubeMapImp* _texCM;
};

}

#endif // TEXTUREADAPTER
