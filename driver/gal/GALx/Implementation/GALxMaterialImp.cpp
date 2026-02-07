/**************************************************************************
 *
 */

#include "GALxMaterialImp.h"
#include "GALxFixedPipelineStateImp.h"

using namespace libGAL;

GALxMaterialImp::GALxMaterialImp(gal_uint face, GALxFixedPipelineStateImp *fpStateImp)
: _face(face), _fpStateImp(fpStateImp)
{

}

void GALxMaterialImp::setMaterialAmbientColor(gal_float r, gal_float g, gal_float b, gal_float a)
{
    GALxFloatVector4 materialAmbientColor;

    materialAmbientColor[0] = r;
    materialAmbientColor[1] = g;
    materialAmbientColor[2] = b;
    materialAmbientColor[3] = a;

    if (_face == 0)
        _fpStateImp->_materialFrontAmbient = materialAmbientColor;
    else
        _fpStateImp->_materialBackAmbient = materialAmbientColor;
}

void GALxMaterialImp::setMaterialDiffuseColor(gal_float r, gal_float g, gal_float b, gal_float a)
{
    GALxFloatVector4 materialDiffuseColor;

    materialDiffuseColor[0] = r;
    materialDiffuseColor[1] = g;
    materialDiffuseColor[2] = b;
    materialDiffuseColor[3] = a;

    if (_face == 0)
        _fpStateImp->_materialFrontDiffuse = materialDiffuseColor;
    else
        _fpStateImp->_materialBackDiffuse = materialDiffuseColor;
}

void GALxMaterialImp::setMaterialSpecularColor(gal_float r, gal_float g, gal_float b, gal_float a)
{
    GALxFloatVector4 materialSpecularColor;

    materialSpecularColor[0] = r;
    materialSpecularColor[1] = g;
    materialSpecularColor[2] = b;
    materialSpecularColor[3] = a;

    if (_face == 0)
        _fpStateImp->_materialFrontSpecular = materialSpecularColor;
    else
        _fpStateImp->_materialBackSpecular = materialSpecularColor;
}

void GALxMaterialImp::setMaterialEmissionColor(gal_float r, gal_float g, gal_float b, gal_float a)
{
    GALxFloatVector4 materialEmissionColor;

    materialEmissionColor[0] = r;
    materialEmissionColor[1] = g;
    materialEmissionColor[2] = b;
    materialEmissionColor[3] = a;

    if (_face == 0)
        _fpStateImp->_materialFrontEmission = materialEmissionColor;
    else
        _fpStateImp->_materialBackEmission = materialEmissionColor;

}

void GALxMaterialImp::setMaterialShininess(gal_float shininess)
{
    if (_face == 0)
        _fpStateImp->_materialFrontShininess = shininess;
    else
        _fpStateImp->_materialBackShininess = shininess;
}
