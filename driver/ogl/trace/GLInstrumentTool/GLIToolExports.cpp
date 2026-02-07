/**************************************************************************
 *
 */

#include "GLIToolExports.h"
#include "GLIToolManager.h"

GLITool* APIENTRY getTool()
{
    static GLITool* tool = 0;
    if ( !tool )
        tool = GLIToolManager::instance().getTool();
    return tool;
}

void APIENTRY releaseTool(GLITool* tool)
{
    delete tool;
}
