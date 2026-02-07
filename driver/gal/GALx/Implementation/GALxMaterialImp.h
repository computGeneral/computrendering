/**************************************************************************
 *
 */

#ifndef GALx_MATERIAL_IMP_H
    #define GALx_MATERIAL_IMP_H

#include "GALxTransformAndLightingStage.h"
#include "GALxGlobalTypeDefinitions.h"

namespace libGAL
{

class GALxFixedPipelineStateImp;

class GALxMaterialImp: public GALxMaterial
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
    virtual void setMaterialAmbientColor(gal_float r, gal_float g, gal_float b, gal_float a);

    /**
     * Sets the material diffuse color.
     *
     * @param r        The red component of the color.
     * @param g        The green component of the color.
     * @param b        The blue component of the color.
     * @param a        The alpha component of the color.
     */
    virtual void setMaterialDiffuseColor(gal_float r, gal_float g, gal_float b, gal_float a);

    /**
     * Sets the material specular color.
     *
     * @param r        The red component of the color.
     * @param g        The green component of the color.
     * @param b        The blue component of the color.
     * @param a        The alpha component of the color.
     */
    virtual void setMaterialSpecularColor(gal_float r, gal_float g, gal_float b, gal_float a);

    /**
     * Sets the material emission color.
     *
     * @param r        The red component of the color.
     * @param g        The green component of the color.
     * @param b        The blue component of the color.
     * @param a        The alpha component of the color.
     */
    virtual void setMaterialEmissionColor(gal_float r, gal_float g, gal_float b, gal_float a);

    /**
     * Sets the shininess factor of the material.
     *
     * @param shininess    The shininess factor of the material.
     */
    virtual void setMaterialShininess(gal_float shininess);

//////////////////////////
//  interface extension //
//////////////////////////

public:

    GALxMaterialImp(gal_uint face, GALxFixedPipelineStateImp* fpStateImp);

private:

    GALxFixedPipelineStateImp* _fpStateImp;
    gal_uint _face;

};

} // namespace libGAL

#endif // GALx_MATERIAL_IMP_H
