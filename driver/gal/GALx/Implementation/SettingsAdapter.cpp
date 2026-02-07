/**************************************************************************
 *
 */

#include "SettingsAdapter.h"
#include "support.h"

using namespace libGAL;

SettingsAdapter::SettingsAdapter(const GALx_FIXED_PIPELINE_SETTINGS& fpSettings)
: settings(fpSettings), fpState(this), tlState(this)
{
}

/////////////////////////////////////////////////////////////////////////
///////// Implementation of inner class TLStateImplementation ///////////
/////////////////////////////////////////////////////////////////////////

#define TLSImp SettingsAdapter::TLStateImplementation

TLSImp::TLStateImplementation(SettingsAdapter* parent) : parent(parent)
{}


bool TLSImp::normalizeNormals()
{
    return (parent->settings.normalizeNormals == GALx_FIXED_PIPELINE_SETTINGS::GALx_NORMALIZE);
}

bool TLSImp::isLighting()
{
    return (parent->settings.lightingEnabled);
}

bool TLSImp::isRescaling()
{
    return (parent->settings.normalizeNormals == GALx_FIXED_PIPELINE_SETTINGS::GALx_RESCALE);
}


bool TLSImp::infiniteViewer()
{
    return !(parent->settings.localViewer);
}

bool TLSImp::localLightSource(Light light)
{
    if (light >= GALx_FIXED_PIPELINE_SETTINGS::GALx_MAX_LIGHTS)
        CG_ASSERT("Requested light out of range");
    
    return ((parent->settings.lights[light].lightType == GALx_FIXED_PIPELINE_SETTINGS::GALx_LIGHT_SETTINGS::GALx_POINT) ||
           (parent->settings.lights[light].lightType == GALx_FIXED_PIPELINE_SETTINGS::GALx_LIGHT_SETTINGS::GALx_SPOT));
}

void TLSImp::attenuationCoeficients(Light , float& , float& , float&)
{
    CG_ASSERT("Not implemented. Coefs shouldn´t be required by the FF emulation");
}

bool TLSImp::isSpotLight(Light light)
{
    if (light >= GALx_FIXED_PIPELINE_SETTINGS::GALx_MAX_LIGHTS)
        CG_ASSERT("Requested light out of range");

    return (parent->settings.lights[light].lightType == GALx_FIXED_PIPELINE_SETTINGS::GALx_LIGHT_SETTINGS::GALx_SPOT);
}

bool TLSImp::isLightEnabled(Light light)
{
    if (light >= GALx_FIXED_PIPELINE_SETTINGS::GALx_MAX_LIGHTS)
        CG_ASSERT("Requested light out of range");

    return (parent->settings.lights[light].enabled);
}

int TLSImp::maxLights()
{
    return (GALx_FIXED_PIPELINE_SETTINGS::GALx_MAX_LIGHTS);
}

bool TLSImp::separateSpecular()
{
    return (parent->settings.separateSpecular);
}

bool TLSImp::twoSidedLighting()
{
    return (parent->settings.twoSidedLighting);
}

bool TLSImp::anyLightEnabled()
{
    int i=0;
    bool found = false;

    while (i<GALx_FIXED_PIPELINE_SETTINGS::GALx_MAX_LIGHTS && !found)
    {
        if (parent->settings.lights[i].enabled) found = true;
        i++;
    }

    return found;
}

bool TLSImp::anyLocalLight()
{
    int i=0;
    bool found = false;

    while (i<GALx_FIXED_PIPELINE_SETTINGS::GALx_MAX_LIGHTS && !found)
    {
        if ((parent->settings.lights[i].lightType == GALx_FIXED_PIPELINE_SETTINGS::GALx_LIGHT_SETTINGS::GALx_POINT) ||
           (parent->settings.lights[i].lightType == GALx_FIXED_PIPELINE_SETTINGS::GALx_LIGHT_SETTINGS::GALx_SPOT))
            found = true;

        i++;
    }

    return found;
}

bool TLSImp::isCullingEnabled()
{
    return (parent->settings.cullEnabled);
}

bool TLSImp::isFrontFaceCulling()
{
    return ((parent->settings.cullMode == GALx_FIXED_PIPELINE_SETTINGS::GALx_CULL_FRONT) ||
            (parent->settings.cullMode == GALx_FIXED_PIPELINE_SETTINGS::GALx_CULL_FRONT_AND_BACK));
}

