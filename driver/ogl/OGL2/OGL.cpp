/**************************************************************************
 *
 */

#include "OGL.h"
#include "support.h"
#include <iostream>
#include "GALMatrix.h"
#include <sstream>

// Define the global GLContext
ogl::GLContext* ogl::_ctx;

libGAL::GAL_PRIMITIVE ogl::trPrimitive(GLenum primitive)
{
    switch ( primitive ) {
        case GL_POINTS:
            return libGAL::GAL_POINTS;
        case GL_LINES:
            return libGAL::GAL_LINES;
        case GL_LINE_STRIP:
            return libGAL::GAL_LINE_STRIP;
        case GL_LINE_LOOP:
            return libGAL::GAL_LINE_LOOP;
        case GL_TRIANGLES:
            return libGAL::GAL_TRIANGLES;
        case GL_TRIANGLE_STRIP:
            return libGAL::GAL_TRIANGLE_STRIP;
        case GL_TRIANGLE_FAN:
            return libGAL::GAL_TRIANGLE_FAN;
        case GL_QUADS:
            return libGAL::GAL_QUADS;
        case GL_QUAD_STRIP:
            return libGAL::GAL_QUAD_STRIP;
        case GL_POLYGON:
            CG_ASSERT("CG1 does not support POLIGON PRIMITIVES directly (emulation not implemented)");
        default:
            CG_ASSERT("Other primitive than triangles not implemented yet");
    }
    return libGAL::GAL_TRIANGLES;

}

libGAL::GAL_INTERPOLATION_MODE ogl::trInterpolationMode(GLenum iMode)
{
    switch ( iMode ) {
        case GL_SMOOTH:
            return libGAL::GAL_INTERPOLATION_LINEAR;
        case GL_FLAT:
            return libGAL::GAL_INTERPOLATION_NONE;
        default:
            CG_ASSERT("Unknown OpenGL interpolation mode");
            return libGAL::GAL_INTERPOLATION_NONE;
    }
}

void ogl::initOGL(HAL* driver, libGAL::gal_uint startFrame)
{
    using namespace libGAL;

    libGAL::GALDevice* galDev = libGAL::createDevice(driver);

    galDev->setStartFrame(startFrame);

    _ctx = new GLContext(galDev);
    
    //cout << "*** initOGL completed ***" << endl;
}

void ogl::setResolution(libGAL::gal_uint width, libGAL::gal_uint height)
{
    _ctx->gal().setResolution(width, height);
}


void ogl::swapBuffers() { _ctx->gal().swapBuffers(); }

