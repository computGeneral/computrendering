/**************************************************************************
 *
 */

#ifndef GLITOOLEXPORTS_H
    #define GLITOOLEXPORTS_H

#include "GLITool.h"

/**
 * This two methods must be exported in every GLITool DLL implemented
 */
extern "C"
{
    GLITool* APIENTRY getTool();
    void APIENTRY releaseTool(GLITool* tool);
}

#endif // GLITOOLEXPORTS_H
