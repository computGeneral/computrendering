/**************************************************************************
 *
 */

#include "TextureAdapter.h"
#include "support.h"

using namespace std;
using namespace libGAL;

TextureAdapter::TextureAdapter(GALTexture* tex)
{
    if (tex == 0) return;

    _type = tex->getType();

    switch (_type) 
    {
        case GAL_RESOURCE_TEXTURE2D:
            _tex2D = static_cast<GALTexture2DImp*>(tex); 
            break;
        case GAL_RESOURCE_TEXTURECUBEMAP:
            _texCM = static_cast<GALTextureCubeMapImp*>(tex);
            break;

        case GAL_RESOURCE_TEXTURE3D:
            _tex3D = static_cast<GALTexture3DImp*>(tex);
            break;

        //default:
            //CG_ASSERT("Adapter only supports 2D and CM textures for now");
    }
}

GALTexture* TextureAdapter::getTexture()
{
    switch (_type) 
    {
        case GAL_RESOURCE_TEXTURE2D:
            return static_cast<GALTexture*>(_tex2D); 
            break;
        case GAL_RESOURCE_TEXTURECUBEMAP:
            return static_cast<GALTexture*>(_texCM);
            break;
        case GAL_RESOURCE_TEXTURE3D:
            return static_cast<GALTexture*>(_tex3D);
            break;
        default:
            CG_ASSERT("Adapter only supports 2D and CM textures for now");
            return 0;
            break;
    }
}

const gal_ubyte* TextureAdapter::getData(GAL_CUBEMAP_FACE face, gal_uint mipmap, gal_uint& memorySizeInBytes, gal_uint& rowPitch) const
{
    gal_uint rowPlane;

    switch (_type) 
    {
        case GAL_RESOURCE_TEXTURE2D:
            return _tex2D->getData(mipmap, memorySizeInBytes, rowPitch);
            break;
        case GAL_RESOURCE_TEXTURECUBEMAP:
            return _texCM->getData(face, mipmap, memorySizeInBytes, rowPitch);
            break;
        case GAL_RESOURCE_TEXTURE3D:
            return _tex3D->getData(mipmap, memorySizeInBytes, rowPitch, rowPlane);
            break;
        default:
            CG_ASSERT("Adapter only supports 2D and CM textures for now");
            return 0;
            break;
    }
}

const gal_ubyte* TextureAdapter::getMemoryData(GAL_CUBEMAP_FACE face, gal_uint mipmap, gal_uint& memorySizeInBytes) const
{
    switch (_type) 
    {
        case GAL_RESOURCE_TEXTURE2D:
            return _tex2D->memoryData(mipmap, memorySizeInBytes);
            break;
        case GAL_RESOURCE_TEXTURECUBEMAP:
            //return _texCM->memoryData(face, mipmap, memorySizeInBytes);
            CG_ASSERT("CubeMap texture not supported");
            return 0;
        case GAL_RESOURCE_TEXTURE3D:
            return _tex3D->memoryData(mipmap, memorySizeInBytes);
            break;
        default:
            CG_ASSERT("Adapter only supports 2D and CM textures for now");
            return 0;
    }
}

gal_uint TextureAdapter::getWidth(GAL_CUBEMAP_FACE face, gal_uint mipmap) const
{
    switch (_type) 
    {
        case GAL_RESOURCE_TEXTURE2D:
            return _tex2D->getWidth(mipmap);
            break;
        case GAL_RESOURCE_TEXTURECUBEMAP:
            return _texCM->getWidth(face, mipmap);
            break;
        case GAL_RESOURCE_TEXTURE3D:
            return _tex3D->getWidth(mipmap);
            break;
        default:
            CG_ASSERT("Adapter only supports 2D and CM textures for now");
            return 0;
    }
}

