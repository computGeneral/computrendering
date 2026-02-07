/**************************************************************************
 *
 */

#include "TraceDriverOGL.h"
//#include "GPULib.h"
#include <cstdio>
#include <iostream>
#include "GLContext.h"
//#include "GALDevice.h"
#include "AuxFuncsLib.h"
#include "GPULibInternals.h"
#include "OGLEntryPoints.h"
#include "OGL.h"
#include "GLResolver.h"
#include "BufferDescriptor.h"

#include "GlobalProfiler.h"

using namespace cg1gpu;
using namespace std;

//  TraceDriver constructor.  
TraceDriverOGL::TraceDriverOGL(char *ProfilingFile, HAL* driver, bool triangleSetupOnShader, U32 startFrame) :
    startFrame(startFrame), currentFrame(0)
    // initialize cgoMetaStream buffer control variables
{
    this->driver = driver;
    traceTyp = TraceTypOgl;

    GLOBAL_PROFILER_ENTER_REGION("GLExec", "", "")
    BufferManager& bm = gle.getBufferManager();
    GLOBAL_PROFILER_EXIT_REGION()
        
    // Must be selected before opening Buffer Descriptors file
    bm.acceptDeferredBuffers(true);

    GLOBAL_PROFILER_ENTER_REGION("GLExec", "", "")
    initValue = gle.init(ProfilingFile, "BufferDescriptors.dat", "MemoryRegions.dat");
    GLOBAL_PROFILER_EXIT_REGION()

    _setOGLFunctions(); // overwrite jump table with OGL versions
    ogl::initOGL(driver, startFrame);
}

TraceDriverOGL::TraceDriverOGL( const char* ProfilingFile, const char* bufferFile, const char* memFile,
    HAL* driver, bool triangleSetupOnShader, U32 startFrame ) : 
        startFrame(startFrame), currentFrame(0)
{
    this->driver = driver;

    GLOBAL_PROFILER_ENTER_REGION("GLExec", "", "")
    BufferManager& bm = gle.getBufferManager();
    
    // Must be selected before opening Buffer Descriptors file
    bm.acceptDeferredBuffers(true);

    initValue = gle.init(ProfilingFile, bufferFile, memFile);
    GLOBAL_PROFILER_EXIT_REGION()
    
    _setOGLFunctions(); // overwrite jump table with OGL versions
    ogl::initOGL(driver, startFrame);
}

int TraceDriverOGL::startTrace()
{
    // do not do anything :-)
    return initValue; // return state of TraceDriver object ( 0 means ready )
}


