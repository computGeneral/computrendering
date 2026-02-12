/**************************************************************************
 *
 */

#include "GALRenderTargetImp.h"
#include "GALDeviceImp.h"
#include "GALTexture2DImp.h"
#include "GALSampler.h"

using namespace libGAL;

GALRenderTargetImp::GALRenderTargetImp(GALDeviceImp *device, GALTexture* resource, GAL_RT_DIMENSION dimension, GAL_CUBEMAP_FACE face, gal_uint mipLevel) :
    _device(device), _surface(resource), _dimension(dimension), _face(face), _mipLevel(mipLevel)
{

    //TextureAdapter _surface(resource);

    switch(dimension)
    {
        case GAL_RT_DIMENSION_TEXTURE2D:
        case GAL_RT_DIMENSION_TEXTURECM:
            {    
                _surface.setMemoryLayout(GAL_LAYOUT_RENDERBUFFER);
                
                MemoryObjectAllocator& moa = _device->allocator();
                
                //  Check if the texture was already defined as a render buffer and allocated.
                if (_surface.getState(face, _mipLevel) != MOS_RenderBuffer)
                {
                    //  Get the GPU driver from the GAL device.
                    HAL& driver = device->driver();
                    
                    arch::TextureFormat format;
                    
                    switch(_surface.getFormat(face, _mipLevel))
                    {
                        case GAL_FORMAT_XRGB_8888:
                        case GAL_FORMAT_ARGB_8888:
                            format = arch::GPU_RGBA8888;
                            break;
                            
                        case GAL_FORMAT_RG16F:
                            format = arch::GPU_RG16F;
                            break;
                            
                        case GAL_FORMAT_R32F:
                            format = arch::GPU_R32F;
                            break;
                            
                        case GAL_FORMAT_RGBA16F:
                            format = arch::GPU_RGBA16F;
                            break;

                        case GAL_FORMAT_ABGR_161616:
                            format = arch::GPU_RGBA16;
                            break;
                            
                        case GAL_FORMAT_S8D24:
                            format = arch::GPU_DEPTH_COMPONENT24;
                            break;
                            
                        default:
                            char error[128];
                            sprintf(error, "Format not supported for render buffers: %d", _surface.getFormat(face, _mipLevel));
                            CG_ASSERT(error);
                            break;
                    }


                    gal_uint textureMD, renderTargetMD;

                    if (moa.hasMD(static_cast<GALTexture2DImp*>(_surface.getTexture()), _mipLevel) || moa.hasMD(static_cast<GALTexture2DImp*>(_surface.getTexture()), _surface.region(_face, _mipLevel))) // Discover if the texture is sync with the GPU
                    {
                        //If the texture is not sync with the GPU we don't have to copy the information that saves the texture
                        // to the new renderTarget memory region.

                        renderTargetMD = driver.createRenderBuffer( _surface.getWidth(face, _mipLevel), 
                                                                    _surface.getHeight(face, _mipLevel), 
                                                                    _surface.getMultisampling(face, _mipLevel), 
                                                                    _surface.getSamples(face, _mipLevel),
                                                                    format);

                    }
                    else 
                    {
                        // Obtain the MD used by the texture mipmap we want to convert
                        if (dimension == GAL_RT_DIMENSION_TEXTURECM)
                            textureMD = moa.md(static_cast<GALTextureCubeMapImp*>(_surface.getTexture()), _surface.region(_face, _mipLevel));
                        else
                            textureMD = moa.md(static_cast<GALTexture2DImp*>(_surface.getTexture()), _mipLevel);
                        
                        // Obtain the MD that will hold the renderTarget
                        renderTargetMD = driver.createRenderBuffer( _surface.getWidth(face, _mipLevel), 
                                                                    _surface.getHeight(face, _mipLevel), 
                                                                    _surface.getMultisampling(face, _mipLevel), 
                                                                    _surface.getSamples(face, _mipLevel),
                                                                    format);

                        GALRenderTargetImp* auxRT = new GALRenderTargetImp(_device,
                                                                           _surface.getWidth(face, _mipLevel), 
                                                                           _surface.getHeight(face, _mipLevel), 
                                                                           _surface.getMultisampling(face, _mipLevel), 
                                                                           _surface.getSamples(face, _mipLevel),
                                                                           _surface.getFormat(face, _mipLevel),
                                                                            false,
                                                                            renderTargetMD);

                        // Transfer the actual texture content to the renderTarget
                        //_device->copyTexture2RenderTarget(static_cast<GALTexture2DImp*>(_surface.getTexture()), auxRT);

                        delete auxRT;

                        // Deallocate previous texture memory
                        moa.deallocate(static_cast<GALTexture2DImp*>(_surface.getTexture()));
                    }

                    _surface.postRenderBuffer(face, _mipLevel);

                    // Update the memory descriptor used by the renderTarget
                    if (dimension == GAL_RT_DIMENSION_TEXTURECM)
                        moa.assign(static_cast<GALTextureCubeMapImp*>(_surface.getTexture()), _surface.region(_face, _mipLevel), renderTargetMD);
                    else
                        moa.assign(static_cast<GALTexture2DImp*>(_surface.getTexture()), _mipLevel, renderTargetMD);

                    //  Redefine the texture mipmap as a render buffer.
                    _surface.postRenderBuffer(face, _mipLevel);
                }
            }               
            break;
        case GAL_RT_DIMENSION_UNKNOWN:
        case GAL_RT_DIMENSION_BUFFER:
        case GAL_RT_DIMENSION_TEXTURE1D:
        case GAL_RT_DIMENSION_TEXTURE2DMS:
        case GAL_RT_DIMENSION_TEXTURE3D:
            CG_ASSERT("Unimplement GAL_RT_DIMENSION");
            break;
    }

    //  Render targets created from normal resources don't support compression.
    _compression = false;
}