gal_uint TextureAdapter::getHeight(GAL_CUBEMAP_FACE face, gal_uint mipmap) const
{
    switch (_type) 
    {
        case GAL_RESOURCE_TEXTURE2D:
            return _tex2D->getHeight(mipmap);
            break;
        case GAL_RESOURCE_TEXTURECUBEMAP:
            return _texCM->getHeight(face, mipmap);
            break;
        case GAL_RESOURCE_TEXTURE3D:
            return _tex3D->getHeight(mipmap);
            break;
        default:
            CG_ASSERT("Adapter only supports 2D and CM textures for now");
            return 0;
    }
}

gal_uint TextureAdapter::getDepth(GAL_CUBEMAP_FACE face, gal_uint mipmap) const
{
    switch (_type) 
    {
        case GAL_RESOURCE_TEXTURE2D:
            return 0;
            break;
        case GAL_RESOURCE_TEXTURECUBEMAP:
            return 0;
            break;
        case GAL_RESOURCE_TEXTURE3D:
            return _tex3D->getDepth(mipmap);
            break;
        default:
            CG_ASSERT("Adapter only supports 2D and CM textures for now");
            return 0;
    }
}

gal_uint TextureAdapter::getTexelSize(GAL_CUBEMAP_FACE face, gal_uint mipmap) const
{
    switch (_type) 
    {
        case GAL_RESOURCE_TEXTURE2D:
            return _tex2D->getTexelSize(mipmap);
            break;
        case GAL_RESOURCE_TEXTURECUBEMAP:
            return _texCM->getTexelSize(face, mipmap);
            break;
        case GAL_RESOURCE_TEXTURE3D:
            return _tex3D->getTexelSize(mipmap);
            break;
        default:
            CG_ASSERT("Adapter only supports 2D and CM textures for now");
            return 0;
    }
}

GAL_FORMAT TextureAdapter::getFormat(GAL_CUBEMAP_FACE face, gal_uint mipmap) const
{
    switch (_type) 
    {
        case GAL_RESOURCE_TEXTURE2D:
            return _tex2D->getFormat(mipmap);
            break;
        case GAL_RESOURCE_TEXTURECUBEMAP:
            return _texCM->getFormat(face, mipmap);
            break;
        case GAL_RESOURCE_TEXTURE3D:
            return _tex3D->getFormat(mipmap);
            break;
        default:
            CG_ASSERT("Adapter only supports 2D and CM textures for now");
            return GAL_FORMAT_UNKNOWN;
    }
}


gal_bool TextureAdapter::getMultisampled(GAL_CUBEMAP_FACE face, gal_uint mipmap) const
{
    switch (_type) 
    {
        case GAL_RESOURCE_TEXTURE2D:
            return _tex2D->isMultisampled(mipmap);
            break;
        case GAL_RESOURCE_TEXTURECUBEMAP:
            //return _texCM->isMultisampled(face, mipmap);
            break;
        case GAL_RESOURCE_TEXTURE3D:
            return _tex3D->isMultisampled(mipmap);
            break;
        default:
            CG_ASSERT("Adapter only supports 2D and CM textures for now");
            return false;
    }
}

gal_uint TextureAdapter::getSamples(GAL_CUBEMAP_FACE face, gal_uint mipmap) const
{
    switch (_type) 
    {
        case GAL_RESOURCE_TEXTURE2D:
            return _tex2D->getSamples(mipmap);
            break;
        case GAL_RESOURCE_TEXTURECUBEMAP:
            //return _texCM->getSamples(face, mipmap);
            break;
        case GAL_RESOURCE_TEXTURE3D:
            return _tex3D->getSamples(mipmap);
            break;
        default:
            CG_ASSERT("Adapter only supports 2D and CM textures for now");
            return 0;
    }
}

MemoryObjectState TextureAdapter::getState(GAL_CUBEMAP_FACE face, gal_uint mipmap) const
{
    switch (_type) 
    {
        case GAL_RESOURCE_TEXTURE2D:
            return _tex2D->getState(mipmap);
            break;
        case GAL_RESOURCE_TEXTURECUBEMAP:
            //return _texCM->getState(face, mipmap);
            break;
        case GAL_RESOURCE_TEXTURE3D:
            return _tex3D->getState(mipmap);
            break;
        default:
            CG_ASSERT("Adapter only supports 2D and CM textures for now");
            return MOS_NotFound;
    }
}