// new version to allow hotStart with identical memory footprint
cgoMetaStream* TraceDriverOGL::nxtMetaStream()
{
    cgoMetaStream* agpt;
    APICall oglCommand;

    agpt = NULL;

    static bool resInit = false;
    if ( !resInit )
    {
        GLOBAL_PROFILER_ENTER_REGION("GLExec", "", "")
        if ( startFrame != 0 )
            gle.setDirectivesEnabled(false); // Disable directives
        else
            gle.setDirectivesEnabled(true);
        resInit = true;
        U32 width, height;
        gle.getTraceResolution(width, height);
        GLOBAL_PROFILER_EXIT_REGION()
        if ( width != 0 && height != 0)
        {
            cout << "TraceDriver: Setting resolution to " << width << "x" << height << endl;
            ogl::setResolution(width, height);
        }
        //  Set preload memory mode
        if (startFrame > 0)
            driver->setPreloadMemory(true);
    }

    // Try to drain an cgoMetaStream from driver buffer
    while ( !( agpt = driver->nextMetaStream() ) )
    {
        bool interpretBatchAsFrame = false; // Interpret this batch as a frame

        GLOBAL_PROFILER_ENTER_REGION("GLExec", "", "")
        oglCommand = gle.getCurrentCall();
        GLOBAL_PROFILER_EXIT_REGION()
        
        // Enable this code to dump GL calls ignored by GLExec
        //if ( !gle.isCallAvailable(oglCommand) && oglCommand != APICall_UNDECLARED )
        //    cout << "Warning - OpenGL call " << GLResolver::getFunctionName(oglCommand) << " NOT AVAILABLE" << endl;

        switch ( oglCommand )
        {
            /*
            case APICall_glClear:
                if ( currentFrame < startFrame )
                {
                    gle.skipCurrentCall();
                    continue;
                }
                */
            case APICall_glEnd:
            case APICall_glDrawArrays:
            case APICall_glDrawElements:
            case APICall_glDrawRangeElements:
                GLOBAL_PROFILER_ENTER_REGION("GLExec", "", "")
                interpretBatchAsFrame = gle.checkBatchesAsFrames();
                GLOBAL_PROFILER_EXIT_REGION()
                break;
            case APICall_SPECIAL:
            {
                GLOBAL_PROFILER_ENTER_REGION("GLExec", "", "")
                string str = gle.getSpecialString();
                GLOBAL_PROFILER_EXIT_REGION()
                if ( str == "DUMP_CTX" )
                {
                    // process DUMP_CTX
                    if ( driver->getContext() == 0 )
                        cout << "output:> GL Context not found (command igbnored)" << endl;
                    else {
                        //libGAL::GALDevice* galDev = (libGAL::GALDevice*)driver->getContext();
                        cout << "output:> Performing DUMP_CTX:" << endl;
                        //galDev->DBG_dump("", 0);
                    }                    
                }
                else if ( str == "DUMP_STENCIL" )
                {
                    // process DUMP_STENCIL
                    cout << "output:> GALx does not implement yet stencil buffer dump" << endl;
                }
                else
                    cout << "output:> '" << str << "' unknown command" << endl;
                gle.resetSpecialString();
            }

            case APICall_UNDECLARED:
                return 0; // End of trace
        }
        
        GLOBAL_PROFILER_ENTER_REGION("GLExec", "", "")
        gle.executeCurrentCall(); // Perform call execution
        GLOBAL_PROFILER_EXIT_REGION()

        if ( oglCommand == APICall_wglSwapBuffers || interpretBatchAsFrame )
        {
            APICall nextOGLCall = gle.getCurrentCall();
            if ( interpretBatchAsFrame && nextOGLCall == APICall_wglSwapBuffers )
                continue; // DO NOT DO ANYTHING. Swap will be performed by wglSwapBuffers (including currentFrame++)

            if ( currentFrame >= startFrame )
            {
                cout << "Dumping frame " << currentFrame;
                if ( oglCommand != APICall_wglSwapBuffers )
                    cout << "  (It is a BATCH)";
                cout << endl;
            }
            else
            {
                cout << "Frame " << currentFrame << " Skipped";
                if ( oglCommand != APICall_wglSwapBuffers )
                    cout << "  (It is a BATCH)";
                cout << endl;
            }
            
            ogl::swapBuffers();
            //  Clean end of frame event to remove swap (flush) and DisplayController cycles.                
            if (currentFrame >= startFrame)
                driver->signalEvent(GPU_END_OF_FRAME_EVENT, "");

            currentFrame++;
            
            //  Disable preload memory mode just before ending the last skipped frame
            if (currentFrame == startFrame)
            {
                driver->setPreloadMemory(false);
                
                cout << "TraceDriver::nxtMetaStream() -> Disabling preload..." << endl;
               
                gle.setDirectivesEnabled(true);
           
                //  Reset the end of frame event.
                driver->signalEvent(GPU_END_OF_FRAME_EVENT, "");
                
                //  Signal that the preload phased has finished.
                driver->signalEvent(GPU_UNNAMED_EVENT, "End of preload phase.  Starting simulation.");
            }

            driver->printMemoryUsage();
            //driver->dumpStatistics();
        }
    }
    
    return agpt;
}

//  Return the current position inside the trace file.
U32 TraceDriverOGL::getTracePosition()
{
    return gle.currentTracefileLine();
}

