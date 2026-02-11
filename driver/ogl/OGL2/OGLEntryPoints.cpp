/**************************************************************************
 *
 */

#include "OGLEntryPoints.h"
#include "GALDevice.h"
#include "OGL.h"
#include "GALRasterizationStage.h"
#include "GALZStencilStage.h"
#include "GALBuffer.h"
#include "GL2GAL.h"
#include "GL2GALx.h"

#include "support.h"

#include <sstream>
#include <iostream>
#include <stdio.h>
#include <string>

#include "GALMatrix.h"

#include "Profiler.h"

using std::cout;
using std::hex;
using std::dec;
using std::endl;

using namespace libGAL;
using namespace ogl;

/*
#define OGL_PRINTLN(str) { cout << str << endl; }
#define OGL_PRINT(str) { cout << str; cout.flush(); }
#define OGL_PRINTFUNC { cout << __FUNCTION__ << endl; }
*/

//#define ENDPRINTCALL return ; // Only print the call and exit
#define ENDPRINTCALL {}
//#define GAL_STATS

//#define _PRINTCALLS_

#ifdef _PRINTCALLS_
    #define OGL_PRINTCALL { cout << __FUNCTION__ << "()" << endl; }
    #define OGL_PRINTCALL_1(a) { printCall(__FUNCTION__, a); ENDPRINTCALL }
    #define OGL_PRINTCALL_2(a1,a2) { printCall(__FUNCTION__, a1, a2); ENDPRINTCALL }
    #define OGL_PRINTCALL_3(a1,a2,a3) { printCall(__FUNCTION__, a1, a2, a3); ENDPRINTCALL }
    #define OGL_PRINTCALL_4(a1,a2,a3,a4) { printCall(__FUNCTION__,a1,a2,a3,a4); ENDPRINTCALL }
    #define OGL_PRINTCALL_6(a1,a2,a3,a4,a5,a6) { printCall(__FUNCTION__,a1,a2,a3,a4,a5,a6); ENDPRINTCALL }
    #define OGL_PRINTCALL_8(a1,a2,a3,a4,a5,a6,a7,a8) { printCall(__FUNCTION__,a1,a2,a3,a4,a5,a6,a7,a8); ENDPRINTCALL }
    #define OGL_PRINTCALL_9(a1,a2,a3,a4,a5,a6,a7,a8,a9) { printCall(__FUNCTION__,a1,a2,a3,a4,a5,a6,a7,a8,a9); ENDPRINTCALL }
#else
    #define OGL_PRINTCALL { ENDPRINTCALL }
    #define OGL_PRINTCALL_1(a) { ENDPRINTCALL }
    #define OGL_PRINTCALL_2(a1,a2) { ENDPRINTCALL }
    #define OGL_PRINTCALL_3(a1,a2,a3) { ENDPRINTCALL }
    #define OGL_PRINTCALL_4(a1,a2,a3,a4) { ENDPRINTCALL }
    #define OGL_PRINTCALL_6(a1,a2,a3,a4,a5,a6) { ENDPRINTCALL }
    #define OGL_PRINTCALL_8(a1,a2,a3,a4,a5,a6,a7,a8) { ENDPRINTCALL }
    #define OGL_PRINTCALL_9(a1,a2,a3,a4,a5,a6,a7,a8,a9) { ENDPRINTCALL }
#endif

template<class T>
std::string arrayParam(const T* arrayVar, gal_uint arrayCount)
{
    stringstream ss;
    ss << "{";
    gal_uint i;
    for ( i = 0; i < arrayCount - 1; ++i ) {
        ss << arrayVar[i] << ",";
    }
    ss << arrayVar[i] << "}";

    return ss.str();
}

template<class T>
void printCall(const gal_char* funcName, const T arg)
{
    cout << funcName << "(" << arg << ")" << endl;
}

template<class T1, class T2>
void printCall(const gal_char* funcName, const T1 arg1, const T2 arg2)
{
    cout << funcName << "(" << arg1 << "," << arg2 << ")" << endl;
}


template<class T1, class T2, class T3>
void printCall(const gal_char* funcName, const T1 arg1, const T2 arg2, const T3 arg3)
{
    cout << funcName << "(" << arg1 << "," << arg2 << "," << arg3 << ")" << endl;
}

template<class T1, class T2, class T3, class T4>
void printCall(const gal_char* funcName, const T1 arg1, const T2 arg2, const T3 arg3, const T4 arg4)
{
    cout << funcName << "(" << arg1 << "," << arg2 << "," << arg3 << "," << arg4 << ")" << endl;
}

template<class T1, class T2, class T3, class T4, class T5, class T6>
void printCall(const gal_char* funcName, const T1 arg1, const T2 arg2, const T3 arg3, const T4 arg4,
               const T5 arg5, const T6 arg6)
{
    cout << funcName << "(" << arg1 << "," << arg2 << "," << arg3 << "," << arg4
                     << "," << arg5 << "," << arg6 << ")" << endl;
}

template<class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8>
void printCall(const gal_char* funcName, const T1 arg1, const T2 arg2, const T3 arg3, const T4 arg4,
               const T5 arg5, const T6 arg6, const T7 arg7, const T8 arg8)
{
    cout << funcName << "(" << arg1 << "," << arg2 << "," << arg3 << "," << arg4
                     << "," << arg5 << "," << arg6 << "," << arg7 << "," << arg8 << ")" << endl;
}

template<class T1, class T2, class T3, class T4, class T5, class T6, class T7, class T8, class T9>
void printCall(const gal_char* funcName, const T1 arg1, const T2 arg2, const T3 arg3, const T4 arg4,
               const T5 arg5, const T6 arg6, const T7 arg7, const T8 arg8, const T9 arg9)
{
    cout << funcName << "(" << arg1 << "," << arg2 << "," << arg3 << "," << arg4
                     << "," << arg5 << "," << arg6 << "," << arg7 << "," << arg8
                     << "," << arg9 << ")" << endl;
}

GLTextureObject& getTextureObject (GLenum target)
{
    switch ( target )
    {
        case GL_TEXTURE_2D:
            target = GL_TEXTURE_2D;
            break;
        case GL_TEXTURE_3D:
            target = GL_TEXTURE_3D;
            break;
        case GL_TEXTURE_RECTANGLE:
            target = GL_TEXTURE_RECTANGLE;
            break;
        case GL_TEXTURE_CUBE_MAP_POSITIVE_X:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_X:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Y:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Y:
        case GL_TEXTURE_CUBE_MAP_POSITIVE_Z:
        case GL_TEXTURE_CUBE_MAP_NEGATIVE_Z:
            target = GL_TEXTURE_CUBE_MAP;
            break;
        default:
            CG_ASSERT("only TEXTURE_2D or CUBEMAP faces are allowed as target");
    }

    // Get current texture object
    GLTextureManager& texTM = _ctx->textureManager();
    return texTM.getTarget(target).getCurrent();

}

/***********************
 * OpenGL entry points *
 ***********************/

