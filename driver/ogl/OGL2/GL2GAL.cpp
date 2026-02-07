/**************************************************************************
 *
 */

#include "GL2GAL.h"
#include <stdio.h>

using namespace ogl;
using namespace libGAL;

GAL_TEXTURE_ADDR_MODE ogl::getGALTextureAddrMode(GLenum texAddrMode)
{
    switch (texAddrMode)
    {
        case (GL_CLAMP):
            return GAL_TEXTURE_ADDR_CLAMP;
            break;
        case (GL_CLAMP_TO_EDGE):
            return GAL_TEXTURE_ADDR_CLAMP_TO_EDGE;
            break;
        case (GL_REPEAT):
            return GAL_TEXTURE_ADDR_REPEAT;
            break;
        case (GL_CLAMP_TO_BORDER):
            return GAL_TEXTURE_ADDR_CLAMP_TO_BORDER;
            break;
        /*case ():
            return GAL_TEXTURE_ADDR_MIRRORED_REPEAT;
            break;*/
        default:
            CG_ASSERT("Filter type not found");
            return static_cast<GAL_TEXTURE_ADDR_MODE>(0);
    }
}

GAL_TEXTURE_FILTER ogl::getGALTextureFilter(GLenum texFilter)
{
    switch (texFilter)
    {
        case (GL_NEAREST):
            return GAL_TEXTURE_FILTER_NEAREST;
            break;
        case (GL_LINEAR):
            return GAL_TEXTURE_FILTER_LINEAR;
            break;
        case (GL_NEAREST_MIPMAP_NEAREST):
            return GAL_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST;
            break;
        case (GL_NEAREST_MIPMAP_LINEAR):
            return GAL_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR;
            break;
        case (GL_LINEAR_MIPMAP_NEAREST):
            return GAL_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST;
            break;
        case (GL_LINEAR_MIPMAP_LINEAR):
            return GAL_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR;
            break;
        default:
            CG_ASSERT("Filter type not found");
            return static_cast<GAL_TEXTURE_FILTER>(0);
    }
}

GAL_STREAM_DATA ogl::getGALStreamType (GLenum type, GLboolean normalized)
{
    if (normalized)
    {
        switch (type)
        {
            case GL_UNSIGNED_BYTE:
                    return GAL_SD_UNORM8;
                    break;

            case GL_UNSIGNED_SHORT:
                    return GAL_SD_UNORM16;
                    break;

            case GL_SHORT:
                    return GAL_SD_SNORM16;
                    break;

            case GL_UNSIGNED_INT:
                    return GAL_SD_UNORM32;
                    break;

            case GL_FLOAT:
                    return GAL_SD_FLOAT32;
                    break;

            default:
                CG_ASSERT("Stream Type not supported yet");
                return static_cast<GAL_STREAM_DATA>(0);
        }
    }
    else
    {
        switch (type)
        {
            case GL_UNSIGNED_BYTE:
                    return GAL_SD_UINT8;
                    break;

            case GL_UNSIGNED_SHORT:
                    return GAL_SD_UINT16;
                    break;

            case GL_SHORT:
                    return GAL_SD_SINT16;
                    break;

            case GL_UNSIGNED_INT:
                    return GAL_SD_UINT32;
                    break;

            case GL_FLOAT:
                    return GAL_SD_FLOAT32;
                    break;

            default:
                CG_ASSERT("Stream Type not supported yet");
                return static_cast<GAL_STREAM_DATA>(0);
        }
    }
}

libGAL::GAL_CULL_MODE ogl::getGALCullMode(GLenum cullMode)
{
    switch(cullMode)
    {
        case GL_FRONT:
            return GAL_CULL_FRONT;
        case GL_BACK:
            return GAL_CULL_BACK;
        case GL_FRONT_AND_BACK:
            return GAL_CULL_FRONT_AND_BACK;
        default:
            CG_ASSERT("Cull Mode not found");
            return static_cast<libGAL::GAL_CULL_MODE>(0);
   }
}


libGAL::GAL_COMPARE_FUNCTION ogl::getGALDepthFunc(GLenum func)
{
    switch (func)
    {
        case GL_NEVER:
            return libGAL::GAL_COMPARE_FUNCTION_NEVER;

        case GL_LESS:
            return GAL_COMPARE_FUNCTION_LESS;

        case GL_EQUAL:
            return GAL_COMPARE_FUNCTION_EQUAL;

        case GL_LEQUAL:
            return GAL_COMPARE_FUNCTION_LESS_EQUAL;

        case GL_GREATER:
            return GAL_COMPARE_FUNCTION_GREATER;

        case GL_NOTEQUAL:
            return GAL_COMPARE_FUNCTION_NOT_EQUAL;

        case GL_GEQUAL:
            return GAL_COMPARE_FUNCTION_GREATER_EQUAL;

        case GL_ALWAYS:
            return GAL_COMPARE_FUNCTION_ALWAYS;

        default:
            CG_ASSERT("function not found");
            return static_cast<libGAL::GAL_COMPARE_FUNCTION>(0);
    }
}


