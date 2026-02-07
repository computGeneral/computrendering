/**************************************************************************
 *
 */

#ifndef GAL_TEXTURE3D
    #define GAL_TEXTURE3D

#include "GALTexture.h"

namespace libGAL
{

/**
 * This interface represents 3D dimensional texture (interface not published yet)
 */
class GALTexture3D : public GALTexture
{
public:

    /**
     * Returns the width in texels of the most detailed mipmap level texture
     *
     * @returns the texture width in texels 
     */
    virtual gal_uint getWidth(gal_uint mipmap) const = 0;

    /**
     * Returns the height in texels of the most detailed mipmap level of the texture
     *
     * @returns the texture height in texels
     *
     * @note Always 0 for 1D textures
     */
    virtual gal_uint getHeight(gal_uint mipmap) const = 0;

    /**
     * Returns the depth in texels of the most detailed mipmap level of the texture
     *
     * @returns the texture depth in texels
     *
     * @note Always 0 for 1D textures
     */
    virtual gal_uint getDepth(gal_uint mipmap) const = 0;

    /**
     * Gets the texture format
     *
     * @returns The texture format
     */
    virtual GAL_FORMAT getFormat(gal_uint mipmap) const = 0;

    /**
     *
     *  Get if the texture is multisampled.
     *
     *  @param mipmap Mipmap of the texture for which information is requested.
     * 
     *  @return If the texture is multisampled.
     *
     */
     
    virtual gal_bool isMultisampled(gal_uint mipmap) const = 0;
    
    /**
     *
     *  Get the number of samples per pixel defined for the texture.
     *
     *  @param mipmap Mipmap of the texture for which information is requested.
     *
     *  @return The samples per pixel defiend for the texture.
     *
     */     
     
    virtual gal_uint getSamples(gal_uint mipmap) const = 0;     
     
    /**
     * Returns the size of the texels in the texture (in bytes)
     *
     * @returns the size of one texel of the texture
     */
    virtual gal_uint getTexelSize(gal_uint mipmap) const = 0;

    /**
     * Sets/resets data into a texture mipmap
     *
     * @note It is assumed that source texel data contains 'height' rows of size width * getTexelSize()
     *       each initial texel of each two consecutive rows is separated by rowPitch bytes
     */
    virtual void setData( gal_uint mipLevel,
                          gal_uint width,
                          gal_uint height,
                          gal_uint depth,
                          GAL_FORMAT format,
                          gal_uint rowPitch,
                          const gal_ubyte* srcTexelData,
                          gal_uint texSize,
                          gal_bool preloadData = false) = 0;

    /**
     *
     *  Defines a texture mipmap.
     *
     *  @param mipLevel Mipmap level in the texture to be defined.
     *  @param width Width in pixels of the mipmap.
     *  @param height Height in pixels of the mipmap.
     *  @param multisampling Defines if the 
     */
    virtual void setData(gal_uint mipLevel,
                         gal_uint width,
                         gal_uint height,
                         gal_uint depth,
                         gal_bool multisampling,
                         gal_uint samples,
                         GAL_FORMAT format) = 0;
    /**
     * Update texture contents of a subresource region
     *
     * @param mipLevel Mipmap level to update
     * @param x left coordinate of the 2D rectangle to update
     * @param y top coordinate of the 2D rectangle to update
     * @param width Width of the 2D rectagle to update
     * @param height Height of the 2D rectangle to update
     * @param rowPitch Total bytes of a row including the memory layout padding
     * @param srcTexelData Texel data to update the texture
     * @param preloadData Defines if the data is preloaded (zero simulation cost).
     *
     * @note the srcData is expected to contain height rows and width columns
     *       after each row an amount of bytes (rowPaddingBytes)
     *       must be skipped before access the next row
     *
     * @note Use update(miplevel,0,0,0,0,rowPaddingBytes,srcData) to update
     *       the whole mipmap level
     *        
     */
    virtual void updateData( gal_uint mipLevel,
                             gal_uint x,
                             gal_uint y,
                             gal_uint z,
                             gal_uint width,
                             gal_uint height,
                             gal_uint depth,
                             GAL_FORMAT format,
                             gal_uint rowPitch,
                             const gal_ubyte* srcTexelData,
                             gal_bool preloadData = false) = 0;

    /**
     * Allows to map a texture subresource to the user address space
     *
     * @note Only 
     *
     * @param mipLevel to map
     * @param mapType type of mapping requested
     * @retval pData pointer to the texture mipmap data
     * @retval dataRowPitch
     *
     * @code
     *
     *    // Reset a texture mipmap
     * 
     *    gal_uint rowPitch;
     *    gal_ubyte* pData;
     *
     *    t->map(0, GAL_MAP_WRITE_DISCARDPREV, pData, rowPitch);
     *         memZero(pData, t->getHeight() * rowPitch);
     *    t->unmap();
     *
     * @endcode
     */
    virtual gal_bool map( gal_uint mipLevel,
                      GAL_MAP mapType,
                      gal_ubyte*& pData,
                      gal_uint& dataRowPitch,
                      gal_uint& dataPlanePitch) = 0;

    /**
     * Unmaps a mipmap subresource from a 3D texture
     *
     * @param mipLevel the mipmap level to unmap 
     */
    virtual gal_bool unmap(gal_uint mipLevel, gal_bool preloadData = false) = 0; 
};

}

#endif // GAL_TEXTURE3D
