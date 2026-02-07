/**************************************************************************
 *
 */

#include "GL2GALx.h"

using namespace ogl;
using namespace libGAL;

libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_TEXTURE_TARGET ogl::getGALxTextureTarget(GLenum target)
{
    switch (target)
    {
        case GL_TEXTURE_2D:
        case GL_TEXTURE_RECTANGLE:
            return libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_TEXTURE_2D;

        case GL_TEXTURE_3D:
            return libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_TEXTURE_3D;
            
        case GL_TEXTURE_CUBE_MAP:
            return libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_TEXTURE_CUBE_MAP;

        default:
            CG_ASSERT("texture target not found");
            return libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_TEXTURE_2D;
    }
}

GALx_FIXED_PIPELINE_SETTINGS::GALx_FOG_MODE ogl::getGALxFogMode(GLenum fogMode)
{
    switch (fogMode)
    {
        case GL_LINEAR:
            return GALx_FIXED_PIPELINE_SETTINGS::GALx_FOG_LINEAR;

        case GL_EXP:
            return GALx_FIXED_PIPELINE_SETTINGS::GALx_FOG_EXP;

        case GL_EXP2:
            return GALx_FIXED_PIPELINE_SETTINGS::GALx_FOG_EXP2;

        default:
            CG_ASSERT("Fog mode not supported");
            return GALx_FIXED_PIPELINE_SETTINGS::GALx_FOG_LINEAR;
    }
}

GALx_FIXED_PIPELINE_SETTINGS::GALx_FOG_COORDINATE_SOURCE ogl::getGALxFogCoordinateSource(GLenum fogCoordinate)
{

    switch (fogCoordinate)
    {
        case GL_FRAGMENT_DEPTH:
            return GALx_FIXED_PIPELINE_SETTINGS::GALx_FRAGMENT_DEPTH;

        case GL_FOG_COORD:
            return GALx_FIXED_PIPELINE_SETTINGS::GALx_FOG_COORD;

        default:
            CG_ASSERT("Fog Coordinate Source not found");
            return GALx_FIXED_PIPELINE_SETTINGS::GALx_FRAGMENT_DEPTH;;
    }
}

GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_TEXTURE_STAGE_FUNCTION ogl::getGALxTextureStageFunction (GLenum texFunction)
{

    switch (texFunction)
    {
        case GL_ADD:
            return GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_ADD;
            break;
        case GL_MODULATE:
            return GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_MODULATE;
            break;
        case GL_REPLACE:
            return GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_REPLACE;
            break;
        case GL_DECAL:
            return GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_DECAL;
            break;
        case GL_BLEND:
            return GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_BLEND;
            break;
        case GL_COMBINE:
            return GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_COMBINE;
            break;
        case GL_COMBINE4_NV:
            return GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_COMBINE_NV;
            break;
        default:
            CG_ASSERT("Texture Function not found");
            return GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_MODULATE;
    }
}

GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_BASE_INTERNAL_FORMAT ogl::getGALxTextureInternalFormat (GLenum textureInternalFormat)
{

    switch (textureInternalFormat)
    {
        case GL_ALPHA:
            return GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_ALPHA;
            break;
        case GL_LUMINANCE:
            return GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_LUMINANCE;
            break;
        case GL_LUMINANCE_ALPHA:
            return GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_LUMINANCE_ALPHA;
            break;
        case GL_DEPTH:
            return GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_DEPTH;
            break;
        case GL_INTENSITY:
            return GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_INTENSITY;
            break;
        case GL_RGB:
            return GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_RGB;
            break;
        case GL_RGBA:
            return GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_RGBA;
            break;
        default:
            CG_ASSERT("Texture internal Format not found");
            return GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_ALPHA;
    }
}