GLAPI void GLAPIENTRY OGL_glBegin( GLenum primitive )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_1(primitive);

    _ctx->gal().setPrimitive( ogl::trPrimitive(primitive) );

    _ctx->initInternalBuffers(true); // previous buffer contents can be discarded
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glBindProgramARB (GLenum target, GLuint pid)
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_2(target, pid);

    ARBProgramManager& arbPM = _ctx->arbProgramManager();
    if ( pid == 0 )
        arbPM.target(target).setDefaultAsCurrent();
    else 
    {
        // Get/Create the program with pid 'pid'
        ARBProgramObject& arbp = static_cast<ARBProgramObject&>(arbPM.bindObject(target, pid));
        arbp.attachDevice(&_ctx->gal());
    }
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glClear( GLbitfield mask )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_1(mask);

    gal_bool clearColorBuffer = ((mask & GL_COLOR_BUFFER_BIT) == GL_COLOR_BUFFER_BIT);
    gal_bool clearZ = ((mask & GL_DEPTH_BUFFER_BIT) == GL_DEPTH_BUFFER_BIT);
    gal_bool clearStencil = ((mask & GL_STENCIL_BUFFER_BIT) == GL_STENCIL_BUFFER_BIT);

    GALDevice& galDev = _ctx->gal();

    if ( clearColorBuffer ) 
    {
        gal_ubyte r, g, b, a;
        _ctx->getClearColor(r,g,b,a);
        galDev.clearColorBuffer(r,g,b,a);
    }

    if ( clearZ || clearStencil ) 
    {
        gal_float zValue = _ctx->getDepthClearValue();
        gal_int stencilValue = _ctx->getStencilClearValue();
        galDev.clearZStencilBuffer(clearZ, clearStencil, zValue, stencilValue);
    }
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glClearColor( GLclampf red, GLclampf green,
                                        GLclampf blue, GLclampf alpha )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_4(red,green,blue,alpha);

    _ctx->setClearColor(red, green, blue, alpha);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glClearDepth( GLclampd depthValue )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_1(depthValue);

    _ctx->setDepthClearValue(depthValue);

    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glColor3f( GLfloat red, GLfloat green, GLfloat blue )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_3(red, green, blue);

    _ctx->setColor(red, green, blue);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glColor4f( GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_4(red, green, blue, alpha);

    _ctx->setColor(red, green, blue, alpha);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glCullFace( GLenum mode )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_1(mode);

    GAL_CULL_MODE GALmode = ogl::getGALCullMode(mode);
    _ctx->gal().rast().setCullMode (GALmode);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glDepthFunc (GLenum func)
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_1(func);

    _ctx->gal().zStencil().setZFunc(ogl::getGALDepthFunc(func));
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glDisable( GLenum cap )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_1(cap);

    switch ( cap ) {
        case GL_DEPTH_TEST:
            _ctx->gal().zStencil().setZEnabled(false);
            break;
        case GL_LIGHTING:
            _ctx->fixedPipelineSettings().lightingEnabled = false;
            break;
        case GL_LIGHT0:
        case GL_LIGHT1:
        case GL_LIGHT2:
        case GL_LIGHT3:
        case GL_LIGHT4:
        case GL_LIGHT5:
        case GL_LIGHT6:
        case GL_LIGHT7:
        {
            gal_uint light = cap - GL_LIGHT0;
            _ctx->fixedPipelineSettings().lights[light].enabled = false;
            break;
        }
        case GL_VERTEX_PROGRAM_ARB:
            _ctx->setRenderState(RS_VERTEX_PROGRAM, false); // Disable user vertex program
            break;
        case GL_FRAGMENT_PROGRAM_ARB:
            _ctx->setRenderState(RS_FRAGMENT_PROGRAM, false); // Disable user fragment program
            break;
        case GL_TEXTURE_1D:
        case GL_TEXTURE_2D:
        case GL_TEXTURE_3D:
        case GL_TEXTURE_CUBE_MAP:
        case GL_TEXTURE_RECTANGLE:
            {
                GLTextureManager& texTM = _ctx->textureManager();
                GLuint tu = texTM.getCurrentGroup(); // get active texture unit

                TextureUnit& texUnit = _ctx->getTextureUnit(tu);

                texUnit.disableTarget(cap); // disable target in the active texture unit
                GLuint texUnitNum = _ctx->getActiveTextureUnitNumber();
                //  Check if all targets have been disabled for the active texture unit.
                if (!texUnit.isTextureUnitActive())
                    _ctx->fixedPipelineSettings().textureStages[texUnitNum].enabled = false;
            }
            break;
        case GL_TEXTURE_GEN_S:
        case GL_TEXTURE_GEN_T:
        case GL_TEXTURE_GEN_R:
        case GL_TEXTURE_GEN_Q:
            {
                TextureUnit& texUnit = _ctx->getActiveTextureUnit();
                GLenum select = 0;

                if ( cap == GL_TEXTURE_GEN_S )
                    select = GL_S;
                else if ( cap == GL_TEXTURE_GEN_T )
                    select = GL_T;
                else if ( cap == GL_TEXTURE_GEN_R )
                    select = GL_R;
                else if ( cap == GL_TEXTURE_GEN_Q )
                    select = GL_Q;
                if ( texUnit.isEnabledTexGen(select) )
                    texUnit.setEnabledTexGen(select, false);
            }
        case GL_FOG:
            _ctx->fixedPipelineSettings().fogEnabled = false;
            break;
        case GL_BLEND:
            _ctx->gal().blending().setEnable(0,false);
            break;
        case GL_CULL_FACE:
            _ctx->gal().rast().setCullMode(GAL_CULL_NONE);
            break;
        case GL_NORMALIZE:
            _ctx->fixedPipelineSettings().normalizeNormals = libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_NO_NORMALIZE;
            break;
        case GL_STENCIL_TEST:
            _ctx->gal().zStencil().setStencilEnabled(false);
            break;
        case GL_ALPHA_TEST:
            _ctx->fixedPipelineSettings().alphaTestEnabled = false;
            _ctx->gal().alphaTestEnabled(false);
            break;
        case GL_COLOR_MATERIAL:
            _ctx->fixedPipelineSettings().colorMaterialEnabledFace = libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_NONE;
            break;
        case GL_SCISSOR_TEST:
            _ctx->gal().rast().enableScissor(false);
            break;
        case GL_POLYGON_OFFSET_FILL:
                _ctx->gal().zStencil().setPolygonOffset(0.0, 0.0);
            break;
        default:
            break;
            //CG_ASSERT("Unknown glDisable parameter");
    }
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glEnable( GLenum cap )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_1(cap);

    switch ( cap ) {
        case GL_DEPTH_TEST:
            _ctx->gal().zStencil().setZEnabled(true);
            break;
        case GL_LIGHTING:
            _ctx->fixedPipelineSettings().lightingEnabled = true;
            break;
        case GL_LIGHT0:
        case GL_LIGHT1:
        case GL_LIGHT2:
        case GL_LIGHT3:
        case GL_LIGHT4:
        case GL_LIGHT5:
        case GL_LIGHT6:
        case GL_LIGHT7:
        {
            gal_uint light = cap - GL_LIGHT0;
            _ctx->fixedPipelineSettings().lights[light].enabled = true;
            break;
        }
        case GL_VERTEX_PROGRAM_ARB:
            _ctx->setRenderState(RS_VERTEX_PROGRAM, true); // Enable user vertex program
            break;
        case GL_FRAGMENT_PROGRAM_ARB:
            _ctx->setRenderState(RS_FRAGMENT_PROGRAM, true); // Enable user fragment program
            break;
        case GL_CULL_FACE:
            _ctx->gal().rast().setCullMode(GAL_CULL_BACK);
            break;
        case GL_TEXTURE_1D:
        case GL_TEXTURE_2D:
        case GL_TEXTURE_3D:
        case GL_TEXTURE_CUBE_MAP:
            {
                TextureUnit& texUnit = _ctx->getActiveTextureUnit();
                GLTextureManager& texTM = _ctx->textureManager();
                GLTextureObject& to = texTM.getTarget(cap).getCurrent();
                texUnit.enableTarget(cap); // enable target in the active texture unit
                texUnit.setTextureObject(cap, &to);
                GLuint texUnitNum = _ctx->getActiveTextureUnitNumber();
                _ctx->fixedPipelineSettings().textureStages[texUnitNum].enabled = true;
                _ctx->fixedPipelineSettings().textureStages[texUnitNum].activeTextureTarget = ogl::getGALxTextureTarget(cap);
            }
            break;
        case GL_TEXTURE_GEN_S:
        case GL_TEXTURE_GEN_T:
        case GL_TEXTURE_GEN_R:
        case GL_TEXTURE_GEN_Q:
            {
                TextureUnit& texUnit = _ctx->getActiveTextureUnit();
                GLenum select = 0;

                if ( cap == GL_TEXTURE_GEN_S )
                    select = GL_S;
                else if ( cap == GL_TEXTURE_GEN_T )
                    select = GL_T;
                else if ( cap == GL_TEXTURE_GEN_R )
                    select = GL_R;
                else if ( cap == GL_TEXTURE_GEN_Q )
                    select = GL_Q;
                if ( !texUnit.isEnabledTexGen(select) )
                    texUnit.setEnabledTexGen(select, true);
            }
            break;
        case GL_FOG:
            _ctx->fixedPipelineSettings().fogEnabled = true;
            break;
        case GL_BLEND:
            _ctx->gal().blending().setEnable(0,true);
            break;
        case GL_NORMALIZE:
            _ctx->fixedPipelineSettings().normalizeNormals = libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_NORMALIZE;
            break;
        case GL_STENCIL_TEST:
            _ctx->gal().zStencil().setStencilEnabled(true);
            break;
        case GL_ALPHA_TEST:
            _ctx->fixedPipelineSettings().alphaTestEnabled = true;
            _ctx->gal().alphaTestEnabled(true);
            break;
        case GL_COLOR_MATERIAL:
            _ctx->fixedPipelineSettings().colorMaterialEnabledFace = libGAL::GALx_FIXED_PIPELINE_SETTINGS::GALx_CM_FRONT;
            break;
        case GL_SCISSOR_TEST:
            _ctx->gal().rast().enableScissor(true);
            break;
        case GL_POLYGON_OFFSET_FILL:
            // Nothing to be done
            break;
        default:
            break;
            //CG_ASSERT("Unknown glEnable parameter");
    }
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glEnableClientState (GLenum cap)
{
    TRACING_ENTER_REGION("OGL2", "", "")

    OGL_PRINTCALL_1(cap);

    switch ( cap )
    {
        case GL_VERTEX_ARRAY:
        case GL_COLOR_ARRAY:
        case GL_NORMAL_ARRAY:
        case GL_INDEX_ARRAY:
        case GL_TEXTURE_COORD_ARRAY:
        case GL_EDGE_FLAG_ARRAY:
            _ctx->enableBufferArray (cap);
            break;
        default:
            CG_ASSERT("Unexpected client state");
    }
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glDisableClientState (GLenum cap)
{
    TRACING_ENTER_REGION("OGL2", "", "")

    OGL_PRINTCALL_1(cap);

    switch ( cap )
    {
        case GL_VERTEX_ARRAY:
        case GL_COLOR_ARRAY:
        case GL_NORMAL_ARRAY:
        case GL_INDEX_ARRAY:
        case GL_TEXTURE_COORD_ARRAY:
        case GL_EDGE_FLAG_ARRAY:
            _ctx->disableBufferArray (cap);
            break;
        default:
            CG_ASSERT("Unexpected client state");
    }
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glEnd( void )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL;

    // Sets the proper shader (user/auto-generated) and update GPU shader constants
    _ctx->setShaders();

    // Use internal buffers to draw
    _ctx->attachInternalBuffers(); // Attach internal buffers to streams

    _ctx->attachInternalSamplers();

    // Perform the draw using the internal buffers as input
    _ctx->gal().draw(0, _ctx->countInternalVertexes());

#ifdef GAL_STATS
    _ctx->gal().DBG_dump("galdump.txt", 0);
    _ctx->fixedPipelineState().DBG_dump("GALxjustbeforedraw.txt",0);
#endif

    _ctx->deattachInternalSamplers();

    _ctx->deattachInternalBuffers();
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glFlush( void )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL;

    // nothing to be done
    TRACING_EXIT_REGION()
}


GLAPI void GLAPIENTRY OGL_glLightfv( GLenum light, GLenum pname, const GLfloat *params )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_3(light,pname,params);

    _ctx->setLightParam(light, pname, params);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glLightf( GLenum light, GLenum pname, const GLfloat params )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_3(light,pname,params);

    _ctx->setLightParam(light, pname, &params);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glLightModelfv( GLenum pname, const GLfloat *params )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_2(pname,params);

    _ctx->setLightModel(pname, params);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glLightModelf( GLenum pname, const GLfloat param )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_2(pname,param);

    _ctx->setLightModel(pname, &param);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glLightModeli( GLenum pname, const GLint param )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_2(pname,param);

    GLfloat _param = static_cast<GLfloat>(param);
    _ctx->setLightModel(pname, &_param);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glLoadIdentity( void )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL;

    // Create identity matrix
    GALxFloatMatrix4x4 identity;
    libGAL::identityMatrix(identity);

    // Load the identity matrix in the current active stack/unit
    _ctx->matrixStack().set(identity);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glMaterialfv( GLenum face, GLenum pname, const GLfloat *params )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_3(face,pname,params);

    _ctx->setMaterialParam(face, pname, params);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glMaterialf( GLenum face, GLenum pname, const GLfloat params )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_3(face,pname,params);

    _ctx->setMaterialParam(face, pname, &params);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glMatrixMode( GLenum mode )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_1(mode);

    // Set active modelview decoding the GLenum token if required
    if ( mode == GL_MODELVIEW )
        _ctx->setActiveModelview(0);
    else if ( GL_MODELVIEW1_ARB <= mode && mode <= GL_MODELVIEW31_ARB ) {
        mode = GL_MODELVIEW;
        _ctx->setActiveModelview(GL_MODELVIEW31_ARB - mode + 1);
    }

    _ctx->setMatrixMode(mode);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glMultMatrixd( const GLdouble *m )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_1(arrayParam(m,16));

    // Convert doubles to floats
    GLfloat floatData[16];
    for ( gal_uint i = 0; i < 16; ++i )
        floatData[i] = static_cast<GLfloat>(m[i]);

    GALxFloatMatrix4x4 mat(floatData, false); // Set row-major to false, matrix is expressed in major-column

    _ctx->matrixStack().multiply(mat);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glNormal3f( GLfloat nx, GLfloat ny, GLfloat nz )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_3(nx, ny, nz);

    _ctx->setNormal(nx, ny, nz);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glNormal3fv( const GLfloat *v )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_1(arrayParam(v,3));

    _ctx->setNormal(v[0], v[1], v[2]);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glOrtho( GLdouble left, GLdouble right,
                                 GLdouble bottom, GLdouble top,
                                 GLdouble near_, GLdouble far_ )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_6(left, right, bottom, top, near_, far_);

    GALxFloatMatrix4x4 mat;

    mat(0,0) = 2.0F / static_cast<gal_float>(right-left);
    mat(1,0) = 0.0F;
    mat(2,0) = 0.0F;
    mat(3,0) = 0.0F;

    mat(0,1) = 0.0F;
    mat(1,1) = 2.0F / static_cast<gal_float>(top-bottom);
    mat(2,1) = 0.0F;
    mat(3,1) = 0.0F;

    mat(0,2) = 0.0F;
    mat(1,2) = 0.0F;
    mat(2,2) = -2.0F / static_cast<gal_float>(far_-near_);
    mat(3,2) = 0.0F;

    mat(0,3) = static_cast<gal_float>(-(right+left) / (right-left));
    mat(1,3) = static_cast<gal_float>(-(top+bottom) / (top-bottom));
    mat(2,3) = static_cast<gal_float>(-(far_+near_) / (far_-near_));
    mat(3,3) = 1.0F;

    // Multiply the current matrix by 'mat'
    _ctx->matrixStack().multiply(mat);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glFrustum( GLdouble left, GLdouble right,
                                    GLdouble bottom, GLdouble top,
                                    GLdouble near_, GLdouble far_ )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_6(left, right, bottom, top, near_, far_);

    GALxFloatMatrix4x4 mat;

    mat(0,0) = (2.0F*static_cast<gal_float>(near_)) / static_cast<gal_float>(right-left);
    mat(1,0) = 0.0F;
    mat(2,0) = 0.0F;
    mat(3,0) = 0.0F;

    mat(0,1) = 0.0F;
    mat(1,1) = (2.0F*static_cast<gal_float>(near_)) / static_cast<gal_float>(top-bottom);
    mat(2,1) = 0.0F;
    mat(3,1) = 0.0F;

    mat(0,2) = static_cast<gal_float>((right+left) / (right-left));
    mat(1,2) = static_cast<gal_float>((top+bottom) / (top-bottom));
    mat(2,2) = static_cast<gal_float>(-(far_+near_) / (far_-near_));
    mat(3,2) = -1.0F;

    mat(0,3) = 0.0F;
    mat(1,3) = 0.0F;
    mat(2,3) = static_cast<gal_float>(-(2*far_*near_) / (far_-near_));
    mat(3,3) = 0.0F;

    // Multiply the current matrix by 'mat'
    _ctx->matrixStack().multiply(mat);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glPopMatrix( void )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL;

    _ctx->matrixStack().pop();
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glProgramLocalParameter4fARB (GLenum target, GLuint index,
                                                  GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_6(target,index,x,y,z,w);

    // Get the local parameter bank of the current
    ARBRegisterBank& locals = _ctx->arbProgramManager().target(target).getCurrent().getLocals();

    // Update local parameter bank
    locals.set(index, x, y, z, w);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glProgramLocalParameters4fvEXT (GLenum target, GLuint index, GLsizei count, const GLfloat* d)
{
    TRACING_ENTER_REGION("OGL2", "", "")
    //OGL_PRINTCALL_6(target,index,x,y,z,w);

    // Get the local parameter bank of the current
    ARBRegisterBank& locals = _ctx->arbProgramManager().target(target).getCurrent().getLocals();

    // Update local parameter bank
    for (gal_uint i = 0; i < count ; i++)
        locals.set(index + i, d[i*4], d[i*4+1], d[i*4+2], d[i*4+3]);

    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glProgramLocalParameter4fvARB (GLenum target, GLuint index, const GLfloat *v)
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_3(target, index, v);

    // Get the local parameter bank of the current
    ARBRegisterBank& locals = _ctx->arbProgramManager().target(target).getCurrent().getLocals();

    // Update local parameter bank
    locals.set(index,v[0], v[1], v[2], v[3]);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glProgramStringARB (GLenum target, GLenum format, GLsizei len, const GLvoid * str)
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_4(target, format, len, str);

    _ctx->arbProgramManager().programString(target, format, len, str);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glPushMatrix( void )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL;

    _ctx->matrixStack().push();
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glRotatef( GLfloat angle, GLfloat x, GLfloat y, GLfloat z )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_4(angle, x, y, z);

    GALFloatMatrix4x4 m = _ctx->matrixStack().get();

    libGAL::_rotate(m, angle, x, y, z);

    _ctx->matrixStack().set(m);

    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glRotated( GLdouble angle, GLdouble x, GLdouble y, GLdouble z )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_4(angle, x, y, z);

    GALFloatMatrix4x4 m = _ctx->matrixStack().get();

    libGAL::_rotate(m, static_cast<gal_float> (angle), static_cast<gal_float> (x), static_cast<gal_float> (y), static_cast<gal_float> (z));

    _ctx->matrixStack().set(m);

    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glScalef( GLfloat x, GLfloat y, GLfloat z)
{
    OGL_PRINTCALL_3(x, y, z);

    GALFloatMatrix4x4 m = _ctx->matrixStack().get();

    libGAL::_scale(m, static_cast<gal_float> (x), static_cast<gal_float> (y), static_cast<gal_float> (z));

    _ctx->matrixStack().set(m);
}

GLAPI void GLAPIENTRY OGL_glShadeModel( GLenum mode )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_1(mode);

    // CG1 Rasterizer uses fshInputAttribute[1] to store the fragment color
    _ctx->gal().rast().setInterpolationMode(1, ogl::trInterpolationMode(mode));
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glTranslatef( GLfloat x, GLfloat y, GLfloat z )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_3(x,y,z);

    GALFloatMatrix4x4 m = _ctx->matrixStack().get();

    libGAL::_translate(m, x, y, z);

    _ctx->matrixStack().set(m);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glTranslated( GLdouble x, GLdouble y, GLdouble z )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_3(x,y,z);

    GALFloatMatrix4x4 m = _ctx->matrixStack().get();

    libGAL::_translate(m,  static_cast<gal_float> (x),  static_cast<gal_float> (y),  static_cast<gal_float> (z));

    _ctx->matrixStack().set(m);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glVertex3f( GLfloat x, GLfloat y, GLfloat z )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_3(x, y, z);

    _ctx->addVertex(x, y, z);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glVertex2f( GLfloat x, GLfloat y)
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_2(x, y);

    _ctx->addVertex(x, y, 0);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glVertex2i ( GLint x, GLint y)
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_2(x, y);

    _ctx->addVertex(static_cast<GLfloat>(x), static_cast<GLfloat>(y), 0);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glVertex3fv( const GLfloat *v )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_1(arrayParam(v,3));

    _ctx->addVertex(v[0], v[1], v[2]);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glViewport( GLint x, GLint y, GLsizei width, GLsizei height )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_4(x,y,width,height);

    GALDevice& galDev = _ctx->gal();

    galDev.rast().setViewport(x, y, width, height);

    // PATCH for old traces which do not contain the window resolution
    // Use the viewport as current window resolution
    gal_uint w, h;
    if ( !galDev.getResolution(w,h) ) // Resolution not defined yet (backwards compatibility)
        galDev.setResolution(static_cast<gal_uint>(width), static_cast<gal_uint>(height));
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glVertexPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *ptr )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_4(size,type,stride,ptr);

    BufferManager& bufBM = _ctx->bufferManager();

    _ctx->getPosVArray() = GLContext::VArray(size, type, stride, ptr, true);

    if (bufBM.getTarget(GL_ARRAY_BUFFER).hasCurrent())
        _ctx->getPosVArray().bufferID = bufBM.getTarget(GL_ARRAY_BUFFER).getCurrent().getName();
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glColorPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *ptr )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_4(size,type,stride,ptr);

    BufferManager& bufBM = _ctx->bufferManager();

    _ctx->getColorVArray() = GLContext::VArray(size, type, stride, ptr, false);

    if (bufBM.getTarget(GL_ARRAY_BUFFER).hasCurrent())
        _ctx->getColorVArray().bufferID = bufBM.getTarget(GL_ARRAY_BUFFER).getCurrent().getName();
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glNormalPointer( GLenum type, GLsizei stride, const GLvoid *ptr )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_3(type,stride,ptr);

    BufferManager& bufBM = _ctx->bufferManager();

    _ctx->getNormalVArray() = GLContext::VArray(3, type, stride, ptr, true);

      if (bufBM.getTarget(GL_ARRAY_BUFFER).hasCurrent())
        _ctx->getNormalVArray().bufferID = bufBM.getTarget(GL_ARRAY_BUFFER).getCurrent().getName();
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glDrawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices )
{
    TRACING_ENTER_REGION("glDraw", "", "")
    OGL_PRINTCALL_4(mode,count,type,indices);

    BufferManager& bufBM = _ctx->bufferManager();

    _ctx->initInternalBuffers(false);
    _ctx->gal().setPrimitive( ogl::trPrimitive(mode) );

    set<GLuint> iSet;
    libGAL::GALBuffer* indicesBuffer; //

    if ( !_ctx->allBuffersBound() && !_ctx->allBuffersUnbound() )
        CG_ASSERT("Mixed VBO and client-side vertex arrays not supported in indexed mode");

    if ( _ctx->allBuffersBound())
    {
        if ( bufBM.getTarget(GL_ELEMENT_ARRAY_BUFFER).hasCurrent() )
        {
            BufferObject boElementArray = bufBM.target(GL_ELEMENT_ARRAY_BUFFER).getCurrent();
            indicesBuffer = boElementArray.getData();
                _ctx->gal().setIndexBuffer(indicesBuffer, (U64)indices, ogl::getGALStreamType(type, false));

        }
        else
        {
            indicesBuffer = _ctx->rawIndices(indices, type, count);
                _ctx->gal().setIndexBuffer(indicesBuffer, 0, ogl::getGALStreamType(type, false));

        }
    }
    else // ctx->allBuffersUnbound() == TRUE
    {
        if ( bufBM.getTarget(GL_ELEMENT_ARRAY_BUFFER).hasCurrent())
            CG_ASSERT("Unbound GL_ARRAY_BUFFER & bound GL_ELEMENTS_ARRAY_BUFFER not supported");
        else
        {
            _ctx->getIndicesSet(iSet, type, indices, count);
            indicesBuffer = _ctx->createIndexBuffer (indices, type, count, iSet);
        }

            _ctx->gal().setIndexBuffer(indicesBuffer, 0, ogl::getGALStreamType(type, false));

    }


    _ctx->setShaders(); // Sets the proper shader (user/auto-generated) and update GPU shader constants

    _ctx->setVertexArraysBuffers(0, count, iSet);

    _ctx->attachInternalBuffers();

    _ctx->attachInternalSamplers(); // This operation is done with the information provided by the shader. This is why it MUST BE DONE AFTER setShader

    _ctx->gal().drawIndexed(0, count, 0, 0, 0); // Perform the draw using the internal buffers as input

#ifdef GAL_STATS
    _ctx->gal().DBG_dump("galdump.txt", 0);
    _ctx->fixedPipelineState().DBG_dump("GALxjustbeforedraw.txt",0);
#endif

    _ctx->deattachInternalSamplers();

    _ctx->deattachInternalBuffers();

    _ctx->resetDescriptors();

    if (!bufBM.getTarget(GL_ELEMENT_ARRAY_BUFFER).hasCurrent())
        _ctx->gal().destroy(indicesBuffer);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glDrawArrays( GLenum mode, GLint first, GLsizei count )
{
    TRACING_ENTER_REGION("glDraw", "", "")

    OGL_PRINTCALL_3(mode,first,count);

    _ctx->gal().setPrimitive( ogl::trPrimitive(mode) );

    set<GLuint> iSet;

    _ctx->setShaders(); // Sets the proper shader (user/auto-generated) and update GPU shader constants

    _ctx->setVertexArraysBuffers(first, count, iSet);

    _ctx->attachInternalBuffers(); // Attach internal buffers to streams

    _ctx->attachInternalSamplers();

    _ctx->gal().draw(0, count); // Perform the draw using the internal buffers as input

#ifdef GAL_STATS
    _ctx->gal().DBG_dump("galdump.txt", 0);
    _ctx->fixedPipelineState().DBG_dump("GALxjustbeforedraw.txt",0);
#endif

    _ctx->deattachInternalSamplers();

    _ctx->deattachInternalBuffers();

    _ctx->resetDescriptors();

}

GLAPI void GLAPIENTRY OGL_glDrawRangeElements( GLenum mode, GLuint start, GLuint end, GLsizei count,
                                           GLenum type, const GLvoid *indices )

{
    TRACING_ENTER_REGION("glDraw", "", "")

    OGL_PRINTCALL_6(mode, start, end, count, type, indices);

    BufferManager& bufBM = _ctx->bufferManager();

    _ctx->gal().setPrimitive( ogl::trPrimitive(mode) );


    set<GLuint> iSet;   // Conjunt d'indexos que s'utilitzen en el drawElements
    libGAL::GALBuffer* indicesBuffer; //

    if ( !_ctx->allBuffersBound() && !_ctx->allBuffersUnbound() )
        CG_ASSERT("Mixed VBO and client-side vertex arrays not supported in indexed mode");

    if ( _ctx->allBuffersBound())
    {
        if ( bufBM.getTarget(GL_ELEMENT_ARRAY_BUFFER).hasCurrent() )
        {
            BufferObject boElementArray = bufBM.target(GL_ELEMENT_ARRAY_BUFFER).getCurrent();
            indicesBuffer = _ctx->rawIndices(boElementArray.getData()->getData(), type, count);
        }
        else
            indicesBuffer = _ctx->rawIndices(indices, type, count);
    }
    else // ctx->allBuffersUnbound() == TRUE
    {
        if ( bufBM.getTarget(GL_ELEMENT_ARRAY_BUFFER).hasCurrent())
            CG_ASSERT("Unbound GL_ARRAY_BUFFER & bound GL_ELEMENTS_ARRAY_BUFFER not supported");
        else
        {
            _ctx->getIndicesSet(iSet, type, indices, count);
            indicesBuffer = _ctx->createIndexBuffer (indices, type, count, iSet);
        }
    }

    _ctx->gal().setIndexBuffer(indicesBuffer, 0, ogl::getGALStreamType(type, false));

    _ctx->setShaders(); // Sets the proper shader (user/auto-generated) and update GPU shader constants

    _ctx->setVertexArraysBuffers(0, count, iSet);

    _ctx->attachInternalBuffers();

    _ctx->attachInternalSamplers();

    _ctx->gal().drawIndexed(0, count, 0, 0, 0); // Perform the draw using the internal buffers as input

#ifdef GAL_STATS
    _ctx->gal().DBG_dump("galdump.txt", 0);
    _ctx->fixedPipelineState().DBG_dump("GALxjustbeforedraw.txt",0);
#endif

    _ctx->deattachInternalBuffers();

    _ctx->deattachInternalSamplers();

    _ctx->resetDescriptors();

    if (!bufBM.getTarget(GL_ELEMENT_ARRAY_BUFFER).hasCurrent())
        _ctx->gal().destroy(indicesBuffer);

    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glMultMatrixf( const GLfloat *m )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_1(arrayParam(m,16));

    GALxFloatMatrix4x4 mat(m, false);
    _ctx->matrixStack().multiply(mat);

    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glBindBufferARB(GLenum target, GLuint buffer)
{
    TRACING_ENTER_REGION("OGL2", "", "")

    OGL_PRINTCALL_2(target,buffer);

    BufferManager& bufBM = _ctx->bufferManager();

    if ( buffer == 0 )
        bufBM.target(target).resetCurrent(); // unbind buffer (not using VBO)
    else
    {
        BufferObject& buff = static_cast<BufferObject&>(bufBM.bindObject(target, buffer));
        buff.attachDevice(&_ctx->gal());
    }
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glDeleteBuffersARB(GLsizei n, const GLuint *buffers)
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_2(n,buffers);

    BufferManager& bufBM = _ctx->bufferManager();
    bufBM.removeObjects(n, buffers);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glBufferDataARB(GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage)
{
    TRACING_ENTER_REGION("OGL2", "", "")

    OGL_PRINTCALL_4(target, size, data, usage);

    BufferManager& bufBM = _ctx->bufferManager();
    bufBM.target(target).getCurrent().setContents(size, (U08*)data, usage);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glBufferSubDataARB (GLenum target, GLintptrARB offset,
                                              GLsizeiptrARB size, const GLvoid *data)
{
    TRACING_ENTER_REGION("OGL2", "", "")
    
    OGL_PRINTCALL_4(target, offset, size, data);

    BufferManager& bufBM = _ctx->bufferManager();
    bufBM.target(target).getCurrent().setPartialContents(offset, size, (U08*)data);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glTexCoord2f( GLfloat s, GLfloat t )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_2(s, t);

    _ctx->setTexCoord(0,s,t); // Set texture coordinate for texture unit 0
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glBindTexture( GLenum target, GLuint texture )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_2(target, texture);

    GLTextureManager& tm = _ctx->textureManager(); // Get the texture manager

    GLuint tu = tm.getCurrentGroup(); // get id of current texture unit

    if ( texture == 0 ) // binding the default (it is not a TextureObject)
    {
        GLTextureTarget& tg = tm.getTarget(target);
        tg.setDefaultAsCurrent(); // set the default as current
        _ctx->getTextureUnit(tu).setTextureObject(target, &tg.getCurrent());
    }
    else
    {
        // Find the texture object
        GLTextureObject& to = static_cast<GLTextureObject&>(tm.bindObject(target, texture)); // get (or create) the texture object
        // Set the object as the current for the texture unit
        to.attachDevice(&_ctx->gal(),target);
        _ctx->getTextureUnit(tu).setTextureObject(target, &to);
    }
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glTexParameteri( GLenum target, GLenum pname, GLint param )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_3(target, pname, param);

    GLTextureManager& texTM = _ctx->textureManager();
    texTM.getTarget(target).getCurrent().setParameter(pname, &param);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glTexParameterf( GLenum target, GLenum pname, GLfloat param )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_3(target, pname, param);

    GLTextureManager& texTM = _ctx->textureManager();
    texTM.getTarget(target).getCurrent().setParameter(pname, &param);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glTexImage2D( GLenum target, GLint level,
                                        GLint internalFormat,
                                        GLsizei width, GLsizei height,
                                        GLint border, GLenum format, GLenum type,
                                        const GLvoid *pixels )
{
    TRACING_ENTER_REGION("OGL2", "", "")


    OGL_PRINTCALL_9(target,level, internalFormat, width, height, border, format, type, pixels);

    GLTextureObject& to = getTextureObject(target);
    to.attachDevice(&_ctx->gal(),target);

    TRACING_ENTER_REGION("setContents", "GLTextureObject", "setContents")

    switch (internalFormat)
    {
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        case GL_COMPRESSED_LUMINANCE_LATC1_EXT:
        case GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT:
        case GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:
        case GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT:
            internalFormat = GL_RGBA;
            break;
    }

    to.setContents(target, level, internalFormat, width, height, 1, border, format, type, (const GLubyte*)pixels);
    
    TRACING_EXIT_REGION()
    TRACING_EXIT_REGION()
}


GLAPI void GLAPIENTRY OGL_glTexImage3D( GLenum target, GLint level,
                                        GLint internalFormat,
                                        GLsizei width, GLsizei height,
                                        GLsizei depth, GLint border,
                                        GLenum format, GLenum type,
                                        const GLvoid *pixels )

{
    TRACING_ENTER_REGION("OGL2", "", "")


    OGL_PRINTCALL_9(target,level, internalFormat, width, height, border, format, type, pixels);

    GLTextureObject& to = getTextureObject(target);
    to.attachDevice(&_ctx->gal(),target);

    TRACING_ENTER_REGION("setContents", "GLTextureObject", "setContents")
    
        switch (internalFormat)
    {
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
        case GL_COMPRESSED_LUMINANCE_LATC1_EXT:
        case GL_COMPRESSED_SIGNED_LUMINANCE_LATC1_EXT:
        case GL_COMPRESSED_LUMINANCE_ALPHA_LATC2_EXT:
        case GL_COMPRESSED_SIGNED_LUMINANCE_ALPHA_LATC2_EXT:
            internalFormat = GL_RGBA;
            break;
    }

    to.setContents(target, level, internalFormat, width, height, depth, border, format, type, (const GLubyte*)pixels);
    
    TRACING_EXIT_REGION()
    TRACING_EXIT_REGION()

}

GLAPI void GLAPIENTRY OGL_glCompressedTexImage2D( GLenum target, GLint level,
                                              GLenum internalFormat,
                                              GLsizei width, GLsizei height,
                                              GLint border, GLsizei imageSize,
                                              const GLvoid *data )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    

    OGL_PRINTCALL_8(target, level, internalFormat, width, height, border, imageSize, data);


    GLTextureObject& to = getTextureObject(target);
    to.attachDevice(&_ctx->gal(),target);


    TRACING_ENTER_REGION("setContents", "GLTextureObject", "setContents")


    to.setContents(target, level, internalFormat, width, height, 1, border, 0, 0, (const GLubyte*)data, imageSize);


    TRACING_EXIT_REGION()
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glCompressedTexSubImage2DARB (  GLenum target,
                                                    GLint level,
                                                    GLint xoffset,
                                                    GLint yoffset,
                                                    GLsizei width,
                                                    GLsizei height,
                                                    GLenum format,
                                                    GLsizei imageSize,
                                                    const GLvoid *data)
{
    TRACING_ENTER_REGION("OGL2", "", "")
    

    OGL_PRINTCALL_8(target, level, internalFormat, width, height, border, imageSize, data);

    GLTextureObject& to = getTextureObject(target);
    to.attachDevice(&_ctx->gal(),target);


    TRACING_ENTER_REGION("setContents", "GLTextureObject", "setContents")


    to.setPartialContents(target, level, xoffset, yoffset, 0, width, height, 1, format, 0, (const GLubyte*)data, imageSize);


    TRACING_EXIT_REGION()
    TRACING_EXIT_REGION()

}

GLAPI void GLAPIENTRY OGL_glCompressedTexImage2DARB( GLenum target, GLint level,
                                                 GLenum internalFormat,
                                                 GLsizei width, GLsizei height,
                                                 GLint border, GLsizei imageSize,
                                                 const GLvoid *data )
{
    TRACING_ENTER_REGION("OGL2", "", "")


    OGL_PRINTCALL_8(target, level, internalFormat, width, height, border, imageSize, data);

    GLTextureObject& to = getTextureObject(target);
    to.attachDevice(&_ctx->gal(),target);

    TRACING_ENTER_REGION("setContents", "GLTextureObject", "setContents")

    to.setContents(target, level, internalFormat, width, height, 1, border, 0, 0, (const GLubyte*)data, imageSize);

    TRACING_EXIT_REGION()
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glLoadMatrixf( const GLfloat *m )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_1(m);
    // Create identity matrix
    GALxFloatMatrix4x4 mat(m, false);

    _ctx->matrixStack().set(mat);

    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glTexEnvi( GLenum target, GLenum pname, GLint param )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_3(target, pname, param);

    TextureUnit& tu = _ctx->getActiveTextureUnit();
    tu.setParameter(target, pname, &param);

    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glTexEnvf( GLenum target, GLenum pname, GLfloat param )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_3(target, pname, param);

    TextureUnit& tu = _ctx->getActiveTextureUnit();
    tu.setParameter(target, pname, &param);

    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glTexEnvfv( GLenum target, GLenum pname, const GLfloat* param )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_3(target, pname, param);

    TextureUnit& tu = _ctx->getActiveTextureUnit();
    tu.setParameter(target, pname, param);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glActiveTextureARB(GLenum texture)
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_1(texture);

    GLTextureManager& texTM = _ctx->textureManager();
    GLuint maxUnit = texTM.numberOfGroups() - 1;
    GLuint group = texture - GL_TEXTURE0;

    if ( GL_TEXTURE0 <= texture && texture <= GL_TEXTURE0 + maxUnit )
        texTM.selectGroup(group); // select the active group (active texture unit)
    else
    {
        char msg[256];
        sprintf(msg, "TextureUnit %d does not exist", group);
        CG_ASSERT(msg);
    }
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glFogf( GLenum pname, GLfloat param )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_2(pname,param);

    _ctx->setFog(pname, &param);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glFogi( GLenum pname, GLint param )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_2(pname,param);

    _ctx->setFog(pname, &param);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glFogfv( GLenum pname, const GLfloat *params )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_2(pname,params);

    _ctx->setFog(pname, params);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glFogiv( GLenum pname, const GLint *params )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_2(pname,params);

    _ctx->setFog(pname, params);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glTexCoordPointer( GLint size, GLenum type, GLsizei stride, const GLvoid *ptr )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_4(size, type, stride, ptr);

    BufferManager& bufBM = _ctx->bufferManager();

    GLuint clientTUnit = _ctx->getCurrentClientTextureUnit();

    _ctx->getTextureVArray(clientTUnit) = GLContext::VArray(size, type, stride, ptr, true);

    if (bufBM.getTarget(GL_ARRAY_BUFFER).hasCurrent())
        _ctx->getTextureVArray(clientTUnit).bufferID = bufBM.getTarget(GL_ARRAY_BUFFER).getCurrent().getName();
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glBlendFunc( GLenum sfactor, GLenum dfactor )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_2(sfactor, dfactor);

    _ctx->gal().blending().setSrcBlend(0, ogl::getGALBlendOption(sfactor));
    _ctx->gal().blending().setSrcBlendAlpha(0, ogl::getGALBlendOption(sfactor));
    _ctx->gal().blending().setDestBlend(0, ogl::getGALBlendOption(dfactor));
    _ctx->gal().blending().setDestBlendAlpha(0, ogl::getGALBlendOption(dfactor));
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glFrontFace( GLenum mode )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_1(mode);

    switch ( mode )
    {
        case GL_CW:
            _ctx->gal().rast().setFaceMode(libGAL::GAL_FACE_CW);
            break;
        case GL_CCW:
            _ctx->gal().rast().setFaceMode(libGAL::GAL_FACE_CCW);
            break;
        default:
            CG_ASSERT("Unkown front face mode");
    }
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glTexGenfv(GLenum coord, GLenum pname, const GLfloat* param)
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_3(coord,pname,param);

    _ctx->setTexGen(coord, pname, param);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glTexGeni(GLenum coord, GLenum pname, const GLint param)
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_3(coord,pname,param);

    _ctx->setTexGen(coord, pname, param);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glTexGenf(GLenum coord, GLenum pname, const GLfloat param)
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_3(coord,pname,param);

    _ctx->setTexGen(coord, pname, GLfloat(param));
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glClientActiveTextureARB(GLenum texture)
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_1(texture);

    _ctx->setCurrentClientTextureUnit(texture - GL_TEXTURE0);
}

GLAPI void GLAPIENTRY OGL_glCopyTexImage2D( GLenum target, GLint level,
                                            GLenum internalFormat,
                                            GLint x, GLint y,
                                            GLsizei width, GLsizei height,
                                            GLint border )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_8(target, level, internalFormat, x, y, width, height, border);

    GLTextureObject& to = getTextureObject(target);

    switch(internalFormat)
    {
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
            internalFormat = GL_RGBA;
            break;

        case GL_DEPTH_COMPONENT24:
            internalFormat = GL_RGBA;
    }

    GLubyte* data = new GLubyte[width*height*4];

    for (int i=0; i< width*height*4; i+=4)
    {
        if (i%0x3 == 0)
        {
            data[i] = 255;
            data[i+1] = 0;
            data[i+2] = 0;
            data[i+3] = 255;
        }
        else if (i%0x3 == 1)
        {
            data[i] = 0;
            data[i+1] = 255;
            data[i+2] = 0;
            data[i+3] = 255;
        }
        else if (i%0x3 == 2)
        {
            data[i] = 0;
            data[i+1] = 0;
            data[i+2] = 255;
            data[i+3] = 255;
        }
        else
        {
            data[i] = 255;
            data[i+1] = 255;
            data[i+2] = 0;
            data[i+3] = 255;
        }
    }

    TRACING_ENTER_REGION("setContents", "GLTextureObject", "setContext")
    to.setContents(target, level, internalFormat, width, height, 1, border, internalFormat, GL_UNSIGNED_BYTE, (const GLubyte*)data, 0);
    TRACING_EXIT_REGION()

    delete[] data;

    _ctx->gal().sampler(_ctx->getActiveTextureUnitNumber()).performBlitOperation2D(0, 0,
        x, y, width, height, to.getWidth(target,level), ogl::getGALTextureFormat(internalFormat), (libGAL::GALTexture2D*)to.getTexture(), level);

    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glCopyTexSubImage2D( GLenum target, GLint level,
                                           GLint xoffset, GLint yoffset,
                                           GLint x, GLint y,
                                           GLsizei width, GLsizei height )
{
    TRACING_ENTER_REGION("OGL2", "", "")

    OGL_PRINTCALL_8(target, level, xoffset, yoffset, x, y, width, height);

    GLTextureObject& to = getTextureObject(target);

	GLenum internalFormat = to.getFormat(target,level);
    internalFormat = to.getGLTextureFormat((GAL_FORMAT) internalFormat);

    switch(internalFormat)
    {
        case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
        case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
            internalFormat = GL_RGBA;
            break;
        case GL_DEPTH_COMPONENT24:
            internalFormat = GL_RGBA;
    }

    GLubyte* subData = new GLubyte[width * height * 4];

    for (int i=0; i< width*height*4; i+=4)
    {
        if (i%0x3 == 0)
        {
            subData[i] = 255;
            subData[i+1] = 0;
            subData[i+2] = 0;
            subData[i+3] = 255;
        }
        else if (i%0x3 == 1)
        {
            subData[i] = 0;
            subData[i+1] = 255;
            subData[i+2] = 0;
            subData[i+3] = 255;
        }
        else if (i%0x3 == 2)
        {
            subData[i] = 0;
            subData[i+1] = 0;
            subData[i+2] = 255;
            subData[i+3] = 255;
        }
        else
        {
            subData[i] = 255;
            subData[i+1] = 255;
            subData[i+2] = 0;
            subData[i+3] = 255;
        }
    }

    TRACING_ENTER_REGION("setContents", "GLTextureObject", "setContext")
    to.setPartialContents(target, level, xoffset, yoffset, 0, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, subData, 0);
    TRACING_EXIT_REGION()

    delete[] subData;

    _ctx->gal().sampler(_ctx->getActiveTextureUnitNumber()).performBlitOperation2D(xoffset, yoffset,
        x, y, width, height,to.getWidth(target,level), ogl::getGALTextureFormat(internalFormat), (libGAL::GALTexture2D*)to.getTexture(), level);

    TRACING_EXIT_REGION()
}


GLAPI void GLAPIENTRY OGL_glDepthMask( GLboolean flag )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_1(flag);

    _ctx->gal().zStencil().setZMask(flag);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glEnableVertexAttribArrayARB (GLuint index)
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_1(index);

    _ctx->setEnabledGenericAttribArray(index, true);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glDisableVertexAttribArrayARB (GLuint index)
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_1(index);

    _ctx->setEnabledGenericAttribArray(index, false);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glVertexAttribPointerARB(GLuint index, GLint size, GLenum type,
                                                GLboolean normalized, GLsizei stride, const GLvoid *pointer)
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_6(index, size, type, normalized, stride, pointer);

    BufferManager& bufBM = _ctx->bufferManager();

    _ctx->getGenericVArray(index) = GLContext::VArray(size, type, stride, pointer, normalized);

    if (bufBM.getTarget(GL_ARRAY_BUFFER).hasCurrent())
        _ctx->getGenericVArray(index).bufferID = bufBM.getTarget(GL_ARRAY_BUFFER).getCurrent().getName();

    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glStencilFunc( GLenum func, GLint ref, GLuint mask )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_3(func, ref, mask);

    _ctx->gal().zStencil().setStencilFunc(libGAL::GAL_FACE_FRONT,ogl::getGALCompareFunc(func), ref, mask);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glStencilOp( GLenum fail, GLenum zfail, GLenum zpass )
{
    TRACING_ENTER_REGION("OGL2", "", "")

    OGL_PRINTCALL_3 (fail, zfail, zpass);

    _ctx->gal().zStencil().setStencilOp(libGAL::GAL_FACE_FRONT,
                                        ogl::getGALStencilOp(fail),
                                        ogl::getGALStencilOp(zfail),
                                        ogl::getGALStencilOp(zpass));

    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glStencilOpSeparateATI(GLenum face, GLenum fail, GLenum zfail, GLenum zpass )
{
    TRACING_ENTER_REGION("OGL2", "", "")

    OGL_PRINTCALL_3 (fail, zfail, zpass);

    _ctx->gal().zStencil().setStencilOp(ogl::getGALFace(face),
                                        ogl::getGALStencilOp(fail),
                                        ogl::getGALStencilOp(zfail),
                                        ogl::getGALStencilOp(zpass));

    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glClearStencil( GLint stencilValue )
{
    TRACING_ENTER_REGION("OGL2", "", "")

    OGL_PRINTCALL_1(stencilValue);

    _ctx->setStencilClearValue(stencilValue);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glStencilMask( GLuint mask )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_1(mask);

    _ctx->gal().zStencil().setStencilUpdateMask(mask);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glColorMask( GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha )
{
    TRACING_ENTER_REGION("OGL2", "", "")

    OGL_PRINTCALL_4(red, green, blue, alpha);

    _ctx->gal().blending().setColorMask(0, red, green, blue, alpha);

    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glAlphaFunc( GLenum func, GLclampf ref )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_2(func, ref);

    _ctx->fixedPipelineSettings().alphaTestFunction = ogl::getGALxAlphaFunc(func);
    //_ctx->fixedPipelineState().postshade().setAlphaTestRefValue((gal_float) ref);

    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glBlendColor( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_4(red, green, blue, alpha);

    _ctx->gal().blending().setBlendColor(0, red, green, blue, alpha);

    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glColorMaterial( GLenum face, GLenum mode )
{
    TRACING_ENTER_REGION("OGL2", "", "")

    OGL_PRINTCALL_2(face, mode);

    _ctx->fixedPipelineSettings().colorMaterialEnabledFace = ogl::getGALxColorMaterialFace(face);
    _ctx->fixedPipelineSettings().colorMaterialMode = ogl::getGALxColorMaterialMode(mode);

    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glScissor( GLint x, GLint y, GLsizei width, GLsizei height)
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_4(x, y, width, height);

    _ctx->gal().rast().setScissor(x, y, width, height);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glDepthRange( GLclampd near_val, GLclampd far_val )
{
    TRACING_ENTER_REGION("OGL2", "", "")

    OGL_PRINTCALL_2(near_val, far_val);

    _ctx->fixedPipelineState().fshade().setDepthRange(near_val, far_val);
    _ctx->gal().zStencil().setDepthRange(static_cast<GLfloat>(near_val), static_cast<GLfloat>(far_val));

    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glPolygonOffset( GLfloat factor, GLfloat units )
{
    TRACING_ENTER_REGION("OGL2", "", "")

    OGL_PRINTCALL_2(factor, units);

    _ctx->gal().zStencil().setPolygonOffset (factor, units);

    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glPushAttrib( GLbitfield mask )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_1(mask);

    _ctx->pushAttrib(mask);
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glPopAttrib( void )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_1(void);

    _ctx->popAttrib();
    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glBlendEquation( GLenum mode )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_1(mode);

    _ctx->gal().blending().setBlendFunc(0, ogl::getGALBlendFunction(mode));

    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glColor4ub( GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha )
{
    TRACING_ENTER_REGION("OGL2", "", "")

    OGL_PRINTCALL_4(red, green, blue, alpha);

    GLfloat r = GLfloat(red) / 0xFF;
    GLfloat g = GLfloat(green) / 0xFF;
    GLfloat b = GLfloat(blue) / 0xFF;
    GLfloat a = GLfloat(alpha) / 0xFF;

    _ctx->setColor(r, g, b, a);

    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glColor4fv( const GLfloat *v )
{
    TRACING_ENTER_REGION("OGL2", "", "")

    OGL_PRINTCALL_4(red, green, blue, alpha);

    _ctx->setColor(v[0], v[1], v[2], v[3]);

    TRACING_EXIT_REGION()
}

GLAPI const GLubyte* GLAPIENTRY OGL_glGetString( GLenum name )
{
    TRACING_ENTER_REGION("OGL2", "", "")

    OGL_PRINTCALL_1(name);

    static GLubyte vendor[] = "DisplayController";
    static GLubyte renderer[] = "CG1 GPU";
    static GLubyte ver[] = "1.0";
    static GLubyte extensions[] = "Why do you want to check extensions ? We support ALL";
    switch ( name )
    {
        case GL_VENDOR:
            return vendor;
        case GL_RENDERER:
            return renderer;
        case GL_VERSION:
            return ver;
        case GL_EXTENSIONS:
            return extensions;
    }

    TRACING_EXIT_REGION()
    CG_ASSERT("Unknown constant");
    return 0;

}

GLAPI void GLAPIENTRY OGL_glTexSubImage2D( GLenum target, GLint level,
                                       GLint xoffset, GLint yoffset,
                                       GLsizei width, GLsizei height,
                                       GLenum format, GLenum type,
                                       const GLvoid *pixels )
{
    TRACING_ENTER_REGION("OGL2", "", "")

    OGL_PRINTCALL_9(target,level, xoffset, yoffset, width, height, format, type, pixels);

    GLTextureObject& to = getTextureObject(target);

    TRACING_ENTER_REGION("setContents", "GLTextureObject", "setContext")
    to.setPartialContents(target, level, xoffset, yoffset, 0, width, height, 0, format, type, (const GLubyte*)pixels);
    TRACING_EXIT_REGION()

    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glMultiTexCoord2f( GLenum texture, GLfloat s, GLfloat t )
{
    TRACING_ENTER_REGION("OGL2", "", "")

    OGL_PRINTCALL_3(texture, s, t);

    GLuint texUnit = texture - GL_TEXTURE0;
    _ctx->setTexCoord(texUnit, s, t);

    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glMultiTexCoord2fv( GLenum texture, const GLfloat *v )
{
    TRACING_ENTER_REGION("OGL2", "", "")

    OGL_PRINTCALL_2(texture, v);

    GLuint texUnit = texture - GL_TEXTURE0;
    _ctx->setTexCoord(texUnit, v[0], v[1]);

    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glPixelStorei( GLenum pname, GLint param )
{
    OGL_PRINTCALL_2(pname, param );

    //cout << "glPixelStorei ---> call ignored\n";

}

GLAPI void GLAPIENTRY OGL_glIndexPointer( GLenum type, GLsizei stride, const GLvoid *ptr )
{
    TRACING_ENTER_REGION("OGL2", "", "")

    OGL_PRINTCALL_3(type, stride, ptr);

    BufferManager& bufBM = _ctx->bufferManager();

    _ctx->getIndexVArray() = GLContext::VArray(1, type, stride, ptr, false);

    if ( bufBM.getTarget(GL_ARRAY_BUFFER).hasCurrent())
        _ctx->getIndexVArray().bufferID = bufBM.getTarget(GL_ARRAY_BUFFER).getCurrent().getName();

    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glEdgeFlagPointer( GLsizei stride, const GLvoid *ptr )
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_2(stride, ptr);

    BufferManager& bufBM = _ctx->bufferManager();

    _ctx->getEdgeVArray() = GLContext::VArray(1,GL_UNSIGNED_BYTE,stride, ptr, false);

    if ( bufBM.getTarget(GL_ARRAY_BUFFER).hasCurrent() )
        _ctx->getEdgeVArray().bufferID = bufBM.getTarget(GL_ARRAY_BUFFER).getCurrent().getName();

    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glArrayElement( GLint i )
{
    OGL_PRINTCALL_1(i);

    CG_ASSERT("Function not implemented :-)");

    // called inside glBegin/End
    // uses glBegin/glEnd buffers
}

GLAPI void GLAPIENTRY OGL_glInterleavedArrays( GLenum format, GLsizei stride, const GLvoid *pointer )
{
    TRACING_ENTER_REGION("OGL2", "", "")

    OGL_PRINTCALL_3(format, stride, pointer);

    switch ( format )
    {
        case GL_C4F_N3F_V3F:
            // enable vertex arrays
            _ctx->enableBufferArray (GL_VERTEX_ARRAY);
            _ctx->enableBufferArray (GL_COLOR_ARRAY);
            _ctx->enableBufferArray (GL_NORMAL_ARRAY);

            if (stride == 0)
                stride = 10*sizeof(GLfloat); // See OpenGL 2.0 Specification section 2.8 Vertex Arrays at pages 31 and 32

            _ctx->getColorVArray() = GLContext::VArray(4, GL_FLOAT, stride, pointer, true);
            _ctx->getNormalVArray() = GLContext::VArray(3, GL_FLOAT, stride, ((GLubyte *)pointer) + 4*sizeof(GLfloat), true);
            _ctx->getPosVArray() = GLContext::VArray(3, GL_FLOAT, stride, ((GLubyte *)pointer) + 7*sizeof(GLfloat), true);
            break;
        default:
            CG_ASSERT("Format not yet unsupported");
    }

    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glDeleteTextures(GLsizei n, const GLuint *textures)
{
    TRACING_ENTER_REGION("OGL2", "", "")

    OGL_PRINTCALL_2(n, textures);

    GLTextureManager& texTM = _ctx->textureManager();
    texTM.removeObjects(n, textures);
    TRACING_EXIT_REGION()

}

GLAPI void GLAPIENTRY OGL_glProgramEnvParameter4fvARB (GLenum target, GLuint index, const GLfloat *params)
{
    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_3(target, index, params);

    // Get the enviromental parameter bank of the current
    ARBRegisterBank& env = _ctx->arbProgramManager().target(target).getEnv();

    // Update enviromental parameter bank
    env.set(index,params[0], params[1], params[2], params[3]);

    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glProgramEnvParameters4fvEXT (GLenum target, GLuint index, GLsizei count, const GLfloat* d)
{
    TRACING_ENTER_REGION("OGL2", "", "")

    // Get the enviromental parameter bank of the current
    ARBRegisterBank& env = _ctx->arbProgramManager().target(target).getEnv();

    // Update enviromental parameter bank
    for (gal_uint i = 0; i < count ; i++)
        env.set(index + i, d[i*4], d[i*4+1], d[i*4+2], d[i*4+3]);

    TRACING_EXIT_REGION()
}


GLAPI void GLAPIENTRY OGL_glPolygonMode	(GLenum face, GLenum mode)
{

    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_2(face, mode);

    if (mode != GL_FILL)
        CG_ASSERT("GPU only supports GL_FILL mode");

    TRACING_EXIT_REGION()
}

GLAPI void GLAPIENTRY OGL_glBlendEquationEXT (GLenum mode)
{

    TRACING_ENTER_REGION("OGL2", "", "")
    OGL_PRINTCALL_1(mode);

    _ctx->gal().blending().setBlendFunc(0, ogl::getGALBlendFunction(mode));

    TRACING_EXIT_REGION()
}
