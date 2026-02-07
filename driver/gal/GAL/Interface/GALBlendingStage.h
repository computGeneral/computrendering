/**************************************************************************
 *
 */

#ifndef GAL_BLENDINGSTAGE
    #define GAL_BLENDINGSTAGE

#include "GALTypes.h"

namespace libGAL
{

/**
 * Blend options. A blend option identifies the data source and an optional pre-blend operation
 */
enum GAL_BLEND_OPTION
{
    GAL_BLEND_ZERO,
    GAL_BLEND_ONE,
    GAL_BLEND_SRC_COLOR,
    GAL_BLEND_INV_SRC_COLOR,
    GAL_BLEND_SRC_ALPHA,
    GAL_BLEND_INV_SRC_ALPHA,
    GAL_BLEND_DEST_ALPHA,
    GAL_BLEND_INV_DEST_ALPHA,
    GAL_BLEND_DEST_COLOR,
    GAL_BLEND_INV_DEST_COLOR,
    GAL_BLEND_SRC_ALPHA_SAT,
    GAL_BLEND_BLEND_FACTOR,
    GAL_BLEND_INV_BLEND_FACTOR,
    GAL_BLEND_CONSTANT_COLOR,
    GAL_BLEND_INV_CONSTANT_COLOR,
    GAL_BLEND_CONSTANT_ALPHA,
    GAL_BLEND_INV_CONSTANT_ALPHA,
};


/**
 * Blend operations available
 */
enum GAL_BLEND_FUNCTION
{
    GAL_BLEND_ADD,
    GAL_BLEND_SUBTRACT,
    GAL_BLEND_REVERSE_SUBTRACT,
    GAL_BLEND_MIN,
    GAL_BLEND_MAX,
};

/**
 * Interface to configure the CG1 color/blending stage
 *
 * @author Carlos González Rodríguez (cgonzale@ac.upc.edu)
 * @date 02/07/2007
 */
class GALBlendingStage
{
public:

    /**
     * Enable or disable blending for one of the render targets attached to color blending stage
     *
     * @param renderTargetID the render target position
     * @param enable True to enable blending, false to disable it
     */
    virtual void setEnable(gal_uint renderTargetID, gal_bool enable) = 0;

    /**
     * Sets the first RGB data source and includes an optional pre-blend operation
     *
     * @param renderTargetID the render target position
     * @param srcBlend the first RGB data source and an optional pre-blend operation
     */
    virtual void setSrcBlend(gal_uint renderTargetID, GAL_BLEND_OPTION srcBlend) = 0;

    /**
     * Sets the second RGB data source and includes an optional pre-blend operation
     *
     * @param renderTargetID the render target position
     * @param destBlend the second RGB data source and an optional pre-blend operation
     */
    virtual void setDestBlend(gal_uint renderTargetID, GAL_BLEND_OPTION destBlend) = 0;

    /**
     * Sets how to combine the RGB data sources
     *
     * @param renderTargetID the render target position
     * @param blendOp operation to combine RGB data sources
     */
    virtual void setBlendFunc(gal_uint renderTargetID, GAL_BLEND_FUNCTION blendOp) = 0;

    /**
     * Sets the first alpha data source and an optional pre-blend operation
     *
     * @param renderTargetID the render target position
     * @param srcBlendAlpha the first alpha data source and the optional pre-blend operation
     *
     * @note Blending operations ended with _COLOR are not allowed as a parameter for this method
     */
    virtual void setSrcBlendAlpha(gal_uint renderTargetID, GAL_BLEND_OPTION srcBlendAlpha) = 0;

    /**
     * Sets the second alpha data source and an optional pre-blend operation
     *
     * @param renderTargetID the render target position
     * @param destBlendAlpha the second alpha data source and the optional pre-blend operation
     *
     * @note Blending operations ended with _COLOR are not allowed as a parameter for this method
     */
    virtual void setDestBlendAlpha(gal_uint renderTargetID, GAL_BLEND_OPTION destBlendAlpha) = 0;
    
    /**
     * Sets how to combine the alpha data sources
     *
     * @param renderTargetID the render target position
     * @param blendOpAlpha operation to combine the alpha data sources
     */
    virtual void setBlendFuncAlpha(gal_uint renderTargetID, GAL_BLEND_FUNCTION blendOpAlpha) = 0;

    /**
     * Sets the blending color
     *
     * @param renderTargetID the render target position
     * @param red Red component of the blend color
     * @param green Green component of the blend color
     * @param blue Blue component of the blend color
     * @param alpha Alpha value of the blend color
     */
    virtual void setBlendColor(gal_uint renderTargetID, gal_float red, gal_float green, gal_float blue, gal_float alpha) = 0;

    /**
     * Sets the blending color
     *
     * @param renderTargetID the render target position
     * @param rgba 4-component vector with the RGBA blend components
     */
    virtual void setBlendColor(gal_uint renderTargetID, const gal_float* rgba) = 0;

    /**
     * Sets the color mask
     *
     * @param renderTargetID the render target position
     * @param red Active the red component
     * @param green Active the green component
     * @param blue Active the blue component
     * @param alpha Active the alpha component
     */
    virtual void setColorMask(gal_uint renderTargetID, gal_bool red, gal_bool green, gal_bool blue, gal_bool alpha) = 0;
    
    /**
     *
     *  Disables color write (render target undefined).
     *
     * @param renderTargetID the render target position
     *
     */
  
    virtual void disableColorWrite(gal_uint renderTargetID) = 0;
    
    /**
     *
     *  Disables color write (render target defined).
     *
     * @param renderTargetID the render target position
     *
     */

    virtual void enableColorWrite(gal_uint renderTargetID) = 0;
    
    /**
     *
     *  Returns if color write is enabled or disabled.
     *
     * @param renderTargetID the render target position
     *
     */

    virtual gal_bool getColorEnable(gal_uint renderTargetID) = 0;
};

}

#endif // GAL_BLENDINGSTAGE
