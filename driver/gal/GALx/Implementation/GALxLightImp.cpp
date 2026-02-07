/**************************************************************************
 *
 */

#include "GALxLightImp.h"
#include "GALxFixedPipelineStateImp.h"

using namespace libGAL;

GALxLightImp::GALxLightImp(gal_uint lightNumber, GALxFixedPipelineStateImp* fpStateImp)
: _lightNumber(lightNumber), _fpStateImp(fpStateImp)
{
}

void GALxLightImp::setLightAmbientColor(gal_float r, gal_float g, gal_float b, gal_float a)
{
    GALxFloatVector4 ambientColor;

    ambientColor[0] = r;
    ambientColor[1] = g;
    ambientColor[2] = b;
    ambientColor[3] = a;

    _fpStateImp->_lightAmbient[_lightNumber] = ambientColor;
}

void GALxLightImp::setLightDiffuseColor(gal_float r, gal_float g, gal_float b, gal_float a)
{
    GALxFloatVector4 diffuseColor;

    diffuseColor[0] = r;
    diffuseColor[1] = g;
    diffuseColor[2] = b;
    diffuseColor[3] = a;

    _fpStateImp->_lightDiffuse[_lightNumber] = diffuseColor;
}

void GALxLightImp::setLightSpecularColor(gal_float r, gal_float g, gal_float b, gal_float a)
{
    GALxFloatVector4 specularColor;

    specularColor[0] = r;
    specularColor[1] = g;
    specularColor[2] = b;
    specularColor[3] = a;

    _fpStateImp->_lightSpecular[_lightNumber] = specularColor;
}

void GALxLightImp::setLightPosition(gal_float x, gal_float y, gal_float z, gal_float w)
{
    GALxFloatVector4 position;

    position[0] = x;
    position[1] = y;
    position[2] = z;
    position[3] = w;

    _fpStateImp->_lightPosition[_lightNumber] = position;
}

void GALxLightImp::setLightDirection(gal_float x, gal_float y, gal_float z)
{
    GALxFloatVector3 direction;

    direction[0] = x;
    direction[1] = y;
    direction[2] = z;

    _fpStateImp->_lightDirection[_lightNumber] = direction;
}


void GALxLightImp::setAttenuationCoeffs(gal_float constant, gal_float linear, gal_float quadratic)
{
    GALxFloatVector3 attenuationCoeffs;

    attenuationCoeffs[0] = constant;
    attenuationCoeffs[1] = linear;
    attenuationCoeffs[2] = quadratic;

    _fpStateImp->_lightAttenuation[_lightNumber] = attenuationCoeffs;
}

gal_float GALxLightImp::getConstantAttenuation () 
{
    GALxFloatVector3 attenuationCoeffs;
    attenuationCoeffs = _fpStateImp->_lightAttenuation[_lightNumber];

    return attenuationCoeffs[0];
}

gal_float GALxLightImp::getLinearAttenuation () 
{
    GALxFloatVector3 attenuationCoeffs;
    attenuationCoeffs = _fpStateImp->_lightAttenuation[_lightNumber];

    return attenuationCoeffs[1];
}

gal_float GALxLightImp::getQuadraticAttenuation () 
{
    GALxFloatVector3 attenuationCoeffs;
    attenuationCoeffs = _fpStateImp->_lightAttenuation[_lightNumber];

    return attenuationCoeffs[2];
}

void GALxLightImp::setSpotLightDirection(gal_float x, gal_float y, gal_float z)
{
    GALxFloatVector3 spotDirection;

    spotDirection[0] = x;
    spotDirection[1] = y;
    spotDirection[2] = z;
    
    _fpStateImp->_lightSpotDirection[_lightNumber] = spotDirection;
}

void GALxLightImp::setSpotLightCutOffAngle(gal_float cos_angle)
{
    _fpStateImp->_lightSpotCutOffAngle[_lightNumber] = cos_angle;
}

void GALxLightImp::setSpotExponent(gal_float exponent)
{
    _fpStateImp->_lightSpotExponent[_lightNumber] = exponent;
}