/*
cgoMetaStream* TraceDriver::nxtMetaStream()
{
    cgoMetaStream* agpt;
    APICall oglCommand;

    static bool resInit = false;
    if ( !resInit )
    {
        if ( startFrame != 0 )
            gle.setDirectivesEnabled(false); // Disable directives
        else
            gle.setDirectivesEnabled(true);
        resInit = true;
        U32 width, height;
        gle.getTraceResolution(width, height);
        if ( width != 0 && height != 0)
        {
            cout << "TraceDriver: Setting resolution to " << width << "x" << height << endl;
            driver->setResolution(width, height);
            driver->initBuffers();
        }
    }

    // Try to drain an cgoMetaStream from driver buffer
    while ( !( agpt = driver->nxtMetaStream() ) )
    {
        bool interpretBatchAsFrame = false; // Interpret this batch as a frame
        bool skipCall = false; // Skip this call

        // If there is not cgoMetaStreams read an execute a new OGL Command from ProfilingFile


        oglCommand = gle.getCurrentCall();

        switch ( oglCommand )
        {
            case APICall_glClear:
            case APICall_glBegin:
            case APICall_glVertex2f:
            case APICall_glVertex3f:
            case APICall_glVertex3fv:
            case APICall_glVertex2i:
                if ( currentFrame < startFrame )
                    skipCall = true;
                break;
            case APICall_glDrawArrays:
            case APICall_glEnd:
                if  ( currentFrame < startFrame )
                {
                    skipCall = true;
                    if ( hotStart )
                        ctx->preloadMemory();
                }
                if ( gle.checkBatchesAsFrames() )
                    interpretBatchAsFrame = true;
                break;

            case APICall_glDrawElements:
            case APICall_glDrawRangeElements:
                if  ( currentFrame < startFrame )
                {
                    skipCall = true;
                    if ( hotStart )
                        ctx->preloadMemory(true);
                }
                if ( gle.checkBatchesAsFrames() )
                    interpretBatchAsFrame = true;
                break;
            case APICall_UNDECLARED:
                return NULL;
            case APICall_SPECIAL:
            {
                string str = gle.getSpecialString();
                if ( str == "DUMP_CTX" )
                {
                    // process DUMP_CTX
                    libgl::GLContext* ctx = (libgl::GLContext*)driver->getContext();
                    if ( ctx )
                    {
                        cout << "output:> Performing DUMP_CTX:" << endl;
                        ctx->dump();
                    }
                    else
                        cout << "output:> GL Context not found (command igbnored)" << endl;
                }
                else if ( str == "DUMP_STENCIL" )
                {
                    // process DUMP_STENCIL
                    libgl::GLContext* ctx = (libgl::GLContext*)driver->getContext();
                    if ( ctx )
                    {
                        cout << "output:> Performing Dump of Stencil Buffer." << endl;
                        libgl::afl::dumpStencilBuffer(ctx);
                        interpretBatchAsFrame = true;
                    }
                    else
                        cout << "output:> GL Context not found (command igbnored)" << endl;
                }
                else
                    cout << "output:> '" << str << "' unknown command" << endl;
                gle.resetSpecialString();
            }
        }

        APICall nextOGLCall;

        if ( skipCall ) // Current (BATCH) call must be skipped
        {
            gle.skipCurrentCall();
            nextOGLCall = gle.getCurrentCall();
            if ( interpretBatchAsFrame && nextOGLCall != APICall_wglSwapBuffers ) // Batch that must be counted
            {
                cout << "Frame " << currentFrame << " Skipped (it is a BATCH)" << endl;
                currentFrame++;
                ctx->currentFrame(currentFrame); // DEBUG
            }
            // if nextOGLCall == APICall_wglSwapBuffers then frame count will be performed by wglSwapBuffers
            continue; // go to next loop iteration
        }

        gle.executeCurrentCall(); // Perform call execution

        if ( oglCommand == APICall_wglSwapBuffers || interpretBatchAsFrame )
        {
            if ( currentFrame >= startFrame )
            {
                if ( interpretBatchAsFrame && nextOGLCall == APICall_wglSwapBuffers )
                {
                    continue; // DO NOT DO ANYTHING. Swap will be performed by wglSwapBuffers (including currentFrame++)
                }

                if ( currentFrame >= startFrame )
                    gle.setDirectivesEnabled(true);

                cout << "Dumping frame " << currentFrame; // << " (trace line " << gle.currentTracefileLine() << ")";
                if ( oglCommand != APICall_wglSwapBuffers )
                    cout << "  (It is a BATCH)";
                cout << endl;

                privateSwapBuffers();
            }
            else
            {
                if ( interpretBatchAsFrame )
                    CG_ASSERT("PROG ERROR...");
                cout << "Frame " << currentFrame << " Skipped" << endl;
            }
            currentFrame++;
            ctx->currentFrame(currentFrame); // DEBUG
            driver->printMemoryUsage();
        }
    }
    return agpt;
}
*/

