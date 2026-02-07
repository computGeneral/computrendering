/**************************************************************************
 *
 */

#ifndef OGL_GL2GAL
    #define OGL_GL2GAL

#include "gl.h"
#include "glext.h"
#include "GALDevice.h"
#include "GALBlendingStage.h"
#include "GALRasterizationStage.h"
#include "GALZStencilStage.h"
#include "GALSampler.h"
#include "GALStream.h"

namespace ogl
{

    libGAL::GAL_BLEND_FUNCTION getGALBlendFunction(GLenum blendFunction);
    libGAL::GAL_BLEND_OPTION getGALBlendOption(GLenum blendOption);
    libGAL::GAL_CULL_MODE getGALCullMode(GLenum cullMode);
    libGAL::GAL_COMPARE_FUNCTION getGALDepthFunc(GLenum func);
    libGAL::GAL_TEXTURE_ADDR_MODE getGALTextureAddrMode(GLenum texAddrMode);
    libGAL::GAL_TEXTURE_FILTER getGALTextureFilter(GLenum texFilter);
    libGAL::GAL_COMPARE_FUNCTION getGALCompareFunc (GLenum compareFunc);
    libGAL::GAL_STENCIL_OP getGALStencilOp (GLenum texGenMode);
    libGAL::GAL_STREAM_DATA getGALStreamType (GLenum type, GLboolean normalized);
    libGAL::GAL_FORMAT getGALTextureFormat (GLenum galFormat);
    libGAL::GAL_CUBEMAP_FACE getGALCubeMapFace(GLenum cubeFace);
    libGAL::GAL_FACE getGALFace(GLenum face);
    GLuint getGALTexelSize(GLenum format);

}

#endif

