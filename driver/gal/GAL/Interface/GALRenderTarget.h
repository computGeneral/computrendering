/**************************************************************************
 *
 */

#ifndef GAL_RENDERTARGET
    #define GAL_RENDERTARGET

#include "GALTypes.h"

namespace libGAL
{
/**
 * Render target interface is an abstraction of a generic render target subresource
 *
 *
 * @author Carlos González Rodríguez (cgonzale@ac.upc.edu)
 * @date 02/12/2007
 */
class GALRenderTarget
{

public:

    /**
     *
     *  Get the width in pixels of the render target.
     *
     *  @return The width in pixels of the render target.
     */
     
    virtual gal_uint getWidth() const = 0;

    /**
     *
     *  Get the height in pixels of the render target.
     *
     *  @return The height in pixels of the render target.
     */

    virtual gal_uint getHeight() const = 0;
    
    /** 
     *
     *  Get the format of the render target.
     *
     *  @return The format of the render target.
     *
     */
     
    virtual GAL_FORMAT getFormat() const = 0;
    
    /**
     *
     *  Get if the render target supports multisampling.
     *
     *  @return If the render target supports multisampling.
     *
     */
     
    virtual gal_bool isMultisampled() const = 0;
    
    /** 
     *
     *  Get the number of samples supported per pixel for the render target.
     *
     *  @return The number of samples supporter per pixel for the render target.
     *
     */
     
    virtual gal_uint getSamples() const = 0;
    
    /**
     *
     *  Get if the render targets allows compression.
     *
     *  @return If the render target allows compression.
     *
     */
     
    virtual gal_bool allowsCompression() const = 0;

    /**
     *
     *  Get the texture that holds the renderTarget
     *
     *  @return The thexture that holds the renderTarget
     *
     */
    virtual GALTexture* getTexture() = 0;

};

}

#endif // GAL_RENDERTARGET
