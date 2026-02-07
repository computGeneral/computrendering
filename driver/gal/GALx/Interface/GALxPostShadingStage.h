/**************************************************************************
 *
 */

#ifndef GALx_POST_SHADING_STAGE_H
    #define GALx_POST_SHADING_STAGE_H

#include "GALxGlobalTypeDefinitions.h"

namespace libGAL
{

/**
 * Interface to configure the GALx Post Fragment Shading Stages state (FOG and Alpha).
 *
 * @author Jordi Roca Monfort (jroca@ac.upc.edu)
 * @date 03/13/2007
 */
class GALxPostShadingStage
{
public:

    /////////
    // FOG //
    /////////

    /**
     * Sets the FOG blending color.
     *
     * @param r        The red component of the color.
     * @param g        The green component of the color.
     * @param b        The blue component of the color.
     * @param a        The alpha component of the color.
     */
     virtual void setFOGBlendColor(gal_float r, gal_float g, gal_float b, gal_float a) = 0; 

    /**
     * Sets the FOG density exponent
     *
     * @param exponent The density exponent
     */
     virtual void setFOGDensity(gal_float exponent) = 0;

    /**
     * Sets the FOG linear mode start and end distances.
     *
     * @param start The FOG linear start distance.
     * @param end   The FOG linear end distance.
     */
     virtual void setFOGLinearDistances(gal_float start, gal_float end) = 0;

     ////////////////
     // ALPHA TEST //
     ////////////////

     /**
      * Sets the Alpha Test Reference Value.
      *
      * @param refValue The reference value.
      */
     virtual void setAlphaTestRefValue(gal_float refValue) = 0;
};

} // namespace libGAL

#endif // GALx_POST_SHADING_STAGE_H
