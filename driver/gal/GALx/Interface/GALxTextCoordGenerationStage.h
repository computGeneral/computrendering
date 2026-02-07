/**************************************************************************
 *
 */

#ifndef GALx_TEXT_COORD_GENERATION_STAGE_H
    #define GALx_TEXT_COORD_GENERATION_STAGE_H

#include "GALxGlobalTypeDefinitions.h"

namespace libGAL
{
/**
 * Texture coordinate names
 */
enum GALx_TEXTURE_COORD
{
    GALx_COORD_S,    ///< S texture coordinate
    GALx_COORD_T,    ///< T texture coordinate
    GALx_COORD_R,    ///< R texture coordinate
    GALx_COORD_Q,    ///< Q texture coordinate
};

/**
 * Texture coordinate generation planes
 */
enum GALx_TEXTURE_COORD_PLANE
{
    GALx_OBJECT_PLANE,    ///< The object linear coordinate plane 
    GALx_EYE_PLANE,        ///< The eye linear coordinate plane
};

/**
 * Interface to configure the GALx Texture Coordinate Generation Stage state
 *
 * @author Jordi Roca Monfort (jroca@ac.upc.edu)
 * @date 03/13/2007
 */
class GALxTextCoordGenerationStage
{
public:

    /**
     * Sets the texture coordinate generation object and eye planes.
     *
     * @param textureStage The texture stage affected.
     * @param plane           The Eye or Object plane.
     * @param coord           The texture coordinate affected.
     * @param a               The a coefficient of the plane equation.
     * @param b               The b coefficient of the plane equation.
     * @param c               The c coefficient of the plane equation.
     * @param d               The d coefficient of the plane equation.
     */
    virtual void setTextureCoordPlane(gal_uint textureStage, GALx_TEXTURE_COORD_PLANE plane, GALx_TEXTURE_COORD coord, gal_float a, gal_float b, gal_float c, gal_float d) = 0;

    /**
     * Sets the texture coordinate matrix
     *
     * @param textureStage The texture stage affected.
     * @param matrix       The texture coordinate matrix.
     */
    virtual void setTextureCoordMatrix(gal_uint textureStage, const GALxFloatMatrix4x4& matrix) = 0;

    /**
     * Gets the texture coordinate matrix
     *
     * @param textureStage The texture stage matrix selected
     */
    virtual GALxFloatMatrix4x4 getTextureCoordMatrix(gal_uint textureStage) const = 0;
};

} // namespace libGAL

#endif // GALx_TEXT_COORD_GENERATION_STAGE_H