bool TLSImp::isBackFaceCulling()
{
    return ((parent->settings.cullMode == GALx_FIXED_PIPELINE_SETTINGS::GALx_CULL_BACK) ||
            (parent->settings.cullMode == GALx_FIXED_PIPELINE_SETTINGS::GALx_CULL_FRONT_AND_BACK));
}

GALxTLState::ColorMaterialMode TLSImp::colorMaterialMode()
{
    switch(parent->settings.colorMaterialMode)
    {
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_EMISSION: return GALxTLState::EMISSION;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_AMBIENT: return GALxTLState::AMBIENT;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_DIFFUSE: return GALxTLState::DIFFUSE;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_SPECULAR: return GALxTLState::SPECULAR;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_AMBIENT_AND_DIFFUSE: return GALxTLState::AMBIENT_AND_DIFFUSE;
        default:
            CG_ASSERT("Unsupported color material mode");
            return GALxTLState::AMBIENT_AND_DIFFUSE; // to avoid stupid compiler warnings
    }
}

bool TLSImp::colorMaterialFrontEnabled()
{
    return ((parent->settings.colorMaterialEnabledFace == GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_FRONT) ||
            (parent->settings.colorMaterialEnabledFace == GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_FRONT_AND_BACK));
}

bool TLSImp::colorMaterialBackEnabled()
{
    return ((parent->settings.colorMaterialEnabledFace == GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_BACK) ||
            (parent->settings.colorMaterialEnabledFace == GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_FRONT_AND_BACK));
}

bool TLSImp::isTextureUnitEnabled(GLuint texUnit)
{
    if (texUnit >= GALx_FIXED_PIPELINE_SETTINGS::GALx_MAX_TEXTURE_STAGES)
        CG_ASSERT("Requested texture stage out of range");

    return (parent->settings.textureStages[texUnit].enabled);
}

bool TLSImp::anyTextureUnitEnabled()
{
    int i=0;
    bool found = false;

    while ( i < GALx_FIXED_PIPELINE_SETTINGS::GALx_MAX_TEXTURE_STAGES && !found)
    {
        if (parent->settings.textureStages[i].enabled) found = true;
        i++;
    }

    return found;
}


int TLSImp::maxTextureUnits()
{
    return GALx_FIXED_PIPELINE_SETTINGS::GALx_MAX_TEXTURE_STAGES;
}


