/**************************************************************************
 *
 */

#ifndef __TRACEDRIVER_OGL_H__
#define __TRACEDRIVER_OGL_H__

#include "GPUType.h"
#include "MetaStream.h"
#include "TraceDriverBase.h"
#include "HAL.h"
#include "GLExec.h"
#include "zfstream.h"

void privateSwapBuffers();

/**
 *
 *  GL Trace Driver class.
 *
 *  Generates MetaStreams for the CG1 GPU from an OpenGL API trace file.
 *
 */
class TraceDriverOGL : public cgoTraceDriverBase
{

private:

    GLExec gle;         // OpenGL command manager ( executes ogl commands in a tracefile )
    HAL*   driver;      // GPU Driver
    char*  ProfilingFile;   // API trace file
    int    initValue;   // 0 if creation was ok, < 0 value if not
    U32    startFrame;
    U32    currentFrame;

    void _setOGLFunctions();
 
public:

    /**
     * GL Trace Driver constructor
     *
     * Creates a GL Trace Driver object and initializes it.
     * @param ProfilingFile The name of the API trace file from which to read 3D commands.
     * @return An initialized GL Trace Driver object.
     * @note Remains available for backwards compatibility (implicity sets bufferFile to "GLIBuffers.dat"),
     *       use TraceDriver constructor with 3 parameters instead
     */
    TraceDriverOGL( char *ProfilingFile, HAL* driver, bool triangleSetupOnShader, U32 startFrame = 0);

    /**
     * GL Trace driver constructor
     *
     * Creates a GL Trace Driver object and initializes it.
     * @param ProfilingFile The name of the API trace file from which to read 3D commands.
     * @param bufferFile File containing all buffers ( binary ) required por
     *        executing the trace
     * @return An initialized GL Trace Driver object.
     */
    TraceDriverOGL( const char* ProfilingFile, const char* bufferFile, const char* memFile, HAL* driver, bool triangleSetupOnShader, U32 startFrame = 0);

    /**
     * Starts the trace driver.
     *
     * Verifies if a GL TraceDriver object is correctly created and available for use
     * @return  0 if all is ok.
     *         -1 opengl functions cannot be loaded
     *         -2 if ProfilingFile could not be opened
     *         -3 if bufferFile could not be opened
     */
    int startTrace();

    /**
     *
     *  Generates the next MetaStream from the API trace file.
     *  @return A pointer to the new MetaStream, NULL if there are no more MetaStreams.
     *
     */
    cg1gpu::cgoMetaStream* nxtMetaStream();

    
    /**
     *  Returns the current position (line) inside the OpenGL trace file.
     *  @return The current position inside the OpenGL trace file (line).
     */

    U32 getTracePosition();

    // NOT AVAILABLE IN THE PUBLIC INTERFACE
    /**
     *
     * Call the specified OpenGL library command encoded in a TraceElement
     *
     * Indirectly called by nextcgoMetaStreams if there are not cgoMetaStreams
     * in HAL buffer
     *
     */
    //void performOGLlibraryCall( TraceElement* te );

};


#endif
