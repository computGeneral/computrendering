/**************************************************************************
 *
 */

#ifndef GAL_SAMPLER
    #define GAL_SAMPLER

#include "GALTypes.h"
#include "GALTexture.h"
#include "GALTexture2D.h"

namespace libGAL
{


/**
 * Defines the texture address modes supported by the GAL
 */
enum GAL_TEXTURE_ADDR_MODE
{
    GAL_TEXTURE_ADDR_CLAMP,
    GAL_TEXTURE_ADDR_CLAMP_TO_EDGE,
    GAL_TEXTURE_ADDR_REPEAT,
    GAL_TEXTURE_ADDR_CLAMP_TO_BORDER,
    GAL_TEXTURE_ADDR_MIRRORED_REPEAT
};

/**
 * Texture coordinates tokens
 */
enum GAL_TEXTURE_COORD
{
    GAL_TEXTURE_S_COORD,
    GAL_TEXTURE_T_COORD,
    GAL_TEXTURE_R_COORD
};

/**
 * Native texture filters
 */
enum GAL_TEXTURE_FILTER
{
    GAL_TEXTURE_FILTER_NEAREST,
    GAL_TEXTURE_FILTER_LINEAR,
    GAL_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST,
    GAL_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR,
    GAL_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST,
    GAL_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR
};

/**
 *
 * Native comparison functions.
 *
 */
enum GAL_TEXTURE_COMPARISON
{
    GAL_TEXTURE_COMPARISON_NEVER,
    GAL_TEXTURE_COMPARISON_ALWAYS,
    GAL_TEXTURE_COMPARISON_LESS,
    GAL_TEXTURE_COMPARISON_LEQUAL,
    GAL_TEXTURE_COMPARISON_EQUAL,
    GAL_TEXTURE_COMPARISON_GEQUAL,
    GAL_TEXTURE_COMPARISON_GREATER,
    GAL_TEXTURE_COMPARISON_NOTEQUAL
};
 

/**
 * This interface represent an CG1 texture sampler
 *
 * @author Carlos Gonz�lez Rodr�guez (cgonzale@ac.upc.edu)
 * @version 1.0
 * @date 01/23/2007
 */
class GALSampler
{
public:

    /**
     * Enables or disables the sampler unit
     */
    virtual void setEnabled(gal_bool enable) = 0;

    /**
     * Checks if the texture sampler is enabled or disabled
     */
    virtual gal_bool isEnabled() const = 0;

    /**
     * Sets the texture used by this sampler
     *
     * @param texture The set texture
     */
    virtual void setTexture(GALTexture* texture) = 0;

    /**
     * Sets texture address mode
     */
    virtual void setTextureAddressMode(GAL_TEXTURE_COORD coord, GAL_TEXTURE_ADDR_MODE mode) = 0;

    /**
     *
     *  Set texture non-normalized coordinates mode.
     *
     *  @param enable Boolean value to enable or disable non-normalized texture coordinates.
     *
     */
     
    virtual void setNonNormalizedCoordinates(gal_bool enable) = 0;
         
    /**
     * Sets the minification filter
     *
     * @param minFilter the minification filter
     */
    virtual void setMinFilter(GAL_TEXTURE_FILTER minFilter) = 0;

    /**
     * Sets the magnification filter
     *
     * @param magFilter he magnification filter
     */
    virtual void setMagFilter(GAL_TEXTURE_FILTER magFilter) = 0;

    /**
     *
     *  Sets if comparison filter (PCF) is enabled for this sampler.
     *
     *  @param enable Boolean value that defines if the comparison filter is
     *  enabled for this sampler.
     *
     */
     
    virtual void setEnableComparison(gal_bool enable) = 0;
    
    /**
     *
     *  Sets the comparison function (PCF) for this sampler.
     *
     *  @param function The comparison function defined for the sampler.
     *
     */
     
    virtual void setComparisonFunction(GAL_TEXTURE_COMPARISON function) = 0;
     
    /**
     *
     *  Sets the conversion from sRGB space to linear space of the texture data for the sampler.
     *
     *  @param enable Boolean value that defines if the conversion is enabled for this sampler.
     *  
     */
     
    virtual void setSRGBConversion(gal_bool enable) = 0;    
     
    virtual void setMinLOD(gal_float minLOD) = 0;

    virtual void setMaxLOD(gal_float maxLOD) = 0;

    virtual void setMaxAnisotropy(gal_uint maxAnisotropy) = 0;

    virtual void setLODBias(gal_float lodBias) = 0;

    virtual void setUnitLODBias(gal_float unitLodBias) = 0;

    virtual void setMinLevel(gal_uint minLevel) = 0;
    
    /**
     * Gets the magnification filter
     *
     * @returns the magnification filter
     */
    virtual GAL_TEXTURE_FILTER getMagFilter() const = 0;
    
    /**
     * Gets the current texture of the sampler
     *
     * @returns the current texture attached to the sampler
     */
    virtual GALTexture* getTexture() const = 0;

    /**
     * Gets the minification filter
     *
     * @returns the minification filter value
     */
    virtual GAL_TEXTURE_FILTER getMinFilter() const = 0;

    /**
     * Performs the blito operation
     *
     * 
     */
    virtual void performBlitOperation2D(gal_uint xoffset, gal_uint yoffset, gal_uint x, gal_uint y, gal_uint width, gal_uint height, gal_uint textureWidth, GAL_FORMAT internalFormat, GALTexture2D* tex2D, gal_uint level) = 0;
};

}

#endif // GAL_SAMPLER
