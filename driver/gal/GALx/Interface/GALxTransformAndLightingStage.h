/**************************************************************************
 *
 */

#ifndef GALx_TRANSFORM_AND_LIGHTING_STAGE_H
    #define GALx_TRANSFORM_AND_LIGHTING_STAGE_H

#include "GALxGlobalTypeDefinitions.h"

namespace libGAL
{

/**
 * Interface to configure a light state parameters.
 *
 * @author Jordi Roca Monfort (jroca@ac.upc.edu)
 * @date 03/13/2007
 */
class GALxLight
{
public:
    
    /**
     * Sets the light ambient color.
     *
     * @param r        The red component of the color.
     * @param g        The green component of the color.
     * @param b        The blue component of the color.
     * @param a        The alpha component of the color.
     */
    virtual void setLightAmbientColor(gal_float r, gal_float g, gal_float b, gal_float a) = 0;

    /**
     * Sets the light diffuse color.
     *
     * @param r        The red component of the color.
     * @param g        The green component of the color.
     * @param b        The blue component of the color.
     * @param a        The alpha component of the color.
     */
    virtual void setLightDiffuseColor(gal_float r, gal_float g, gal_float b, gal_float a) = 0;

    /**
     * Sets the light specular color.
     *
     * @param r        The red component of the color.
     * @param g        The green component of the color.
     * @param b        The blue component of the color.
     * @param a        The alpha component of the color.
     */
    virtual void setLightSpecularColor(gal_float r, gal_float g, gal_float b, gal_float a) = 0;

    /**
     * Sets the light position (only for point and spot lights).
     *
     * @param x        The x position coordinate.
     * @param y        The y position coordinate.
     * @param z        The z position coordinate.
     * @param w        The w position coordinate (for homogeneous cordinate system).
     */
    virtual void setLightPosition(gal_float x, gal_float y, gal_float z, gal_float w) = 0;

    /**
     * Sets the light direction (only for directional lights).
     *
     * @param x        The x direction coordinate.
     * @param y        The y direction coordinate.
     * @param z        The z direction coordinate.
     */
    virtual void setLightDirection(gal_float x, gal_float y, gal_float z) = 0;

    /**
     * Sets the light attenuation coefficients (only for point and spot lights).
     *
     * @param constant        The constant attenuation coefficient.
     * @param linear        The linear attenuation coefficient.
     * @param quadratic        The quadratic attenuation coefficient.
     */
    virtual void setAttenuationCoeffs(gal_float constant, gal_float linear, gal_float quadratic) = 0;

    /**
     * Gets the constant light attenuation coefficient (only for point and spot lights).
     */
    virtual gal_float getConstantAttenuation () = 0;

    /**
     * Gets the linear light attenuation coefficient (only for point and spot lights).
     */
    virtual gal_float getLinearAttenuation () = 0;

    /**
     * Gets the quadratic light attenuation coefficient (only for point and spot lights).
     */
    virtual gal_float getQuadraticAttenuation () = 0;

    /**
     * Sets the spot light direction (only for spot lights).
     *
     * @param x        The x direction coordinate.
     * @param y        The y direction coordinate.
     * @param z        The z direction coordinate.
     */
    virtual void setSpotLightDirection(gal_float x, gal_float y, gal_float z) = 0;

    /**
     * Sets the spot light cut-off angle (outer Phi cut-off angle in D3D).
     *
     * @param cos_angle The cosine of the cut-off angle.
     */
    virtual void setSpotLightCutOffAngle(gal_float cos_angle) = 0;

    /**
     * Sets the spot exponent or D3D falloff factor (only for spot lights).
     *
     * @param exponent The spot exponent value
     */
    virtual void setSpotExponent(gal_float exponent) = 0;
};

/**
 * Interface to configure a material state parameters.
 *
 * @author Jordi Roca Monfort (jroca@ac.upc.edu)
 * @date 03/13/2007
 */
class GALxMaterial
{
public:
    