GALxTLState::TextureUnit TLSImp::getTextureUnit(GLuint unit)
{
    if (unit >= GALx_FIXED_PIPELINE_SETTINGS::GALx_MAX_TEXTURE_STAGES)
        CG_ASSERT("Requested texture stage out of range");

    GALxTLState::TextureUnit tu;

    tu.unitId = unit;

    tu.activeTexGenS = (parent->settings.textureCoordinates[unit].coordS != GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_VERTEX_ATTRIB);
    tu.activeTexGenT = (parent->settings.textureCoordinates[unit].coordT != GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_VERTEX_ATTRIB);
    tu.activeTexGenR = (parent->settings.textureCoordinates[unit].coordR != GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_VERTEX_ATTRIB);
    tu.activeTexGenQ = (parent->settings.textureCoordinates[unit].coordQ != GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_VERTEX_ATTRIB);
    
    switch (parent->settings.textureCoordinates[unit].coordS)
    {
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_VERTEX_ATTRIB: break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_OBJECT_LINEAR: tu.texGenModeS = GALxTLState::TextureUnit::OBJECT_LINEAR; break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_EYE_LINEAR: tu.texGenModeS = GALxTLState::TextureUnit::EYE_LINEAR; break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_SPHERE_MAP: tu.texGenModeS = GALxTLState::TextureUnit::SPHERE_MAP; break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_REFLECTION_MAP: tu.texGenModeS = GALxTLState::TextureUnit::REFLECTION_MAP; break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_NORMAL_MAP: tu.texGenModeS = GALxTLState::TextureUnit::NORMAL_MAP; break;
    }
    
    switch (parent->settings.textureCoordinates[unit].coordT)
    {
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_VERTEX_ATTRIB: break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_OBJECT_LINEAR: tu.texGenModeT = GALxTLState::TextureUnit::OBJECT_LINEAR; break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_EYE_LINEAR: tu.texGenModeT = GALxTLState::TextureUnit::EYE_LINEAR; break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_SPHERE_MAP: tu.texGenModeT = GALxTLState::TextureUnit::SPHERE_MAP; break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_REFLECTION_MAP: tu.texGenModeT = GALxTLState::TextureUnit::REFLECTION_MAP; break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_NORMAL_MAP: tu.texGenModeT = GALxTLState::TextureUnit::NORMAL_MAP; break;
    }

    switch (parent->settings.textureCoordinates[unit].coordR)
    {
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_VERTEX_ATTRIB: break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_OBJECT_LINEAR: tu.texGenModeR = GALxTLState::TextureUnit::OBJECT_LINEAR; break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_EYE_LINEAR: tu.texGenModeR = GALxTLState::TextureUnit::EYE_LINEAR; break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_SPHERE_MAP: tu.texGenModeR = GALxTLState::TextureUnit::SPHERE_MAP; break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_REFLECTION_MAP: tu.texGenModeR = GALxTLState::TextureUnit::REFLECTION_MAP; break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_NORMAL_MAP: tu.texGenModeR = GALxTLState::TextureUnit::NORMAL_MAP; break;
    }

    switch (parent->settings.textureCoordinates[unit].coordQ)
    {
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_VERTEX_ATTRIB: break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_OBJECT_LINEAR: tu.texGenModeQ = GALxTLState::TextureUnit::OBJECT_LINEAR; break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_EYE_LINEAR: tu.texGenModeQ = GALxTLState::TextureUnit::EYE_LINEAR; break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_SPHERE_MAP: tu.texGenModeQ = GALxTLState::TextureUnit::SPHERE_MAP; break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_REFLECTION_MAP: tu.texGenModeQ = GALxTLState::TextureUnit::REFLECTION_MAP; break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_NORMAL_MAP: tu.texGenModeQ = GALxTLState::TextureUnit::NORMAL_MAP; break;
    }

    tu.textureMatrixIsIdentity = parent->settings.textureCoordinates[unit].textureMatrixIsIdentity;

    return tu;
}

GALxTLState::FogCoordSrc TLSImp::fogCoordSrc()
{
    switch(parent->settings.fogCoordinateSource)
    {
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_FRAGMENT_DEPTH: return GALxTLState::FRAGMENT_DEPTH;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_FOG_COORD: return GALxTLState::FOG_COORD;
        default:
            CG_ASSERT("Unsupported fog coord source");
            return GALxTLState::FOG_COORD; // to avoid stupid compiler warnings 
    }
}

bool TLSImp::fogEnabled()
{
    return (parent->settings.fogEnabled);
}


/////////////////////////////////////////////////////////////////////////
///////// Implementation of inner class FPStateImplementation ///////////
/////////////////////////////////////////////////////////////////////////

#define FPSImp SettingsAdapter::FPStateImplementation

FPSImp::FPStateImplementation(SettingsAdapter* parent) : parent(parent)
{}

GALxFPState::AlphaTestFunc FPSImp::alphaTestFunc()
{
    switch(parent->settings.alphaTestFunction)
    {
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_ALPHA_NEVER: return GALxFPState::NEVER;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_ALPHA_ALWAYS: return GALxFPState::ALWAYS;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_ALPHA_LESS: return GALxFPState::LESS;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_ALPHA_LEQUAL: return GALxFPState::LEQUAL;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_ALPHA_EQUAL: return GALxFPState::EQUAL;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_ALPHA_GEQUAL: return GALxFPState::GEQUAL;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_ALPHA_GREATER: return GALxFPState::GREATER;
        default:
            CG_ASSERT("Unsupported Alpha test function");
            return GALxFPState::NOTEQUAL;
    }
}

bool FPSImp::alphaTestEnabled()
{
    return (parent->settings.alphaTestEnabled);
}

bool FPSImp::separateSpecular()
{
    return (parent->settings.separateSpecular);
}