gal_bool TextureAdapter::getMultisampling(GAL_CUBEMAP_FACE face, gal_uint mipmap) const
{
    switch (_type) 
    {
        case GAL_RESOURCE_TEXTURE2D:
            return _tex2D->isMultisampled(mipmap);
            break;
        case GAL_RESOURCE_TEXTURECUBEMAP:
            return _texCM->isMultisampled(face, mipmap);
            break;
        case GAL_RESOURCE_TEXTURE3D:
            return _tex3D->isMultisampled(mipmap);
            break;
        default:
            CG_ASSERT("Adapter only supports 2D and CM textures for now");
            return false;
    }
}

gal_uint TextureAdapter::region(GAL_CUBEMAP_FACE face, gal_uint mipmap) const
{
    switch (_type) 
    {
        case GAL_RESOURCE_TEXTURE2D:
            return mipmap;
            break;
        case GAL_RESOURCE_TEXTURECUBEMAP:
            return GALTextureCubeMapImp::translate2region(face, mipmap);
            break;
        case GAL_RESOURCE_TEXTURE3D:
            return mipmap;
            break;
        default:
            CG_ASSERT("Adapter only supports 2D and CM textures for now");
            return 0;
    }
}

void TextureAdapter::updateMipmap(  GAL_CUBEMAP_FACE face, 
                                    gal_uint mipLevel, 
                                    gal_uint x, 
                                    gal_uint y, 
                                    gal_uint z, 
                                    gal_uint width, 
                                    gal_uint height,
                                    gal_uint depth,
                                    GAL_FORMAT format, 
                                    gal_uint rowPitch, 
                                    const gal_ubyte* srcTexelData)
{
    switch (_type) 
    {
        case GAL_RESOURCE_TEXTURE2D:
            _tex2D->updateData(mipLevel, x, y, width, height, format, rowPitch, srcTexelData);
            break;
        case GAL_RESOURCE_TEXTURECUBEMAP:
            _texCM->updateData(face, mipLevel, x, y, width, height, format, rowPitch, srcTexelData);
            break;
        case GAL_RESOURCE_TEXTURE3D:
            _tex3D->updateData(mipLevel, x, y, z, width, height, depth, format, rowPitch, srcTexelData);
            break;
        default:
            CG_ASSERT("Adapter only supports 2D and CM textures for now");
            break;
    }
}

void TextureAdapter::setMemoryLayout(GAL_MEMORY_LAYOUT layout)
{
    switch (_type) 
    {
        case GAL_RESOURCE_TEXTURE2D:
            _tex2D->setMemoryLayout(layout);
            break;
        case GAL_RESOURCE_TEXTURECUBEMAP:
            _texCM->setMemoryLayout(layout);
            break;
        case GAL_RESOURCE_TEXTURE3D:
            _tex3D->setMemoryLayout(layout);
            break;
        default:
            CG_ASSERT("Adapter only supports 2D and CM textures for now");
            break;
    }
}


void TextureAdapter::postRenderBuffer(GAL_CUBEMAP_FACE face, gal_uint mipLevel)
{
    switch (_type) 
    {
        case GAL_RESOURCE_TEXTURE2D:
            _tex2D->postRenderBuffer(mipLevel);
            break;
        case GAL_RESOURCE_TEXTURECUBEMAP:
            _texCM->postRenderBuffer(GALTextureCubeMapImp::translate2region(face, mipLevel));
            break;
        case GAL_RESOURCE_TEXTURE3D:
            _tex3D->postRenderBuffer(mipLevel);
            break;
        default:
            CG_ASSERT("Adapter only supports 2D and CM textures for now");
            break;
    }
}

