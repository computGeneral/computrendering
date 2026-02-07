/**************************************************************************
 *
 */

#ifndef OGL_GL2GALx
    #define OGL_GL2GALx

#include "gl.h"
#include "glext.h"
#include "GALDevice.h"
#include "GALx.h"

namespace ogl
{

    libGAL::GALx_TEXTURE_COORD getGALxTexCoordPlane(GLenum texCoordPlane);
    libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_FOG_MODE getGALxFogMode(GLenum fogMode);
    libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_FOG_COORDINATE_SOURCE getGALxFogCoordinateSource(GLenum fogCoordinate);
    libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_TEXTURE_STAGE_FUNCTION getGALxTextureStageFunction (GLenum texFunction);
    libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_BASE_INTERNAL_FORMAT getGALxTextureInternalFormat (GLenum textureInternalFormat);
    libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_COORDINATE_SETTINGS::GALx_TEXTURE_COORDINATE_MODE getGALxTexGenMode (GLenum texGenMode);
    libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_COLOR_MATERIAL_ENABLED_FACE getGALxColorMaterialFace (GLenum face);
    libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_COLOR_MATERIAL_MODE getGALxColorMaterialMode (GLenum mode);
    libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_ALPHA_TEST_FUNCTION getGALxAlphaFunc(GLenum func);
    libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_TEXTURE_STAGE_SETTINGS::GALx_TEXTURE_TARGET getGALxTextureTarget(GLenum target);
    libGAL::GALx_COMBINE_SETTINGS::GALx_COMBINE_FUNCTION getGALxCombinerFunction (GLenum combineFunction);
    libGAL::GALx_COMBINE_SETTINGS::GALx_COMBINE_SOURCE getGALxCombineSource (GLenum combineSource);
    libGAL::GALx_COMBINE_SETTINGS::GALx_COMBINE_OPERAND getGALxCombineOperand (GLenum combineOperand);

}

#endif