GALx_COMBINE_SETTINGS::GALx_COMBINE_FUNCTION ogl::getGALxCombinerFunction (GLenum combineFunction)
{

    switch (combineFunction)
    {
        case GL_REPLACE:
            return GALx_COMBINE_SETTINGS::GALx_COMBINE_REPLACE;
            break;
        case GL_MODULATE:
            return GALx_COMBINE_SETTINGS::GALx_COMBINE_MODULATE;
            break;
        case GL_ADD:
            return GALx_COMBINE_SETTINGS::GALx_COMBINE_ADD;
            break;
        case GL_ADD_SIGNED:
            return GALx_COMBINE_SETTINGS::GALx_COMBINE_ADD_SIGNED;
            break;
        case GL_INTERPOLATE:
            return GALx_COMBINE_SETTINGS::GALx_COMBINE_INTERPOLATE;
            break;
        /*case GL_SUBSTRACT:
            return GALx_COMBINE_SETTINGS::GALx_COMBINE_SUBSTRACT;
            break;*/
        case GL_DOT3_RGB:
            return GALx_COMBINE_SETTINGS::GALx_COMBINE_DOT3_RGB;
            break;
        case GL_DOT3_RGBA:
            return GALx_COMBINE_SETTINGS::GALx_COMBINE_DOT3_RGBA;
            break;
        /*case GL_MODULATE_ADD:
            return GALx_COMBINE_SETTINGS::GALx_COMBINE_MODULATE_ADD;
            break;
        case GL_MODULATE_SIGNED_ADD:
            return GALx_COMBINE_SETTINGS::GALx_COMBINE_MODULATE_SIGNED_ADD;
            break;
        case GL_MODULATE_SUBSTRACT:
            return GALx_COMBINE_SETTINGS::GALx_COMBINE_MODULATE_SUBTRACT;
            break;*/
        default:
            CG_ASSERT("Combine function not found");
            return GALx_COMBINE_SETTINGS::GALx_COMBINE_MODULATE_SUBTRACT;
    }
}

GALx_COMBINE_SETTINGS::GALx_COMBINE_SOURCE ogl::getGALxCombineSource (GLenum combineSource)
{

    /*switch (combineSource)
    {
        case GL_REPLACE:
            return GALx_COMBINE_SETTINGS::GALx_COMBINE_ZERO;
            break;
        case GL_REPLACE:
            return GALx_COMBINE_SETTINGS::GALx_COMBINE_ONE;
            break;
        case GL_REPLACE:
            return GALx_COMBINE_SETTINGS::GALx_COMBINE_TEXTURE;
            break;
        case GL_REPLACE:
            return GALx_COMBINE_SETTINGS::GALx_COMBINE_TEXTUREn;
            break;
        case GL_REPLACE:
            return GALx_COMBINE_SETTINGS::GALx_COMBINE_CONSTANT;
            break;
        case GL_REPLACE:
            return GALx_COMBINE_SETTINGS::GALx_COMBINE_PRIMARY_COLOR;
            break;
        case GL_REPLACE:
            return GALx_COMBINE_SETTINGS::GALx_COMBINE_PREVIOUS;
            break;
        default:
            CG_ASSERT("Combine source not found");
            return GALx_COMBINE_SETTINGS::GALx_COMBINE_ZERO;
    }*/
    return GALx_COMBINE_SETTINGS::GALx_COMBINE_ZERO;
}

GALx_COMBINE_SETTINGS::GALx_COMBINE_OPERAND ogl::getGALxCombineOperand (GLenum combineOperand)
{

    /*switch (combineOperand)
    {
        case GALx_COMBINE_SRC_COLOR:
            return GALx_COMBINE_SETTINGS::GALx_COMBINE_SRC_COLOR;
            break;
        case GALx_COMBINE_ONE_MINUS_SRC_COLOR:
            return GALx_COMBINE_SETTINGS::GALx_COMBINE_ONE_MINUS_SRC_COLOR;
            break;
        case GALx_COMBINE_SRC_ALPHA:
            return GALx_COMBINE_SETTINGS::GALx_COMBINE_SRC_ALPHA;
            break;
        case GALx_COMBINE_ONE_MINUS_SRC_ALPHA:
            return GALx_COMBINE_SETTINGS::GALx_COMBINE_ONE_MINUS_SRC_ALPHA;
            break;
        default:
            CG_ASSERT("Combine operand not found");
            return GALx_COMBINE_SETTINGS::GALx_COMBINE_SRC_COLOR;
    }*/
    return GALx_COMBINE_SETTINGS::GALx_COMBINE_SRC_COLOR;
}

GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_TEXTURE_COORDINATE_MODE ogl::getGALxTexGenMode (GLenum texGenMode)
{
    switch (texGenMode)
    {
        case GL_OBJECT_LINEAR:
            return GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_OBJECT_LINEAR;
            break;
        case GL_EYE_LINEAR:
            return GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_EYE_LINEAR;
            break;
        case GL_SPHERE_MAP:
            return GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_SPHERE_MAP;
            break;
        case GL_REFLECTION_MAP:
            return GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_REFLECTION_MAP;
            break;
        case GL_NORMAL_MAP:
            return GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_NORMAL_MAP;
            break;
        default:
            CG_ASSERT("Undefined texture gen mode");
            return static_cast<GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_TEXTURE_COORDINATE_MODE>(0);
    }
}


GALx_FIXED_PIPELINE_SETTINGS::GALx_COLOR_MATERIAL_ENABLED_FACE ogl::getGALxColorMaterialFace (GLenum face)
{
    switch (face)
    {
        case GL_FRONT:
            return libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_FRONT;

        case GL_BACK:
            return libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_BACK;

        case GL_FRONT_AND_BACK:
            return libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_FRONT_AND_BACK;

        default:
            CG_ASSERT("Unexpected face");
            return static_cast<GALx_FIXED_PIPELINE_SETTINGS::GALx_COLOR_MATERIAL_ENABLED_FACE>(0);
    }
}

GALx_FIXED_PIPELINE_SETTINGS::GALx_COLOR_MATERIAL_MODE ogl::getGALxColorMaterialMode (GLenum mode)
{

    switch (mode)
    {
        case GL_EMISSION:
            return libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_EMISSION;

        case GL_AMBIENT:
            return libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_AMBIENT;

        case GL_DIFFUSE:
            return libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_DIFFUSE;

        case GL_SPECULAR:
            return libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_SPECULAR;

        case GL_AMBIENT_AND_DIFFUSE:
            return libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_AMBIENT_AND_DIFFUSE;

        default:
            CG_ASSERT("Unexpected face");
            return static_cast<GALx_FIXED_PIPELINE_SETTINGS::GALx_COLOR_MATERIAL_MODE>(0);
    }
}

GALx_FIXED_PIPELINE_SETTINGS::GALx_ALPHA_TEST_FUNCTION ogl::getGALxAlphaFunc(GLenum func)
{
    switch (func)
    {
        case GL_NEVER:
            return libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_ALPHA_NEVER;

        case GL_ALWAYS:
            return libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_ALPHA_ALWAYS;

        case GL_LESS:
            return libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_ALPHA_LESS;

        case GL_LEQUAL:
            return libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_ALPHA_LEQUAL;

        case GL_EQUAL:
            return libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_ALPHA_EQUAL;

        case GL_GEQUAL:
            return libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_ALPHA_GEQUAL;

        case GL_GREATER:
            return libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_ALPHA_GREATER;

        case GL_NOTEQUAL:
            return libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_ALPHA_NOTEQUAL;

        default:
            CG_ASSERT("Unexpected face");
            return static_cast<GALx_FIXED_PIPELINE_SETTINGS::GALx_ALPHA_TEST_FUNCTION>(0);
    }
}

GALx_TEXTURE_COORD ogl::getGALxTexCoordPlane(GLenum texCoordPlane)
{
    switch (texCoordPlane)
    {
        case GL_S:
            return GALx_COORD_S;
            break;
        case GL_T:
            return GALx_COORD_T;
            break;
        case GL_R:
            return GALx_COORD_R;
            break;
        case GL_Q:
            return GALx_COORD_Q;
            break;
        default:
            CG_ASSERT("Unexpected plane coordinate");
            return static_cast<GALx_TEXTURE_COORD>(0);
    }
}