GAL_COMPARE_FUNCTION ogl::getGALCompareFunc (GLenum compareFunc)
{
    switch (compareFunc)
    {
        case GL_NEVER:
            return GAL_COMPARE_FUNCTION_NEVER;

        case GL_LESS:
            return GAL_COMPARE_FUNCTION_LESS;

        case GL_EQUAL:
            return GAL_COMPARE_FUNCTION_EQUAL;

        case GL_LEQUAL:
            return GAL_COMPARE_FUNCTION_LESS_EQUAL;

        case GL_GREATER:
            return GAL_COMPARE_FUNCTION_GREATER;

        case GL_NOTEQUAL:
            return GAL_COMPARE_FUNCTION_NOT_EQUAL;

        case GL_GEQUAL:
            return GAL_COMPARE_FUNCTION_GREATER_EQUAL;

        case GL_ALWAYS:
            return GAL_COMPARE_FUNCTION_ALWAYS;

        default:
            CG_ASSERT("Compare Function incorrect");
            return static_cast<libGAL::GAL_COMPARE_FUNCTION>(0);
    }
}

GAL_STENCIL_OP ogl::getGALStencilOp (GLenum stencilOp)
{
    switch(stencilOp)
    {
        case GL_KEEP :
            return GAL_STENCIL_OP_KEEP;

        case GL_ZERO :
            return GAL_STENCIL_OP_ZERO;

        case GL_REPLACE :
            return GAL_STENCIL_OP_REPLACE;

        case GL_INCR_WRAP :
            return GAL_STENCIL_OP_INCR_SAT;

        case GL_DECR_WRAP :
            return GAL_STENCIL_OP_DECR_SAT;

        case GL_INVERT :
            return GAL_STENCIL_OP_INVERT;

        case GL_INCR :
            return GAL_STENCIL_OP_INCR;

        case GL_DECR :
            return GAL_STENCIL_OP_DECR;

        default:
            CG_ASSERT("Stencil Op incorrect");
            return static_cast<GAL_STENCIL_OP>(0);
    }
}

GAL_BLEND_FUNCTION ogl::getGALBlendFunction(GLenum blendFunc)
{
    switch ( blendFunc )
    {
        case GL_FUNC_ADD:
            return GAL_BLEND_ADD;

        case GL_FUNC_SUBTRACT:
            return GAL_BLEND_SUBTRACT;

        case GL_FUNC_REVERSE_SUBTRACT:
            return GAL_BLEND_REVERSE_SUBTRACT;

        case GL_MIN:
            return GAL_BLEND_MIN;

        case GL_MAX:
            return GAL_BLEND_MAX;

        default:
            CG_ASSERT("Unexpected blend mode equation");
            return static_cast<GAL_BLEND_FUNCTION>(0);
    }
}

libGAL::GAL_BLEND_OPTION ogl::getGALBlendOption(GLenum blendOption)
{
    switch (blendOption)
    {
        case GL_ZERO:
            return GAL_BLEND_ZERO;
            break;
        case GL_ONE:
            return GAL_BLEND_ONE;
            break;
        case GL_SRC_COLOR:
            return GAL_BLEND_SRC_COLOR;
            break;
        case GL_ONE_MINUS_SRC_COLOR:
            return GAL_BLEND_INV_SRC_COLOR;
            break;
        case GL_SRC_ALPHA:
            return GAL_BLEND_SRC_ALPHA;
            break;
        case GL_ONE_MINUS_SRC_ALPHA:
            return GAL_BLEND_INV_SRC_ALPHA;
            break;
        case GL_DST_ALPHA:
            return GAL_BLEND_DEST_ALPHA;
            break;
        case GL_ONE_MINUS_DST_ALPHA:
            return GAL_BLEND_INV_DEST_ALPHA;
            break;
        case GL_DST_COLOR:
            return GAL_BLEND_DEST_COLOR;
            break;
        case GL_ONE_MINUS_DST_COLOR:
            return GAL_BLEND_INV_DEST_COLOR;
            break;
        case GL_SRC_ALPHA_SATURATE:
            return GAL_BLEND_SRC_ALPHA_SAT;
            break;
        /*case :
            return GAL_BLEND_BLEND_FACTOR;
            break;
        case :
            return GAL_BLEND_INV_BLEND_FACTOR;
            break;
        case :
            return GAL_BLEND_CONSTANT_COLOR;
            break;
        case :
            return GAL_BLEND_INV_CONSTANT_COLOR;
            break;
        case :
            return GAL_BLEND_CONSTANT_ALPHA;
            break;
        case :
            return GAL_BLEND_INV_CONSTANT_ALPHA;
            break;*/
        default:
            CG_ASSERT("Blend option not found");
            return static_cast<libGAL::GAL_BLEND_OPTION>(0);
    }

}

