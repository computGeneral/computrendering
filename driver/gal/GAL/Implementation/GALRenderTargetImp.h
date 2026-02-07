/**************************************************************************
 *
 */

#ifndef GAL_RENDER_TARGET_IMP
    #define GAL_RENDER_TARGET_IMP

#include "GALDeviceImp.h"
#include "GALRenderTarget.h"
#include "GALResource.h"
#include "GALTexture.h"
#include "GALTextureCubeMap.h"
#include "TextureAdapter.h"

namespace libGAL
{

class GALRenderTargetImp : public GALRenderTarget
{
public:

    GALRenderTargetImp(GALDeviceImp* device, GALTexture* resource, GAL_RT_DIMENSION dimension, GAL_CUBEMAP_FACE face, gal_uint mipLevel);
    GALRenderTargetImp(GALDeviceImp* device, gal_uint width, gal_uint height, gal_bool multisampling, gal_uint samples, GAL_FORMAT format, bool compression, gal_uint md);

    virtual gal_uint getWidth() const;
    virtual gal_uint getHeight() const;
    virtual gal_bool isMultisampled() const;
    virtual gal_uint getSamples() const;
    virtual GAL_FORMAT getFormat() const;
    virtual gal_bool allowsCompression() const;
    virtual GALTexture* getTexture();

    gal_uint md();

private:
    
    GALDeviceImp* _device;
    GAL_RT_DIMENSION _dimension;
    TextureAdapter _surface;
    GAL_CUBEMAP_FACE _face;
    gal_uint _mipLevel;
    gal_bool _compression;
    
};

} 

#endif // GAL_RENDER_TARGET_IMP