    /**
     * Sets the material ambient color.
     *
     * @param r        The red component of the color.
     * @param g        The green component of the color.
     * @param b        The blue component of the color.
     * @param a        The alpha component of the color.
     */
    virtual void setMaterialAmbientColor(gal_float r, gal_float g, gal_float b, gal_float a) = 0;

    /**
     * Sets the material diffuse color.
     *
     * @param r        The red component of the color.
     * @param g        The green component of the color.
     * @param b        The blue component of the color.
     * @param a        The alpha component of the color.
     */
    virtual void setMaterialDiffuseColor(gal_float r, gal_float g, gal_float b, gal_float a) = 0;

    /**
     * Sets the material specular color.
     *
     * @param r        The red component of the color.
     * @param g        The green component of the color.
     * @param b        The blue component of the color.
     * @param a        The alpha component of the color.
     */
    virtual void setMaterialSpecularColor(gal_float r, gal_float g, gal_float b, gal_float a) = 0;

    /**
     * Sets the material emission color.
     *
     * @param r        The red component of the color.
     * @param g        The green component of the color.
     * @param b        The blue component of the color.
     * @param a        The alpha component of the color.
     */
    virtual void setMaterialEmissionColor(gal_float r, gal_float g, gal_float b, gal_float a) = 0;

    /**
     * Sets the shininess factor of the material.
     *
     * @param shininess    The shininess factor of the material.
     */
    virtual void setMaterialShininess(gal_float shininess) = 0;

};


/**
 * Faces 
 */
enum GALx_FACE
{
    GALx_FRONT,            ///< Front face idenfier
    GALx_BACK,            ///< Back face identifier
    GALx_FRONT_AND_BACK ///< Front and back face identifier
};

/**
 * Interface to configure the GALx Transform & Lighting state
 *
 * @author Jordi Roca Monfort (jroca@ac.upc.edu)
 * @date 03/13/2007
 */
class GALxTransformAndLightingStage
{
public:

    /**
     * Select an light object to be configured
     *
     * @param light The light number.
     * @return        The corresponding GALxLight object.
     */
    virtual GALxLight& light(gal_uint light) = 0;

    /**
     * Select a face material object to be configured
     *
     * @param face  The material properties face.
     * @return        The corresponding GALxMaterial object.
     */
    virtual GALxMaterial& material(GALx_FACE face) = 0;

    /**
     * Sets the light model ambient color
     *
     * @param r        The red component of the color.
     * @param g        The green component of the color.
     * @param b        The blue component of the color.
     * @param a        The alpha component of the color.
     */
    virtual void setLightModelAmbientColor(gal_float r, gal_float g, gal_float b, gal_float a) = 0;
        
    /**
     * Sets the light model scene color
     *
     * @param face  The face where to set the scene color.
     * @param r        The red component of the color.
     * @param g        The green component of the color.
     * @param b        The blue component of the color.
     * @param a        The alpha component of the color.
     */
    virtual void setLightModelSceneColor(GALx_FACE face, gal_float r, gal_float g, gal_float b, gal_float a) = 0;

    // TO DO: Include Transformation matrix set methods 

    /**
     * Sets the Modelview Matrix.
     *
     * @param unit     The vertex unit matrix(Vertex blending support).
     * @param matrix The input matrix.
     */
    virtual void setModelviewMatrix(gal_uint unit, const GALxFloatMatrix4x4& matrix) = 0;

    /**
     * Gets the Modelview Matrix
     *
     * @param unit The vertex unit matrix
     */
    virtual GALxFloatMatrix4x4 getModelviewMatrix(gal_uint unit) const = 0;

    /**
     * Sets the Projection Matrix.
     *
     * @param matrix The input matrix.
     */
    virtual void setProjectionMatrix(const GALxFloatMatrix4x4& matrix) = 0;

    /**
     * Gets the Projection Matrix
     *
     * @return A copy of the current Projection Matrix
     */
    virtual GALxFloatMatrix4x4 getProjectionMatrix() const = 0;

};

} // namespace libGAL

#endif // GALx_TRANSFORM_AND_LIGHTING_STAGE_H