GALRenderTargetImp::GALRenderTargetImp(GALDeviceImp* device, gal_uint width, gal_uint height, gal_bool multisampling, gal_uint samples, GAL_FORMAT format, bool compression, gal_uint md) :
    _device(device), _compression(compression), _surface(static_cast<GALTexture2D*>(0))
{

    GALTexture2DImp* renderTarget = static_cast<GALTexture2DImp*>(_device->createTexture2D());

    _mipLevel = 0;
    renderTarget->setData(_mipLevel, width, height, format, 0, 0, 0);

    renderTarget->setMemoryLayout(GAL_LAYOUT_RENDERBUFFER);

    renderTarget->postRenderBuffer(_mipLevel);

    MemoryObjectAllocator& moa = _device->allocator();

    moa.assign(renderTarget, _mipLevel, md);

    TextureAdapter surface (renderTarget);
    _surface = surface;

    _dimension = GAL_RT_DIMENSION_TEXTURE2D;

    _face = static_cast<GAL_CUBEMAP_FACE>(0);


}

gal_uint GALRenderTargetImp::getWidth() const
{
    return _surface.getWidth(_face, _mipLevel);
}

gal_uint GALRenderTargetImp::getHeight() const
{

    return _surface.getHeight(_face, _mipLevel);
}

gal_bool GALRenderTargetImp::isMultisampled() const
{
    return _surface.getMultisampled(_face, _mipLevel);
}

gal_uint GALRenderTargetImp::getSamples() const
{
    return _surface.getSamples(_face, _mipLevel);
}

GAL_FORMAT GALRenderTargetImp::getFormat() const
{
    return _surface.getFormat(_face, _mipLevel);
}

gal_uint GALRenderTargetImp::md() 
{
    MemoryObjectAllocator& moa = _device->allocator();
    if (_dimension == GAL_RT_DIMENSION_TEXTURE2D)
        return moa.md(static_cast<GALTexture2DImp*>(_surface.getTexture()) , _mipLevel);
    else if (_dimension == GAL_RT_DIMENSION_TEXTURECM)
        return moa.md(static_cast<GALTextureCubeMapImp*>(_surface.getTexture()) , _surface.region( _face, _mipLevel));
    else
    {
        CG_ASSERT("Unexpected texture type.");
        return 0;
    }
}

gal_bool GALRenderTargetImp::allowsCompression() const
{
    // Only for renderTarget
    return _compression;
}

GALTexture* GALRenderTargetImp::getTexture()
{
    return _surface.getTexture();
}


