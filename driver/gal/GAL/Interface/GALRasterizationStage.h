/**************************************************************************
 *
 */

#ifndef GAL_RASTERIZATIONSTAGE
    #define GAL_RASTERIZATIONSTAGE

#include "GALTypes.h"

namespace libGAL
{

/**
 * Fill modes identifiers definition
 */
enum GAL_FILL_MODE
{
    GAL_FILL_SOLID,
    GAL_FILL_WIREFRAME,
};

/**
 * Cull modes identifiers definition
 */
enum GAL_CULL_MODE
{
    GAL_CULL_NONE,
    GAL_CULL_FRONT,
    GAL_CULL_BACK,
    GAL_CULL_FRONT_AND_BACK
};

enum GAL_FACE_MODE
{
    GAL_FACE_CW,
    GAL_FACE_CCW
};

enum GAL_INTERPOLATION_MODE
{
    GAL_INTERPOLATION_NONE,
    GAL_INTERPOLATION_LINEAR, // Default value
    GAL_INTERPOLATION_CENTROID,
    GAL_INTERPOLATION_NOPERSPECTIVE,
};

/**
 * Interface to configure the CG1 rasterization stage
 *
 * @author Carlos González Rodríguez (cgonzale@ac.upc.edu)
 * @date 02/07/2007
 */
class GALRasterizationStage
{
public:

    /**
     * Sets the interpolation mode for a fragment shader input attribute
     *
     * @param The fragment shader input attribute identifier
     * @param mode The interpolation mode
     */
    virtual void setInterpolationMode(gal_uint fshInputAttribute, GAL_INTERPOLATION_MODE mode) = 0;

    /**
     * Enables the scissor test
     *
     * @param enable: enable/disable the scissor test
     */
    virtual void enableScissor(gal_bool enable) = 0;

    /**
     * Sets the fill mode to use when rendering [default: GAL_FILL_MODE_SOLID]
     *
     * @param fillMode The fill mode to use when rendering
     *
     * @note GAL_WIREFRAME is not currently supported
     */
    virtual void setFillMode(GAL_FILL_MODE fillMode) = 0;

    /**
     * Sets the triangle culling mode [default: GAL_CULL_NONE]
     *
     * @param cullMode the triangle culling mode
     */
    virtual void setCullMode(GAL_CULL_MODE cullMode) = 0;

    /**
     *Sets the triangle facing mode [default: GAL_CW]
     *
     * @param faceMode the triangle facing mode
     */
    virtual void setFaceMode(GAL_FACE_MODE faceMode) = 0;

    /**
     *
     *  Sets if the D3D9 rasterization rules will be used, otherwise the OpenGL rules will be used
     *
     *  @param use TRUE for D3D9 rasterization rules, FALSE for OGL rasterization rules.
     *
     */
     
    virtual void useD3D9RasterizationRules(gal_bool use) = 0;
    
    /**
     *
     *  Sets if the D3D9 pixel coordinates convention will be used (top left is [0,0]),
     *  otherwise the OpenGL convention will be used (bottom left is [0,0]).
     *
     *  @param use TRUE for D3D9 pixel coordinates convention, FALSE for OpenGL convention.
     *
     */
     
    virtual void useD3D9PixelCoordConvention(gal_bool use) = 0;
    
    /**
     * Sets the viewport rectangle
     *
     * @param x
     * @param y
     * @param width
     * @param height
     */
    virtual void setViewport(gal_int x, gal_int y, gal_uint width, gal_uint height) = 0;

    /**
     * Sets the scissor rectangle
     *
     * @param x
     * @param y
     * @param width
     * @param height
     */
    virtual void setScissor(gal_int x, gal_int y, gal_uint width, gal_uint height) = 0;

    /**
     *
     *  Gets the viewport rectange.
     *
     *  @param x
     *  @param y
     *  @param width
     *  @param height
     *
     */
     
    virtual void getViewport(gal_int &x, gal_int &y, gal_uint &width, gal_uint &height) = 0;

    /**
     *  Gets the scissor rectangle
     *
     *  @param enabled
     *  @param x
     *  @param y
     *  @param width
     *  @param height
     *
     */
     
    virtual void getScissor(gal_bool &enabled, gal_int &x, gal_int &y, gal_uint &width, gal_uint &height) = 0;
    
};

}

#endif // GAL_RASTERIZATIONSTAGE
