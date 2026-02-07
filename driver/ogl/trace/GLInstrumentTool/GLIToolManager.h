/**************************************************************************
 *
 */

#ifndef GLITOOLMANAGER_H
    #define GLITOOLMANAGER_H

#include "GLITool.h"
#include <string>
#include <map>


class GLIToolManager
{
private:

    GLIToolManager();
    GLIToolManager(const GLIToolManager&);
    GLIToolManager& operator=(const GLIToolManager&);

public:

    static GLIToolManager& instance();
    GLITool* getTool();
};

#endif // GLITOOLMANAGER_H