libGAL::GAL_FORMAT ogl::getGALTextureFormat (GLenum galFormat)
{
    switch (galFormat)
    {
        // Uncompressed formats
        case GL_RGB:
        case GL_RGB8:
            return GAL_FORMAT_RGB_888;
            break;

        case GL_RGBA:
        case GL_RGBA8:
            return GAL_FORMAT_RGBA_8888;
            break;

        case GL_UNSIGNED_INT_8_8_8_8:
        
            return GAL_FORMAT_UNSIGNED_INT_8888;
            break;
            
        case GL_INTENSITY:
        case GL_INTENSITY8:
            return GAL_FORMAT_INTENSITY_8;
            break;
        case GL_INTENSITY12:
            return GAL_FORMAT_INTENSITY_12;
            break;
        case GL_INTENSITY16:
            return GAL_FORMAT_INTENSITY_16;
            break;

        case GL_ALPHA:
        case GL_ALPHA8:
            return GAL_FORMAT_ALPHA_8;
            break;
        case GL_ALPHA12:
            return GAL_FORMAT_ALPHA_12;
            break;
        case GL_ALPHA16:
            return GAL_FORMAT_ALPHA_16;
            break;

        case GL_LUMINANCE:
        case GL_LUMINANCE8:
            return GAL_FORMAT_LUMINANCE_8;
            break;
        case GL_LUMINANCE12:
            return GAL_FORMAT_LUMINANCE_12;
            break;
        case GL_LUMINANCE16:
            return GAL_FORMAT_LUMINANCE_16;
            break;

        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
            return GAL_COMPRESSED_S3TC_DXT1_RGB;
            break;
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
            return GAL_COMPRESSED_S3TC_DXT1_RGBA;
            break;
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
            return GAL_COMPRESSED_S3TC_DXT3_RGBA;
            break;
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
            return GAL_COMPRESSED_S3TC_DXT5_RGBA;
            break;

        case GL_COMPRESSED_LUMINANCE_LATC1_EXT:
            return GAL_COMPRESSED_LUMINANCE_LATC1_EXT;
            break;

        case GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT:
            return GAL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT;
            break;

        case GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:
            return GAL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT;
            break;

        case GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT:
            return GAL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT;
            break;

        case GL_DEPTH_COMPONENT16:
            return GAL_FORMAT_DEPTH_COMPONENT_16;
            break;
        case GL_DEPTH_COMPONENT24:
            return GAL_FORMAT_DEPTH_COMPONENT_24;
            break;
        case GL_DEPTH_COMPONENT32:
            return GAL_FORMAT_DEPTH_COMPONENT_32;

            break;

        default:
            char error[128];
            sprintf(error, "GAL format unknown: %d", galFormat);
            CG_ASSERT(error);
            return GAL_FORMAT_UNKNOWN;
            break;
    }
}

libGAL::GAL_CUBEMAP_FACE ogl::getGALCubeMapFace(GLenum cubeFace)
{
    switch (cubeFace)
    {
        case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
            return GAL_CUBEMAP_POSITIVE_X;
            break;
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
            return GAL_CUBEMAP_NEGATIVE_X;
            break;
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
            return GAL_CUBEMAP_POSITIVE_y;
            break;
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
            return GAL_CUBEMAP_NEGATIVE_Y;
            break;
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
            return GAL_CUBEMAP_POSITIVE_Z;
            break;
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            return GAL_CUBEMAP_NEGATIVE_Z;
            break;
        default:
            char error[128];
            sprintf(error, "Cube map face not supported: %d", cubeFace);
            CG_ASSERT(error);
            return static_cast<libGAL::GAL_CUBEMAP_FACE>(0);
    }
}

libGAL::GAL_FACE ogl::getGALFace(GLenum face)
{
    switch(face)
    {
        case GL_NONE:
            return GAL_FACE_NONE;

        case GL_FRONT:
            return GAL_FACE_FRONT;

        case GL_BACK:
            return GAL_FACE_BACK;

        case GL_FRONT_AND_BACK:
            return GAL_FACE_FRONT_AND_BACK;
        
        default:
            char error[128];
            sprintf(error, "Face mode not supported: %d", face);
            CG_ASSERT(error);
            return static_cast<libGAL::GAL_FACE>(0);
    }
}

GLuint ogl::getGALTexelSize(GLenum format)
{
    switch ( format ) 
    {
        case GL_RGB8:
            return 3;
        case GL_RGBA:
        case GL_DEPTH_COMPONENT24:
            return 4;
        case GL_ALPHA:
        case GL_ALPHA8:
        case GL_LUMINANCE:
        case GL_LUMINANCE8:
            return 1;
        case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
            return 1;
        default:
            char error[128];
            sprintf(error, "Unknown format size: %d", format);
            CG_ASSERT(error);
            return 0;
    }
}

