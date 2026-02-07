/**************************************************************************
 *
 */

#ifndef GAL_TEXTURE
    #define GAL_TEXTURE

#include "GALTypes.h"
#include "GALResource.h"

namespace libGAL
{

static const gal_uint GAL_MIN_TEXTURE_LEVEL =  0;
static const gal_uint GAL_MAX_TEXTURE_LEVEL = 12;

/**
 * Base interface to handle CG1 textures
 *
 * @author Carlos Gonz�lez Rodr�guez (cgonzale@ac.upc.edu)
 * @version 1.0
 * @date 28/03/2007
 */
class GALTexture : public GALResource
{
public:

    virtual gal_uint getBaseLevel() const = 0;

    virtual gal_uint getMaxLevel() const = 0;

    /**
     * Sets the base level of detail of the texture
     *
     * By default = 0
     *
     * Equivalent to setLOD in DirectX 9
     */
    virtual void setBaseLevel(gal_uint minMipLevel) = 0;

    /**
     * Sets the maximum level of detail of the texture (clamp to maximum defined)
     *
     * By default the maximum level defined
     */ 
    virtual void setMaxLevel(gal_uint maxMipLevel) = 0;

    /**
     * Get the number of mipmaps actually setted
     *
     * 
     */ 
	virtual gal_uint getSettedMipmaps() = 0;
};

}

#endif // GAL_TEXTURE
