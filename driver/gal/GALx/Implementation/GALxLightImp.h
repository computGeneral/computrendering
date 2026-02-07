/**************************************************************************
 *
 */

#ifndef GALx_LIGHT_IMP_H
    #define GALx_LIGHT_IMP_H

#include "GALxGlobalTypeDefinitions.h"
#include "GALxTransformAndLightingStage.h"

namespace libGAL
{

class GALxFixedPipelineStateImp;

class GALxLightImp: public GALxLight
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
    virtual void setLightAmbientColor(gal_float r, gal_float g, gal_float b, gal_float a);

    /**
     * Sets the light diffuse color.
     *
     * @param r        The red component of the color.
     * @param g        The green component of the color.
     * @param b        The blue component of the color.
     * @param a        The alpha component of the color.
     */
    virtual void setLightDiffuseColor(gal_float r, gal_float g, gal_float b, gal_float a);

    /**
     * Sets the light specular color.
     *
     * @param r        The red component of the color.
     * @param g        The green component of the color.
     * @param b        The blue component of the color.
     * @param a        The alpha component of the color.
     */
    virtual void setLightSpecularColor(gal_float r, gal_float g, gal_float b, gal_float a);

    /**
     * Sets the light position (only for point and spot lights).
     *
     * @param x        The x position coordinate.
     * @param y        The y position coordinate.
     * @param z        The z position coordinate.
     * @param w        The w position coordinate (for homogeneous cordinate system).
     */
    virtual void setLightPosition(gal_float x, gal_float y, gal_float z, gal_float w);

    /**
     * Sets the light direction (only for directional lights).
     *
     * @param x        The x direction coordinate.
     * @param y        The y direction coordinate.
     * @param z        The z direction coordinate.
     */
    virtual void setLightDirection(gal_float x, gal_float y, gal_float z);

    /**
     * Sets the light attenuation coefficients (only for point and spot lights).
     *
     * @param constant        The constant attenuation coefficient.
     * @param linear        The linear attenuation coefficient.
     * @param quadratic        The quadratic attenuation coefficient.
     */
    virtual void setAttenuationCoeffs(gal_float constant, gal_float linear, gal_float quadratic);

    /**
     * Gets the constant light attenuation coefficient (only for point and spot lights).
     */
    virtual gal_float getConstantAttenuation ();

    /**
     * Gets the linear light attenuation coefficient (only for point and spot lights).
     */
    virtual gal_float getLinearAttenuation ();

    /**
     * Gets the quadratic light attenuation coefficient (only for point and spot lights).
     */
    virtual gal_float getQuadraticAttenuation ();
    /**
     * Sets the spot light direction (only for spot lights).
     *
     * @param x        The x direction coordinate.
     * @param y        The y direction coordinate.
     * @param z        The z direction coordinate.
     */
    virtual void setSpotLightDirection(gal_float x, gal_float y, gal_float z);

    /**
     * Sets the spot light cut-off angle (outer Phi cut-off angle).
     *
     * @param cos_angle The cosine of the cut-off angle.
     */
    virtual void setSpotLightCutOffAngle(gal_float cos_angle);

    /**
     * Sets the spot exponent or D3D falloff factor (only for spot lights).
     *
     * @param exponent The spot exponent value
     */
    virtual void setSpotExponent(gal_float exponent);

//////////////////////////
//  interface extension //
//////////////////////////

public:

    GALxLightImp(gal_uint lightNumber, GALxFixedPipelineStateImp* fpStateImp);

private:

    GALxFixedPipelineStateImp* _fpStateImp;
    gal_uint _lightNumber;

};

} // namespace libGAL

#endif // GALx_LIGHT_IMP_H