void TraceDriverOGL::_setOGLFunctions()
{
    GLJumpTable& jt = gle.jTable();

    memset(&jt, 0, sizeof(GLJumpTable)); // reset previous pointers

    // Set jumptable pointers to point new "OGL14 on GAL" entry points
    jt.glBegin = OGL_glBegin;
    jt.glBindBufferARB = OGL_glBindBufferARB;
    jt.glBindProgramARB = OGL_glBindProgramARB;
    jt.glBufferDataARB = OGL_glBufferDataARB;
    jt.glBufferSubDataARB = OGL_glBufferSubDataARB;
    jt.glClear = OGL_glClear;
    jt.glClearColor = OGL_glClearColor;
    jt.glClearDepth = OGL_glClearDepth;
    jt.glColor3f = OGL_glColor3f;
    jt.glColor4f = OGL_glColor4f;
    jt.glColorPointer = OGL_glColorPointer;
    jt.glDepthFunc = OGL_glDepthFunc;
    jt.glDisableClientState = OGL_glDisableClientState;
    jt.glDrawArrays = OGL_glDrawArrays;
    jt.glDrawElements = OGL_glDrawElements;
    jt.glDrawRangeElements = OGL_glDrawRangeElements;
    jt.glDeleteBuffersARB = OGL_glDeleteBuffersARB;
    jt.glEnable = OGL_glEnable;
    jt.glEnableClientState = OGL_glEnableClientState;
    jt.glEnd = OGL_glEnd;
    jt.glFlush = OGL_glFlush;
    jt.glLightfv = OGL_glLightfv;
    jt.glLightf = OGL_glLightf;
    jt.glLightModelfv = OGL_glLightModelfv;
    jt.glLightModelf = OGL_glLightModelf;
    jt.glLightModeli = OGL_glLightModeli;
    jt.glLoadIdentity = OGL_glLoadIdentity;
    jt.glMaterialfv = OGL_glMaterialfv;
    jt.glMaterialf = OGL_glMaterialf;
    jt.glMatrixMode = OGL_glMatrixMode;
    jt.glMultMatrixd = OGL_glMultMatrixd;
    jt.glMultMatrixf = OGL_glMultMatrixf;
    jt.glNormal3f = OGL_glNormal3f;
    jt.glNormal3fv = OGL_glNormal3fv;
    jt.glNormalPointer = OGL_glNormalPointer;
    jt.glOrtho = OGL_glOrtho;
    jt.glFrustum = OGL_glFrustum;
    jt.glPopMatrix = OGL_glPopMatrix;
    jt.glProgramLocalParameter4fARB = OGL_glProgramLocalParameter4fARB;
    jt.glProgramStringARB = OGL_glProgramStringARB;
    jt.glPushMatrix = OGL_glPushMatrix;
    jt.glRotatef = OGL_glRotatef;
    jt.glScalef = OGL_glScalef;
    jt.glShadeModel = OGL_glShadeModel;
    jt.glTranslatef = OGL_glTranslatef;
    jt.glTranslated = OGL_glTranslated;
    jt.glVertex3f = OGL_glVertex3f;
    jt.glVertex3fv = OGL_glVertex3fv;
    jt.glVertexPointer = OGL_glVertexPointer;
    jt.glViewport = OGL_glViewport;
    
    jt.glTexCoord2f = OGL_glTexCoord2f;
    jt.glTexParameteri = OGL_glTexParameteri;
    jt.glTexParameterf = OGL_glTexParameterf;
    jt.glBindTexture = OGL_glBindTexture;
    jt.glTexImage2D = OGL_glTexImage2D;
    jt.glLoadMatrixf = OGL_glLoadMatrixf;
    jt.glCompressedTexSubImage2DARB = OGL_glCompressedTexSubImage2DARB;
    jt.glCompressedTexImage2DARB = OGL_glCompressedTexImage2DARB;
    jt.glCompressedTexImage2D = OGL_glCompressedTexImage2D;
    jt.glTexEnvi = OGL_glTexEnvi;
    jt.glTexEnvf = OGL_glTexEnvf;
    jt.glTexEnvfv = OGL_glTexEnvfv;
    jt.glTexGenfv = OGL_glTexGenfv;
    jt.glTexGeni = OGL_glTexGeni;
    jt.glTexGenf = OGL_glTexGenf;
    jt.glActiveTextureARB = OGL_glActiveTextureARB;
    jt.glFogf = OGL_glFogf;
    jt.glFogi = OGL_glFogi;
    jt.glFogfv = OGL_glFogfv;
    jt.glFogiv = OGL_glFogiv;
    jt.glTexCoordPointer = OGL_glTexCoordPointer;
    jt.glBlendFunc = OGL_glBlendFunc;
    jt.glFrontFace = OGL_glFrontFace;
    jt.glCullFace = OGL_glCullFace;
    jt.glVertex2i = OGL_glVertex2i;
    jt.glVertex2f = OGL_glVertex2f;
    jt.glDisable =  OGL_glDisable;
    jt.glProgramLocalParameter4fvARB  = OGL_glProgramLocalParameter4fvARB ;
    jt.glEnableVertexAttribArrayARB  = OGL_glEnableVertexAttribArrayARB ;
    jt.glDisableVertexAttribArrayARB  = OGL_glDisableVertexAttribArrayARB ;
    jt.glVertexAttribPointerARB = OGL_glVertexAttribPointerARB;
    jt.glCopyTexImage2D = OGL_glCopyTexImage2D;
    jt.glClientActiveTextureARB = OGL_glClientActiveTextureARB;
    jt.glDepthMask = OGL_glDepthMask;
    jt.glRotated = OGL_glRotated;
  
    jt.glStencilFunc = OGL_glStencilFunc;
    jt.glStencilOp = OGL_glStencilOp;
    jt.glClearStencil = OGL_glClearStencil;
    jt.glStencilMask = OGL_glStencilMask;
    jt.glColorMask = OGL_glColorMask;
    jt.glAlphaFunc = OGL_glAlphaFunc;
    jt.glBlendEquation = OGL_glBlendEquation;
    jt.glBlendColor = OGL_glBlendColor;
    jt.glColorMaterial = OGL_glColorMaterial;
    jt.glScissor = OGL_glScissor;
    jt.glDepthRange = OGL_glDepthRange;
    jt.glPolygonOffset = OGL_glPolygonOffset;
    jt.glPushAttrib = OGL_glPushAttrib;
    jt.glPopAttrib = OGL_glPopAttrib;
    
    jt.glIndexPointer = OGL_glIndexPointer;
    jt.glEdgeFlagPointer = OGL_glEdgeFlagPointer;
    jt.glArrayElement = OGL_glArrayElement;
    jt.glInterleavedArrays = OGL_glInterleavedArrays;
    jt.glGetString = OGL_glGetString;
    jt.glProgramEnvParameter4fvARB  = OGL_glProgramEnvParameter4fvARB ;
    jt.glColor4ub = OGL_glColor4ub;
    jt.glColor4fv = OGL_glColor4fv;
    jt.glDeleteTextures = OGL_glDeleteTextures;
    jt.glPixelStorei = OGL_glPixelStorei;
    jt.glTexSubImage2D = OGL_glTexSubImage2D;
    jt.glCopyTexSubImage2D = OGL_glCopyTexSubImage2D;
    jt.glMultiTexCoord2f = OGL_glMultiTexCoord2f;
    jt.glMultiTexCoord2fv = OGL_glMultiTexCoord2fv;

    jt.glProgramLocalParameters4fvEXT = OGL_glProgramLocalParameters4fvEXT;
    jt.glProgramEnvParameters4fvEXT = OGL_glProgramEnvParameters4fvEXT;
    jt.glPolygonMode = OGL_glPolygonMode;
    jt.glBlendEquationEXT = OGL_glBlendEquationEXT;
    jt.glStencilOpSeparateATI = OGL_glStencilOpSeparateATI;
    jt.glTexImage3D = OGL_glTexImage3D;
    
}
