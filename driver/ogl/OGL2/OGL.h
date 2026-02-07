/**************************************************************************
 *
 */

#ifndef OGL
    #define OGL

#include "gl.h"
#include "GAL.h"
#include "OGLContext.h"
#include "GALRasterizationStage.h"

namespace ogl
{
    // GL current context
    extern GLContext* _ctx;

    // Called by TraceDriverOGL to initialize OpenGL library build on top of GAL
    // The starting frame is passed through to allow the tracking of the
    // current frame number in the library.
    void initOGL(HAL* driver, libGAL::gal_uint startFrame);

    void setResolution(libGAL::gal_uint width, libGAL::gal_uint height);
    
    // Called by TraceDriverOGL to perform a swap buffers on the galDev attached to the current GLContext
    void swapBuffers();

    libGAL::GAL_PRIMITIVE trPrimitive(GLenum primitive);

    // Must be moved to a common GAL place
    libGAL::gal_ubyte trClamp(libGAL::gal_float v);

    libGAL::GAL_INTERPOLATION_MODE trInterpolationMode(GLenum iMode);

}

#endif // OGL
