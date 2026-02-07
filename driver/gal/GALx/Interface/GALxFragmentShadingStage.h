/**************************************************************************
 *
 */

#ifndef GALx_FRAGMENT_SHADING_STAGE_H
    #define GALx_FRAGMENT_SHADING_STAGE_H

#include "GALxGlobalTypeDefinitions.h"

namespace libGAL
{

/**
 * Interface to configure the GALx Fragment Shading Stage state
 *
 * @author Jordi Roca Monfort (jroca@ac.upc.edu)
 * @date 03/13/2007
 */
class GALxFragmentShadingStage
{
public:

    /**
     * Sets the texture environment color used for each texture stage.
     *
     * @param textureStage    The corresponding texture stage.
     * @param r                The red component of the color.
     * @param g                The green component of the color.
     * @param b                The    blue component of the color.
     * @param a                The alpha component of the color.
     */
     virtual void setTextureEnvColor(gal_uint textureStage, gal_float r, gal_float g, gal_float b, gal_float a) = 0;

    /**
     * Sets the depth range (near and far plane distances)
     *
     * @param near           The distance of the near plane.
     * @param far           The distance of the far plane.
     */
     virtual void setDepthRange(gal_float near, gal_float far) = 0;
};

} // namespace libGAL

#endif // GALx_FRAGMENT_SHADING_STAGE_H