bool FPSImp::anyTextureUnitEnabled()
{
    int i=0;

    bool found = false;

    while ( i < GALx_FIXED_PIPELINE_SETTINGS::GALx_MAX_TEXTURE_STAGES && !found)
    {
        if (parent->settings.textureStages[i].enabled) found = true;
        i++;
    }

    return found;
}

int FPSImp::maxTextureUnits()
{
    return GALx_FIXED_PIPELINE_SETTINGS::GALx_MAX_TEXTURE_STAGES;
}

bool FPSImp::isTextureUnitEnabled(GLuint unit)
{
    if (unit >= GALx_FIXED_PIPELINE_SETTINGS::GALx_MAX_TEXTURE_STAGES)
        CG_ASSERT("Requested texture stage out of range");

    return (parent->settings.textureStages[unit].enabled);
}

GALxFPState::TextureUnit FPSImp::getTextureUnit(GLuint unit)
{
    if (unit >= GALx_FIXED_PIPELINE_SETTINGS::GALx_MAX_TEXTURE_STAGES)
        CG_ASSERT("Requested texture stage out of range");
    
    GALxFPState::TextureUnit tu(unit);

    switch(parent->settings.textureStages[unit].activeTextureTarget)
    {
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_TEXTURE_1D: tu.activeTarget = GALxFPState::TextureUnit::TEXTURE_1D; break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_TEXTURE_2D: tu.activeTarget = GALxFPState::TextureUnit::TEXTURE_2D; break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_TEXTURE_3D: tu.activeTarget = GALxFPState::TextureUnit::TEXTURE_3D; break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_TEXTURE_CUBE_MAP: tu.activeTarget = GALxFPState::TextureUnit::CUBE_MAP; break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_TEXTURE_RECT: CG_ASSERT("RECT Texture target not yet supported");
    }

    switch(parent->settings.textureStages[unit].textureStageFunction)
    {
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_REPLACE: tu.function = GALxFPState::TextureUnit::REPLACE; break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_MODULATE: tu.function = GALxFPState::TextureUnit::MODULATE; break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_DECAL: tu.function = GALxFPState::TextureUnit::DECAL; break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_BLEND: tu.function = GALxFPState::TextureUnit::BLEND; break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_ADD: tu.function = GALxFPState::TextureUnit::ADD; break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_COMBINE: tu.function = GALxFPState::TextureUnit::COMBINE; break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_COMBINE_NV: tu.function = GALxFPState::TextureUnit::COMBINE4_NV; break;
    }

    switch(parent->settings.textureStages[unit].baseInternalFormat)
    {
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_ALPHA: tu.format = GALxFPState::TextureUnit::ALPHA; break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_LUMINANCE: tu.format = GALxFPState::TextureUnit::LUMINANCE; break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_LUMINANCE_ALPHA: tu.format = GALxFPState::TextureUnit::LUMINANCE_ALPHA; break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_DEPTH: tu.format = GALxFPState::TextureUnit::DEPTH; break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_INTENSITY: tu.format = GALxFPState::TextureUnit::INTENSITY; break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_RGB: tu.format = GALxFPState::TextureUnit::RGB; break;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_RGBA: tu.format = GALxFPState::TextureUnit::RGBA; break;
    }

    // Combine parameters 
    
    switch(parent->settings.textureStages[unit].combineSettings.RGBFunction)
    {
        case GALx_COMBINE_SETTINGS::GALx_COMBINE_REPLACE: tu.combineMode.combineRGBFunction = GALxFPState::TextureUnit::CombineMode::REPLACE; break;
        case GALx_COMBINE_SETTINGS::GALx_COMBINE_MODULATE: tu.combineMode.combineRGBFunction = GALxFPState::TextureUnit::CombineMode::MODULATE; break;
        case GALx_COMBINE_SETTINGS::GALx_COMBINE_ADD: tu.combineMode.combineRGBFunction = GALxFPState::TextureUnit::CombineMode::ADD; break;
        case GALx_COMBINE_SETTINGS::GALx_COMBINE_ADD_SIGNED: tu.combineMode.combineRGBFunction = GALxFPState::TextureUnit::CombineMode::ADD_SIGNED; break;
        case GALx_COMBINE_SETTINGS::GALx_COMBINE_INTERPOLATE: tu.combineMode.combineRGBFunction = GALxFPState::TextureUnit::CombineMode::INTERPOLATE; break;
        case GALx_COMBINE_SETTINGS::GALx_COMBINE_SUBTRACT: tu.combineMode.combineRGBFunction = GALxFPState::TextureUnit::CombineMode::SUBTRACT; break;
        case GALx_COMBINE_SETTINGS::GALx_COMBINE_DOT3_RGB: tu.combineMode.combineRGBFunction = GALxFPState::TextureUnit::CombineMode::DOT3_RGB; break;
        case GALx_COMBINE_SETTINGS::GALx_COMBINE_DOT3_RGBA: tu.combineMode.combineRGBFunction = GALxFPState::TextureUnit::CombineMode::DOT3_RGBA; break;
        case GALx_COMBINE_SETTINGS::GALx_COMBINE_MODULATE_ADD: tu.combineMode.combineRGBFunction = GALxFPState::TextureUnit::CombineMode::MODULATE_ADD_ATI; break;
        case GALx_COMBINE_SETTINGS::GALx_COMBINE_MODULATE_SIGNED_ADD: tu.combineMode.combineRGBFunction = GALxFPState::TextureUnit::CombineMode::MODULATE_SIGNED_ADD_ATI; break;
        case GALx_COMBINE_SETTINGS::GALx_COMBINE_MODULATE_SUBTRACT: tu.combineMode.combineRGBFunction = GALxFPState::TextureUnit::CombineMode::MODULATE_SUBTRACT_ATI; break;
    }

    switch(parent->settings.textureStages[unit].combineSettings.ALPHAFunction)
    {
        case GALx_COMBINE_SETTINGS::GALx_COMBINE_REPLACE: tu.combineMode.combineALPHAFunction = GALxFPState::TextureUnit::CombineMode::REPLACE; break;
        case GALx_COMBINE_SETTINGS::GALx_COMBINE_MODULATE: tu.combineMode.combineALPHAFunction = GALxFPState::TextureUnit::CombineMode::MODULATE; break;
        case GALx_COMBINE_SETTINGS::GALx_COMBINE_ADD: tu.combineMode.combineALPHAFunction = GALxFPState::TextureUnit::CombineMode::ADD; break;
        case GALx_COMBINE_SETTINGS::GALx_COMBINE_ADD_SIGNED: tu.combineMode.combineALPHAFunction = GALxFPState::TextureUnit::CombineMode::ADD_SIGNED; break;
        case GALx_COMBINE_SETTINGS::GALx_COMBINE_INTERPOLATE: tu.combineMode.combineALPHAFunction = GALxFPState::TextureUnit::CombineMode::INTERPOLATE; break;
        case GALx_COMBINE_SETTINGS::GALx_COMBINE_SUBTRACT: tu.combineMode.combineALPHAFunction = GALxFPState::TextureUnit::CombineMode::SUBTRACT; break;
        case GALx_COMBINE_SETTINGS::GALx_COMBINE_MODULATE_ADD: tu.combineMode.combineALPHAFunction = GALxFPState::TextureUnit::CombineMode::MODULATE_ADD_ATI; break;
        case GALx_COMBINE_SETTINGS::GALx_COMBINE_MODULATE_SIGNED_ADD: tu.combineMode.combineALPHAFunction = GALxFPState::TextureUnit::CombineMode::MODULATE_SIGNED_ADD_ATI; break;
        case GALx_COMBINE_SETTINGS::GALx_COMBINE_MODULATE_SUBTRACT: tu.combineMode.combineALPHAFunction = GALxFPState::TextureUnit::CombineMode::MODULATE_SUBTRACT_ATI; break;
    }

    for (int j=0; j<4; j++)
    {
        switch(parent->settings.textureStages[unit].combineSettings.srcRGB[j])
        {
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_ZERO: tu.combineMode.srcRGB[j] = GALxFPState::TextureUnit::CombineMode::ZERO; break;
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_ONE: tu.combineMode.srcRGB[j] = GALxFPState::TextureUnit::CombineMode::ONE; break;
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_TEXTURE: GALxFPState::TextureUnit::CombineMode::TEXTURE; break;
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_TEXTUREn: GALxFPState::TextureUnit::CombineMode::TEXTUREn; break;
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_CONSTANT: GALxFPState::TextureUnit::CombineMode::CONSTANT; break;
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_PRIMARY_COLOR: GALxFPState::TextureUnit::CombineMode::PRIMARY_COLOR; break;
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_PREVIOUS: GALxFPState::TextureUnit::CombineMode::PREVIOUS; break;
        }

        switch(parent->settings.textureStages[unit].combineSettings.srcALPHA[j])
        {
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_ZERO: tu.combineMode.srcALPHA[j] = GALxFPState::TextureUnit::CombineMode::ZERO; break;
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_ONE: tu.combineMode.srcALPHA[j] = GALxFPState::TextureUnit::CombineMode::ONE; break;
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_TEXTURE: tu.combineMode.srcALPHA[j] = GALxFPState::TextureUnit::CombineMode::TEXTURE; break;
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_TEXTUREn: tu.combineMode.srcALPHA[j] = GALxFPState::TextureUnit::CombineMode::TEXTUREn; break;
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_CONSTANT: tu.combineMode.srcALPHA[j] = GALxFPState::TextureUnit::CombineMode::CONSTANT; break;
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_PRIMARY_COLOR: tu.combineMode.srcALPHA[j] = GALxFPState::TextureUnit::CombineMode::PRIMARY_COLOR; break;
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_PREVIOUS: tu.combineMode.srcALPHA[j] = GALxFPState::TextureUnit::CombineMode::PREVIOUS; break;
        }

        switch(parent->settings.textureStages[unit].combineSettings.operandRGB[j])
        {
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_SRC_COLOR: tu.combineMode.operandRGB[j] = GALxFPState::TextureUnit::CombineMode::SRC_COLOR; break;
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_ONE_MINUS_SRC_COLOR: tu.combineMode.operandRGB[j] = GALxFPState::TextureUnit::CombineMode::ONE_MINUS_SRC_COLOR; break;
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_SRC_ALPHA: tu.combineMode.operandRGB[j] = GALxFPState::TextureUnit::CombineMode::SRC_ALPHA; break;
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_ONE_MINUS_SRC_ALPHA: tu.combineMode.operandRGB[j] = GALxFPState::TextureUnit::CombineMode::ONE_MINUS_SRC_ALPHA; break;
        }

        switch(parent->settings.textureStages[unit].combineSettings.operandALPHA[j])
        {
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_SRC_COLOR: tu.combineMode.operandALPHA[j] = GALxFPState::TextureUnit::CombineMode::SRC_COLOR; break;
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_ONE_MINUS_SRC_COLOR: tu.combineMode.operandALPHA[j] = GALxFPState::TextureUnit::CombineMode::ONE_MINUS_SRC_COLOR; break;
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_SRC_ALPHA: tu.combineMode.operandALPHA[j] = GALxFPState::TextureUnit::CombineMode::SRC_ALPHA; break;
            case GALx_COMBINE_SETTINGS::GALx_COMBINE_ONE_MINUS_SRC_ALPHA: tu.combineMode.operandALPHA[j] = GALxFPState::TextureUnit::CombineMode::ONE_MINUS_SRC_ALPHA; break;
        }

        tu.combineMode.srcRGB_texCrossbarReference[j]  = parent->settings.textureStages[unit].combineSettings.srcRGB_texCrossbarReference[j];
        tu.combineMode.srcALPHA_texCrossbarReference[j] = parent->settings.textureStages[unit].combineSettings.srcALPHA_texCrossbarReference[j];
    }    

    tu.combineMode.rgbScale = parent->settings.textureStages[unit].combineSettings.RGBScale;
    tu.combineMode.alphaScale = parent->settings.textureStages[unit].combineSettings.ALPHAScale;

    return tu;
}

bool FPSImp::fogEnabled()
{
    return (parent->settings.fogEnabled);
}

GALxFPState::FogMode FPSImp::fogMode()
{
    switch (parent->settings.fogMode)
    {
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_FOG_LINEAR: return GALxFPState::LINEAR;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_FOG_EXP: return GALxFPState::EXP;
        case GALx_FIXED_PIPELINE_SETTINGS::GALx_FOG_EXP2: return GALxFPState::EXP2;
        default:
            CG_ASSERT("Unsupported fog mode");
            return GALxFPState::EXP2; // to avoid stupid compiler warnings
    }
}
